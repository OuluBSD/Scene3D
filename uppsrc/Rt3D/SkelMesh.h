#ifndef _Rt3D_SkelMesh_h_
#define _Rt3D_SkelMesh_h_

// The skinning/emission half of upstream's `skelmodel.h`, ported headless: the
// skinned vertex, the skinned mesh, the pose cache and the model that ties a
// skeleton, its meshes and its animations together. What a draw call would do
// with the result is GL-side and not ported (see Rt3D/AGENTS.md); what is here
// is everything between "an animation state" and "an array of transformed
// vertices".
//
// **Must be included after `SkelModel.h` and before `Iqm.h`** in `Rt3D.h`: the
// mesh holds a skeleton and the IQM reader fills these meshes, and sub-headers
// may not include each other.

// skelmodel.h's `vert` (upstream declares it in `render.h`): one skinned
// vertex. The tangent is the tangent *frame* as a quaternion - rotating
// `(1,0,0)` by it gives the tangent direction, and its `w` is the bitangent's
// sign - which is what a vertex buffer holds and what `fixqtangent`
// maintains. `blend` is the index of the skinning combination, `interpindex`
// the index that combination is *read* through once `PrepCpuSkinning` and
// `RemapBlend` have run: the two differ, because a rigidly bound vertex reads
// its bone's own pose entry while a blended one reads a slot in the blend
// array.
struct Rt3SkelVert : Moveable<Rt3SkelVert>
{
	Rt3Vec3 pos, norm;
	Rt3Quat tangent;
	double u = 0, v = 0;
	int blend = 0, interpindex = -1;

	// Upstream's `vert` ctor sets `tangent = quat(0,0,0,1)`, which is also what
	// a vertex of a model with no tangent array gets.
	Rt3SkelVert() : tangent(0, 0, 0, 1) {}
};

// animmodel.h's `fixqtangent`: a tangent quaternion's `w` is the bitangent's
// sign, and building or skinning one can flip it. Declared here because both
// the tangent frame construction (the IQM reader) and the skinning pass (this
// file) need it, and it must be one function, not two copies.
void Rt3FixQuatTangent(Rt3Quat& q, double bt);

// skelmodel.h's `tri`. `vert` indexes the owning mesh's own vertex array.
struct Rt3SkelTri
{
	int vert[3] = {0, 0, 0};
};

// skelmodel.h's `struct skelmesh`: one mesh of a skinned model, in the bind
// pose, plus the bookkeeping a draw call needs to address it inside the
// model's shared vertex and element buffers.
class Rt3SkelMesh : Moveable<Rt3SkelMesh>
{
public:
	String name, material;
	Vector<Rt3SkelVert> verts;
	Vector<Rt3SkelTri> tris;
	// The largest number of bone influences any of this mesh's vertices has
	// (upstream's `maxweights`), which picks the shader variant to draw it
	// with.
	int maxweights = 0;

	// Where this mesh lives in the model's shared buffers: `voffset` is its
	// first vertex, `eoffset` its first element index, and `elen` the number
	// of element indices - the three numbers `genvbo` leaves behind and the
	// draw call reads back. `minvert`/`maxvert` are the vertex range an
	// element index may name.
	int voffset = 0, eoffset = 0, elen = 0;
	int minvert = 0, maxvert = 0;

	// skelmesh's `genvbo` (the CPU branch, which upstream shares with the
	// `ANIM_NOSKIN` path): append this mesh's triangles to the model's element
	// list, rebased onto the model's vertex space, and record the range. The
	// return value is the number of vertices the mesh occupies, so a caller
	// can chain several meshes through one buffer.
	int GenVbo(Vector<int>& idxs, int offset);

	// skelmesh's `interpverts`: the CPU skinning pass for this mesh's slice
	// of `out`. `bdata` is the pose `InterpBones` produced, `blend` the
	// per-combination transforms `BlendBones` produced, and the two share one
	// flat index space - below `numgpubones` it is the pose array, from there
	// on the blend array shifted down by that much. The texture coordinates
	// are copied from the bind pose: nothing in a pose moves them.
	void InterpVerts(const Vector<Rt3DualQuat>& bdata, const Vector<Rt3DualQuat>& blend,
	                 int numgpubones, Vector<Rt3SkelVert>& out) const;
};

// skelmodel.h's `struct skelcacheentry`: the pose array a model was last
// evaluated into, plus the version it is good for. The version is what lets
// the renderer skip re-uploading bone uniforms for a draw whose pose has not
// changed; upstream stamps it with `Shader::uniformlocversion()`, a counter
// owned by the shader layer, which does not exist here - so the counter is
// this type's own (`NextUniformVersion`) and the renderer compares against it.
class Rt3SkelCacheEntry : Moveable<Rt3SkelCacheEntry>
{
public:
	Vector<Rt3DualQuat> bdata;
	int version = -1;

	// The state the pose in `bdata` was produced from, so that evaluating the
	// same state twice can be detected instead of guessed at. Upstream keeps
	// the whole `animstate` array in the cache entry and compares it; here
	// there is one state per entry, plus the pitch the pose was corrected to,
	// which is the other thing that can change a pose without changing the
	// animation state.
	Rt3AnimState as;
	double pitch = 0;

	void NextVersion() { version = NextUniformVersion(); }

	// Whether `bdata` already holds the pose for this state. Upstream's
	// equivalent is the `key->checkversion()` in `setshaderparams`, which asks
	// the *key* whether the cache entry's version is current; with no shader
	// layer the entry has to answer for itself.
	bool Matches(const Rt3AnimState& state, double pitch) const
	{
		return version >= 0 && pitch == this->pitch && as == state;
	}

	// Upstream's `Shader::uniformlocversion()` counter, including its wrap
	// around: a version that wraps past zero invalidates every key rather than
	// aliasing an old one.
	static int NextUniformVersion()
	{
		if(++lastversion <= 0)
		{
			lastversion = 1;
			return 0;
		}
		return lastversion;
	}

private:
	static int lastversion;
};

// A skinned model: a skeleton, the meshes bound to it, and the animation specs
// on the skeleton. Owns the shared buffers a draw call addresses: one skinned
// vertex array for all meshes, and the element list that indexes it.
class Rt3SkelModel : Moveable<Rt3SkelModel>
{
public:
	Rt3Skeleton skel;
	Vector<Rt3SkelMesh> meshes;
	// `elems` is the model's element index list, `vdata` the skinned vertices
	// all meshes share (`voffset`/`eoffset` index into them), and `vlen` the
	// vertex count. Upstream calls the element list `edata` and keeps it in a
	// GL element buffer; there is no buffer here, so it is a plain array.
	Vector<int> elems;
	Vector<Rt3SkelVert> vdata;
	int vlen = 0;

	// skelmeshgroup's `genvbo` (the CPU branch): lay every mesh out in the
	// shared buffers. The bind pose goes into `vdata` first, so a model can be
	// drawn before it is animated. Must run after the combinations have been
	// sorted and the bones remapped, and after `PrepCpuSkinning` - the same
	// order `Iqm` needs.
	void GenVbo();

	// animmodel.h's `skelmodel::animate` reduced to the part that does not need
	// a renderer: interpolate the pose for this animation state, collapse
	// every skinning combination into one transform, and skin every vertex
	// into `vdata`. The pose lands in `sc`, which is the caller's cache entry -
	// a model with several instances evaluates the pose once and shares it,
	// which is the whole reason the pose array is cached rather than stored.
	void Animate(const Rt3AnimState* as, Rt3SkelCacheEntry& sc, double pitch,
	             const Rt3Vec3& axis, const Rt3Vec3& forward, int numanimparts,
	             const byte* partmask);

	// The animation specs, i.e. the clips this model can play. A property
	// rather than a method only because it reads as one.
	int NumFrames() const { return skel.numframes; }
};

#endif

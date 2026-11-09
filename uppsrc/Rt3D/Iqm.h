#ifndef _Rt3D_Iqm_h_
#define _Rt3D_Iqm_h_

// Inter-Quake Model reader, ported from Tesseract-Sauerbraten's
// `src/engine/iqm.h` - the `loadiqmmeshes`/`loadiqmanims` half, with the
// `skelloader`/`skelmeshgroup` plumbing inlined (see below).
//
// IQM is the only *skeletal* model format this fork's assets actually use:
// `packages/models/ogro2/` ships the ogre's mesh in `ogro.iqm` and each of its
// animations in a separate file (`idle.iqm`, `run.iqm`, ...), each holding
// only poses and frame channels. There are no MD5 files in this fork, and MD3
// is loaded through the non-skeletal `vertmodel` path, so IQM is what the
// pedestrians will use.
//
// Nothing here is GL: the reader produces plain data (a bone table, keyframe
// poses, skinning combinations, a triangle mesh) that the data core in
// SkelModel.h consumes directly.
//
// Two properties of the format worth knowing before reading the code:
//  - everything is **little endian**. Upstream byte-swaps every array in
//    place after reading the whole file into one buffer; this port reads
//    through explicit little-endian accessors, so there is no swap-then-
//    re-read phase that could be half-applied,
//  - a **pose** is a per-joint position + orientation offset, and a **frame**
//    is a list of channels scaling those pose offsets over a frame range. So
//    poses are the key poses, and evaluating a frame is arithmetic over its
//    channels - not a lerp between two stored keyframes. That is also why the
//    keyframe pose array is only filled in for the frames a file provides.
//
// The structs below mirror upstream's `iqm*` structs, but they are *decoded*
// representations, not memory images: each is filled field by field through a
// little-endian accessor, so the compiler's padding and `double` alignment
// never touch the file layout. The on-disk sizes are in each comment.

enum
{
	RT3_IQM_POSITION = 0,
	RT3_IQM_TEXCOORD = 1,
	RT3_IQM_NORMAL = 2,
	RT3_IQM_TANGENT = 3,
	RT3_IQM_BLENDINDEXES = 4,
	RT3_IQM_BLENDWEIGHTS = 5,
	RT3_IQM_COLOR = 6,
	RT3_IQM_CUSTOM = 0x10
};

enum
{
	RT3_IQM_BYTE = 0, RT3_IQM_UBYTE = 1, RT3_IQM_SHORT = 2, RT3_IQM_USHORT = 3,
	RT3_IQM_INT = 4, RT3_IQM_UINT = 5, RT3_IQM_HALF = 6, RT3_IQM_FLOAT = 7,
	RT3_IQM_DOUBLE = 8
};

// On disk: the 16-byte magic "INTERQUAKEMODEL" followed by 27 `uint`s, so
// 124 bytes in total. Every count is immediately followed by the offset of
// the data it counts - except the three vertex-array counts, which share one
// offset - and that is the convention the reader walks. `numtext` counts the
// NUL-terminated strings packed at `ofstext`; **a name field is a byte offset
// into that block, not an index into it**, so the block itself is what the
// reader keeps.
struct Rt3IqmHeader
{
	dword version = 0;
	dword filesize = 0;
	dword flags = 0;
	dword numtext = 0, ofstext = 0;
	dword nummeshes = 0, ofsmeshes = 0;
	dword numvertexarrays = 0, numvertexes = 0, ofsvertexarrays = 0;
	dword numtriangles = 0, ofstriangles = 0, ofsadjacency = 0;
	dword numjoints = 0, ofsjoints = 0;
	dword numposes = 0, ofsposes = 0;
	dword numanims = 0, ofsanims = 0;
	dword numframes = 0, numframechannels = 0, ofsframes = 0, ofsbounds = 0;
	dword numcomment = 0, ofscomment = 0;
	dword numextensions = 0, ofsextensions = 0;
};

// On disk: 5 `uint`s, 20 bytes. `type` picks the attribute (an `RT3_IQM_*`
// constant), `format` the element type and `size` the components per vertex.
struct Rt3IqmVertexArray
{
	dword type = 0, flags = 0, format = 0, size = 0, offset = 0;
};

// On disk: 48 bytes - `uint name`, `int parent`, 3+4+3 floats.
struct Rt3IqmJoint
{
	dword name = 0;
	int parent = -1;
	Rt3Vec3 pos;
	Rt3Quat orient;
	Rt3Vec3 size;
};

// On disk: 88 bytes - `int parent`, `uint mask`, then 3+4+3 floats of offset
// and the same of scale. `mask` says which channels this joint actually
// stores, and **the reader must consume the frame data in the same order**,
// which is why the mask is walked bit by bit below.
struct Rt3IqmPose
{
	int parent = -1;
	dword mask = 0;
	Rt3Vec3 offsetpos;
	Rt3Quat offsetorient;
	Rt3Vec3 offsetsize;
	Rt3Vec3 scalepos;
	Rt3Quat scaleorient;
	Rt3Vec3 scalesize;
};

// On disk: 20 bytes - `uint name`, `uint firstframe`, `uint numframes`,
// `float framerate`, `uint flags`. (Upstream's struct spells `framerate` as a
// `float` too; `double` here because that is what the rest of Rt3D uses.)
struct Rt3IqmAnim
{
	dword name = 0;
	dword firstframe = 0, numframes = 0;
	double framerate = 0;
	dword flags = 0;
};

// On disk: 24 bytes - six `uint`s. A mesh is a *range* of the file's shared
// vertex and triangle arrays, not a copy of them.
struct Rt3IqmMesh
{
	dword name = 0;
	dword material = 0;
	dword firstvertex = 0, numvertexes = 0;
	dword firsttriangle = 0, numtriangles = 0;
};

// One mesh's worth of the parsed file: where its vertices and triangles start
// in the model's flat `verts`/`tris` lists, and the names it was read under.
// Recorded by `BuildMesh` so the flat lists can be split back into per-mesh
// `Rt3SkelMesh`es, which is the granularity the renderer draws at (a
// multi-material model has one skin per material).
struct Rt3IqmMeshPart : Moveable<Rt3IqmMeshPart>
{
	String name, material;
	int vertbase = 0, tribase = 0, numverts = 0, numtris = 0;
};

// A parsed IQM file. `verts`/`tris` start out holding the file's shared arrays
// in file order (triangles indexed by *global* vertex); `BuildMesh` appends
// the converted, per-mesh mesh to them, rebasing each mesh's triangle
// indexes to its own first vertex exactly as upstream does.
class Rt3IqmModel : Moveable<Rt3IqmModel>
{
public:
	// The path the model was read from. Kept because it is part of the
	// animation names (`AppendAnims` builds "<path>:<anim>") and because a
	// warning about a dropped mesh is useless without it.
	String path;
	Rt3IqmHeader hdr;
	// The text block itself: `hdr.numtext` NUL-terminated strings, read from
	// `hdr.ofstext`. Every name in the file is an offset into this.
	String text;
	Vector<Rt3IqmVertexArray> vertexarrays;
	Vector<Rt3IqmMesh> meshes;
	Vector<Rt3IqmJoint> joints;
	Vector<Rt3IqmPose> poses;
	Vector<Rt3IqmAnim> anims;
	// `hdr.numframes * hdr.numframechannels` channel values, unsigned shorts:
	// per frame, one (pose index, offset, scale) triple per used channel.
	Vector<word> framechannels;
	Vector<Rt3SkelVert> verts;
	Vector<Rt3SkelTri> tris;
	// One entry per mesh `BuildMesh` appended, in the same order.
	Vector<Rt3IqmMeshPart> meshparts;

	// Reads one file into a fully parsed model: magic, version 2, a sane size,
	// and every declared array inside the payload. A file that fails any check
	// leaves `out` empty and sets the error string - there is no such thing as
	// a half-loaded model. Upstream splits this into `loadiqm`'s two flags
	// (`load` reads meshes, `loadanim` reads animations); here both halves are
	// parsed and the caller picks what to build, because a file that contains
	// only poses is still a valid IQM file.
	static bool Load(const String& path, Rt3IqmModel& out, String& error);

	// The NUL-terminated string at a byte offset into the text block, or an
	// empty string when the file has no text block at all (upstream
	// substitutes `""` for a null `ofs_text` as well) or when the offset is
	// not inside the block. Upstream never bounds-checks - a bad name offset
	// walks off into the next section - and neither does the string it returns
	// end without a terminator, which is why the block is validated on load.
	String Text(dword offset) const;

	// iqm.h's `loadiqmmeshes`, bone table half: the joints become the
	// skeleton's bones, with the rest-pose `base`/`invbase` that every
	// animation pose is expressed against. Parents always precede their
	// children, so one forward pass builds the whole chain; `LinkChildren`
	// then wires the sibling lists. A skeleton that already has bones is
	// left untouched - that is how upstream shares one skeleton between
	// several meshes.
	bool BuildSkeleton(Rt3Skeleton& skel, String& error) const;

	// iqm.h's `loadiqmmeshes`, mesh half. Each mesh's vertices are converted
	// out of IQM's axis convention, skinned with up to four bone influences
	// (or, when the file has no blend arrays, bound rigidly through a single
	// zero-influence combination) and appended to `verts`/`tris`.
	bool BuildMesh(Rt3Skeleton& skel, String& error);

	// skelmodel.h's `sortblendcombos`: put the combination list in shader
	// order and renumber every vertex's `blend` to match. Must run after
	// every `BuildMesh` of the model, which is the only writer of combos.
	void SortBlendCombos(Rt3Skeleton& skel);

	// skelmodel.h's `genvbo` CPU branch: give every vertex the index its
	// combination is read through (`Rt3Skeleton::RemapBlend`). Must run after
	// `SortBlendCombos` (which renumbers the combinations), after
	// `RemapBones` (which fills the combinations' `interpbones`) and after
	// `Rt3Skeleton::PrepCpuSkinning` (which gives the multi-bone combinations
	// their slots).
	void RemapBlendVertices(Rt3Skeleton& skel);

	// The parsed mesh data as skinned meshes, one per mesh in the file, each
	// with its own name and material and holding a copy of the vertices and
	// triangles that belong to it, plus the widest combination's number of
	// influences (which picks the shader variant). The flat `verts`/`tris`
	// lists are left alone - they are the parse result, and this is the view of
	// them the renderer and `Rt3SkelModel` want. `skel` supplies the
	// combinations; must be the skeleton the mesh was built against. Empty for
	// a file whose meshes were all dropped.
	void BuildSkelMeshes(const Rt3Skeleton& skel, Vector<Rt3SkelMesh>& out) const;

	// iqm.h's `loadiqmanims`: append this file's animations to the
	// skeleton's keyframe poses, evaluating each frame's channels. The
	// animation is named `"<path>:<name>"`, which is what
	// `Rt3Skeleton::FindSkelAnim`'s `:` separator matches. An animation whose
	// name is already in the skeleton is skipped, so loading the same file
	// twice adds nothing.
	bool AppendAnims(Rt3Skeleton& skel, const String& path, String& error);

private:
	// The file's bytes, truncated to the header's own `filesize`. Kept after
	// the parse because the decoded arrays are all this reader needs, and a
	// model is small; it also means a failed parse can still say which file it
	// was looking at.
	String file;
	// The decoded per-vertex arrays, kept as plain component lists with
	// upstream's strides (position/normal/tangent 3 per vertex, texture
	// coordinate 2, blend index/weight 4) and the triangles still indexed by
	// the file's global vertex numbering.
	Vector<Rt3Vec3> vpos, vnorm, vtan;
	Vector<double> vtanw, vtc, vindex, vweight;
	Vector<Rt3SkelTri> rawtris;

	bool ReadFile(const String& path, String& error);
	bool ParseHeader(String& error);
	bool ParseVertexArrays(String& error);
	bool ReadDecodedArray(dword type, dword format, dword want, Vector<double>& out,
	                        bool& present, String& error);
};

#endif

#ifndef _Rt3D_SkelModel_h_
#define _Rt3D_SkelModel_h_

// Skeletal animation data core, ported from Tesseract-Sauerbraten's
// `src/engine/skelmodel.h`.
//
// Scope: the **data** half of upstream's `skelmodel : animmodel` - the bone
// table, animation specs, tags, antipodes and the pitch-correction tables,
// with the math that operates on them. Everything that needs a GL context
// (meshes, blend combos, skinning, the VBO/VAO caches, `remapbones`) is
// deliberately not here; see Rt3D/AGENTS.md and the phase plan
// (`004-task3-animmodel-and-skeletal-animation.md`).
//
// The types are value types with U++ containers; upstream's `char *name`
// plus `newstring`/`DELETEA` becomes `String`, and `vector<T>` becomes
// `Vector<T>`. Loop macros (`loopv`/`loopi`/`loopj`) become indexed loops.

// skelmodel.h's bonemask constants. `ApplyBoneMask` walks a `ushort` stream
// of bone indices; a `0` bit in the high half means "clear the subtree".
enum
{
	RT3_BONEMASK_NOT  = 0x8000,
	RT3_BONEMASK_END  = 0xFFFF,
	RT3_BONEMASK_BONE = 0x7FFF
};

// skelmodel.h's `struct tag`: a named point bound to a bone, with the
// transform from bone space into the model's space.
struct Rt3Tag : Moveable<Rt3Tag>
{
	String name;
	int bone = -1;
	Rt3Matrix4x3 matrix;

	Rt3Tag() = default;
	Rt3Tag(const String& name, int bone, const Rt3Matrix4x3& matrix)
		: name(name), bone(bone), matrix(matrix) {}
};

// skelmodel.h's `struct skelanimspec`: the frame an animation starts at and
// how many frames it spans, under its own name (e.g. "run" or "attack 1").
// A name may be a group, with the variants appended after a colon
// ("group:variant"), which is what `FindSkelAnim`'s `sep` argument matches.
struct Rt3SkelAnimSpec : Moveable<Rt3SkelAnimSpec>
{
	String name;
	int frame = 0;
	int range = 0;

	Rt3SkelAnimSpec() = default;
	Rt3SkelAnimSpec(const String& name, int frame, int range)
		: name(name), frame(frame), range(range) {}
};

// skelmodel.h's `struct boneinfo`: one row of the bone table. `parent` is
// the index of the parent bone (-1 at the root), `children`/`next` the
// sibling lists built by `LinkChildren`, and the rest are caches and
// constraints filled in by later passes:
//  - `group`    : antipode constraint, see `CalcAntipodes`
//  - `scheduled`: order in which `CalcAntipodes` visited the bone
//  - `interpindex`/`interpparent`: compacted bone numbering for the
//                 interpolated pose, assigned by `RemapBones` (3B)
//  - `base`/`invbase`: rest-pose transform, set by the model loader
struct Rt3BoneInfo : Moveable<Rt3BoneInfo>
{
	String name;
	int parent = -1;
	int children = -1;
	int next = -1;
	int group = INT_MAX;
	int scheduled = -1;
	int interpindex = -1;
	int interpparent = -1;
	int correctindex = -1;
	double pitchscale = 0, pitchoffset = 0, pitchmin = 0, pitchmax = 0;
	Rt3DualQuat base, invbase;

	Rt3BoneInfo() = default;
	explicit Rt3BoneInfo(const String& name) : name(name) {}
};

// skelmodel.h's `struct antipode`: a pair of bones that must not be blended
// in the same interpolated pose, because their constraints conflict.
struct Rt3Antipode : Moveable<Rt3Antipode>
{
	int parent, child;

	Rt3Antipode() : parent(-1), child(-1) {}
	Rt3Antipode(int parent, int child) : parent(parent), child(child) {}
};

// A bone whose pose is snapshotted for a pitch-correction pass, with the
// index of its own parent entry in the dep list (`-1` for a root bone).
struct Rt3PitchDep : Moveable<Rt3PitchDep>
{
	int bone = -1;
	int parent = -1;
	Rt3DualQuat pose;
};

// A bone/frame pair that is corrected for when the model is pitched (a head
// that should keep looking level, for example).
struct Rt3PitchTarget : Moveable<Rt3PitchTarget>
{
	int bone = -1;
	int frame = 0;
	int corrects = -1;
	int deps = -1;
	double pitchmin = 0, pitchmax = 0, deviated = 0;
	Rt3DualQuat pose;
};

// One correction's contribution to a target: how much of the target's
// remaining pitch budget this bone takes, within its own limits.
struct Rt3PitchCorrect : Moveable<Rt3PitchCorrect>
{
	int bone, target, parent;
	double pitchmin, pitchmax, pitchscale, pitchangle, pitchtotal;

	Rt3PitchCorrect() : bone(-1), target(-1), parent(-1), pitchmin(0), pitchmax(0),
	                    pitchscale(0), pitchangle(0), pitchtotal(0) {}
	Rt3PitchCorrect(int bone, int target, int parent, double pitchmin, double pitchmax, double pitchscale)
		: bone(bone), target(target), parent(parent), pitchmin(pitchmin), pitchmax(pitchmax),
		  pitchscale(pitchscale), pitchangle(0), pitchtotal(0) {}
};

// skelmodel.h's `struct blendcombo`: one vertex's skinning - up to four
// (bone, weight) pairs, sorted by descending weight, normalized to sum 1.
// The same combination shows up once per vertex of a mesh, so a model
// collapses thousands of these into a short list. `interpbones` and
// `interpindex` are assigned by `RemapBones`.
struct Rt3BlendCombo
{
	int uses = 1;
	int interpindex = -1;
	double weights[4] = {0, 0, 0, 0};
	byte bones[4] = {0, 0, 0, 0};
	byte interpbones[4] = {0, 0, 0, 0};

	// The number of entries in use: the weights are sorted, so it is the
	// length of the run of nonzero weights.
	int Size() const
	{
		int i = 1;
		while(i < 4 && weights[i]) i++;
		return i;
	}

	bool operator==(const Rt3BlendCombo& c) const
	{
		for(int k = 0; k < 4; k++)
			if(bones[k] != c.bones[k] || weights[k] != c.weights[k]) return false;
		return true;
	}

	// skelmodel.h's `addweight`: insert a (bone, weight) pair keeping the
	// weights descending, and return how many entries are in use. Weights of
	// 1e-3 or less are dropped, which is why a 5th influence disappears.
	int AddWeight(int sorted, double weight, int bone)
	{
		if(weight <= 1e-3) return sorted;
		for(int k = 0; k < sorted; k++)
			if(weight > weights[k])
			{
				for(int l = min(sorted - 1, 2); l >= k; l--)
				{
					weights[l + 1] = weights[l];
					bones[l + 1] = bones[l];
				}
				weights[k] = weight;
				bones[k] = (byte)bone;
				return sorted < 4 ? sorted + 1 : sorted;
			}
		if(sorted >= 4) return sorted;
		weights[sorted] = weight;
		bones[sorted] = (byte)bone;
		return sorted + 1;
	}

	// skelmodel.h's `blendcombo::sortcmp`: the sort order `sortblendcombos`
	// puts the list in - by descending number of influences, so the short
	// combinations (which a shader can compute cheaply) come first.
	static bool SortCmp(const Rt3BlendCombo& x, const Rt3BlendCombo& y)
	{
		for(int i = 0; i < 4; i++)
		{
			if(x.weights[i])
			{
				if(!y.weights[i]) return true;
			}
			else if(y.weights[i]) return false;
			else break;
		}
		return false;
	}

	// skelmodel.h's `finalize`: clear the unused entries and normalize what
	// is left to sum 1.
	void Finalize(int sorted)
	{
		for(int j = 0; j < 4 - sorted; j++)
		{
			weights[sorted + j] = 0;
			bones[sorted + j] = 0;
		}
		if(sorted <= 0) return;
		double total = 0;
		for(int j = 0; j < sorted; j++) total += weights[j];
		total = 1 / total;
		for(int j = 0; j < sorted; j++) weights[j] *= total;
	}
};

// skelmodel.h's `struct skeleton`: the bone table plus the keyframe poses
// (`framebones`, `nframes` frames of `numbones` dualquaternions each) and
// everything derived from them.
//
// `testTags` replaces upstream's `VAR(testtags, 0, 0, 1)` global: when false,
// `AddTag` refuses to replace an existing tag (a model file is not allowed to
// silently redefine one).
class Rt3Skeleton : Moveable<Rt3Skeleton>
{
public:
	String name;
	int shared = 0;
	int numbones = 0;
	int numinterpbones = 0;
	int numframes = 0;
	// The bones that the GPU interpolates (assigned by `RemapBones`), which
	// is a subset of the bones - the ones no vertex is actually weighted to.
	int numgpubones = 0;
	// The combinations that need a slot of their own in the CPU blend array
	// (the multi-bone ones), assigned by `PrepCpuSkinning`. Upstream calls
	// this `vblends`.
	int numblendbones = 0;
	// `nframes * numbones` keyframe poses, or empty for a skeleton with no
	// animated frames. Upstream is a raw `dualquat *`; a `Vector` gives the
	// same contiguous `frame * numbones + bone` indexing with bounds asserts
	// and no manual free.
	Vector<Rt3DualQuat> framebones;
	Vector<Rt3BoneInfo> bones;
	Vector<Rt3SkelAnimSpec> skelanims;
	Vector<Rt3Tag> tags;
	Vector<Rt3Antipode> antipodes;
	Vector<Rt3PitchDep> pitchdeps;
	Vector<Rt3PitchTarget> pitchtargets;
	Vector<Rt3PitchCorrect> pitchcorrects;
	// Every distinct skinning combination in the model, filled by the model
	// loader. Upstream keeps these per mesh group and walks the groups in
	// `remapbones`; the groups are a GL-side grouping, so the list is flat
	// here and the iteration order is preserved.
	Vector<Rt3BlendCombo> blendcombos;

	// upstream's global `testtags`, which `addtag` consults.
	static bool testTags;

	// skelmodel.h's `findskelanim`. With `sep` non-zero, the name is matched
	// against the part of an entry's name before the first colon, and the
	// exact name is tried as well; without it only the exact name matches.
	int FindSkelAnim(const String& name, char sep = '\0') const;
	Rt3SkelAnimSpec& AddSkelAnim(const String& name);

	// skelmodel.h's `skelmeshgroup::addblendcombo`: find an identical
	// combination and count one more use of it, or append it. Returns the
	// entry's `interpindex`, which is what a vertex stores as its `blend`.
	// Upstream additionally keeps a per-influence-count histogram for the
	// shader choice; that is GL-side and not ported.
	int AddBlendCombo(const Rt3BlendCombo& c);

	// ---- CPU skinning (the `!usegpuskel` half of `skelloader`) ----
	//
	// On the GPU the skinning is a vertex-shader pass over the bone poses, and
	// every combination's bones are sent to the shader. Without a GPU the
	// skeleton collapses each combination into **one** dual quaternion once
	// per frame, and the vertex pass then applies that single transform - so
	// this is the CPU equivalent of the same picture, and the two agree to
	// within the blend's own error.

	// skelloader's `genvbo` CPU branch: every combination with more than one
	// influence gets a slot of its own in the blend array, numbered from
	// `numgpubones` on; a single-bone combination has no slot (its transform
	// is the bone's own, already in the pose array). Sets `numblendbones`.
	void PrepCpuSkinning();

	// skelmeshgroup's `remapblend`: the index a vertex's combination is read
	// through - the slot for a multi-bone combination, the bone itself for a
	// single-bone one.
	int RemapBlend(int blend) const;

	// skelloader's `blendbones`: the per-frame transform of every combination,
	// `numblendbones` entries, each the normalized weighted sum of its bones'
	// poses. `bdata` is the pose array `InterpBones` produced.
	void BlendBones(const Vector<Rt3DualQuat>& bdata, Vector<Rt3DualQuat>& blend) const;

	int FindBone(const String& name) const;
	int FindTag(const String& name) const;
	bool AddTag(const String& name, int bone, const Rt3Matrix4x3& matrix);

	// skelmodel.h's `skeleton::calctags`: the world-space transform of every
	// tag, one per tag, in tag order. `bdata` is the pose `InterpBones`
	// produced, or null for the rest pose - upstream's `sc` argument with a
	// null cache entry, which is the same switch. `scale` is the model's own
	// scale (`p->model->scale * sizescale` upstream): it multiplies the
	// translation only, which is why it is a parameter here rather than
	// something the tag matrix knows.
	//
	// This is the whole of a bone-attached point: what a weapon in a hand, a
	// shield on an arm or a powerup on the chest is placed from, and the only
	// part of a skeletal model that survives without a renderer.
	void CalcTags(const Vector<Rt3DualQuat>* bdata, double scale,
	              Vector<Rt3Matrix4x3>& out) const;

	// skelmodel.h's `linkchildren`: build the `children`/`next` sibling lists
	// from `parent`. Idempotent.
	void LinkChildren();

	// skelmodel.h's `calcantipodes`: walk the bone table and record which
	// bones conflict, so the renderer can never blend both into one pose.
	void CalcAntipodes();

	// skelmodel.h's `remapbones`: give every bone a compact index in the
	// interpolated pose, work out which bones are grouped for antipode
	// purposes (`boneinfo::group`), and chain each index to its parent's
	// (`boneinfo::interpparent`). Must run after the blend combos are in
	// and before `InterpBones`; it ends by calling `CalcAntipodes`.
	void RemapBones();

	// skelmodel.h's `interpbones`: the actual pose. For each bone it blends
	// the current keyframe pair (and, while a blend is in progress, the
	// previous pair too) with each part's own weight, composes the result
	// with its parent, applies the pitch corrections, and finally resolves
	// the antipodes. `bdata` is resized to `numinterpbones` and indexed by
	// `boneinfo::interpindex`.
	//
	// Upstream writes into a cached buffer and bumps the shader's uniform
	// version; this port hands back the buffer instead, because the cache
	// belongs to the renderer task.
	void InterpBones(const Rt3AnimState* as, double pitch, const Rt3Vec3& axis,
	                 const Rt3Vec3& forward, int numanimparts, const byte* partmask,
	                 Vector<Rt3DualQuat>& bdata);

	// Pitch-correction passes. `InitPitchDeps` snapshots the target bones'
	// poses out of `framebones` and chains them to their parents;
	// `CalcPitchCorrects` distributes a pitch deviation over the corrects.
	void AddPitchDep(int bone, int frame);
	int FindPitchDep(int bone) const;
	int FindPitchCorrect(int bone) const;
	void InitPitchDeps();
	void CalcPitchCorrects(double pitch, const Rt3Vec3& axis, const Rt3Vec3& forward);

	// skelmodel.h's `calcdeviation`, in **degrees** (`atan2(dy, dx) / RAD`):
	// how far `forward` has swung off `axis` between two poses. The sign
	// comes from the (negated) cross product dotted with `axis`, so a
	// positive result is the positive rotation about `axis`.
	static double CalcDeviation(const Rt3Vec3& axis, const Rt3Vec3& forward,
	                            const Rt3DualQuat& pose1, const Rt3DualQuat& pose2);

	// skelmodel.h's `expandbonemask`/`applybonemask`: a bonemask is a
	// `ushort` stream of bone indices, terminated by `RT3_BONEMASK_END`, each
	// setting (or, with `RT3_BONEMASK_NOT`, clearing) that bone *and its
	// whole subtree* in the part mask.
	void ExpandBoneMask(Vector<int>& expansion, int bone, int val) const;
	void ApplyBoneMask(const Vector<word>& mask, Vector<byte>& partmask, int partindex) const;

	// `framebones` accessors with the `frame * numbones + bone` index
	// arithmetic the upstream interpolation loops do inline.
	const Rt3DualQuat& BoneFrame(int frame, int bone) const
	{
		return framebones[frame * numbones + bone];
	}
};


#endif

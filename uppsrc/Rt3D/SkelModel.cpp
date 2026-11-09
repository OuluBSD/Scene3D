#include "Rt3D.h"

NAMESPACE_UPP

// animmodel.h's `VAR(testtags, 0, 0, 1)` defaults to off, so a model file
// cannot redefine a tag that is already there.
bool Rt3Skeleton::testTags = false;

// skelmodel.h's `findskelanim`. Upstream returns a pointer, and the group
// form compares the text before the first ':' in the entry's name; the exact
// name is tried as well so "attack 1" can be found by its own name.
int Rt3Skeleton::FindSkelAnim(const String& name, char sep) const
{
	int len = sep ? (int)name.GetLength() : 0;
	for(int i = 0; i < skelanims.GetCount(); i++)
	{
		const String& n = skelanims[i].name;
		if(n.IsEmpty()) continue;
		if(sep)
		{
			int end = n.FindFirstOf(":");
			if(end >= 0 && end == len && n.Mid(0, len) == name) return i;
		}
		if(n == name) return i;
	}
	return -1;
}

Rt3SkelAnimSpec& Rt3Skeleton::AddSkelAnim(const String& name)
{
	skelanims.Add(Rt3SkelAnimSpec(name, 0, 0));
	return skelanims.Top();
}

int Rt3Skeleton::AddBlendCombo(const Rt3BlendCombo& c)
{
	for(int i = 0; i < blendcombos.GetCount(); i++)
		if(blendcombos[i] == c)
		{
			blendcombos[i].uses += c.uses;
			return i;
		}
	blendcombos.Add(c);
	// `interpindex` is the position this entry had when it was added, which is
	// what `sortblendcombos` remaps the vertices through after re-sorting the
	// list into shader order.
	blendcombos.Top().interpindex = blendcombos.GetCount() - 1;
	return blendcombos.GetCount() - 1;
}

int Rt3Skeleton::RemapBlend(int blend) const
{
	// Upstream returns `c.interpindex` for a multi-bone combination (its slot
	// in the blend array) and the bone's own index for a single-bone one - so a
	// vertex bound rigidly to a bone needs no blend slot at all.
	if(blend < 0 || blend >= blendcombos.GetCount()) return -1;
	const Rt3BlendCombo& c = blendcombos[blend];
	return c.weights[1] ? c.interpindex : c.interpbones[0];
}

void Rt3Skeleton::PrepCpuSkinning()
{
	// The slots are numbered from `numgpubones` on, so that a single flat
	// array can hold both the pose (`< numgpubones`) and the blends
	// (`>= numgpubones`) - which is how `interpverts` indexes it upstream
	// (`bdata2 -= blendoffset`).
	numblendbones = 0;
	for(int i = 0; i < blendcombos.GetCount(); i++)
	{
		Rt3BlendCombo& c = blendcombos[i];
		c.interpindex = c.weights[1] ? numgpubones + numblendbones++ : -1;
	}
}

void Rt3Skeleton::BlendBones(const Vector<Rt3DualQuat>& bdata, Vector<Rt3DualQuat>& blend) const
{
	blend.SetCount(numblendbones);
	// `c.interpindex` is only >= 0 for the combinations that got a slot, and
	// they are numbered in order, so the walk can stop at the first -1.
	for(int i = 0; i < blendcombos.GetCount(); i++)
	{
		const Rt3BlendCombo& c = blendcombos[i];
		if(c.interpindex < 0) break;
		Rt3DualQuat& d = blend[c.interpindex - numgpubones];
		d = bdata[c.interpbones[0]];
		d.Mul(c.weights[0]);
		d.Accumulate(bdata[c.interpbones[1]], c.weights[1]);
		// The two deep cases are nested upstream, and stay nested: a
		// three-influence combination must not read a fourth bone's zeroed
		// entry.
		if(c.weights[2])
		{
			d.Accumulate(bdata[c.interpbones[2]], c.weights[2]);
			if(c.weights[3]) d.Accumulate(bdata[c.interpbones[3]], c.weights[3]);
		}
		// The weighted sum of dual quaternions is not a rotation, so the
		// result is renormalized - which is what makes the CPU path agree with
		// the shader's per-vertex blend to within the blend's own slop.
		d.Normalize();
	}
}

int Rt3Skeleton::FindBone(const String& name) const
{
	for(int i = 0; i < bones.GetCount(); i++)
		if(bones[i].name == name) return i;
	return -1;
}

int Rt3Skeleton::FindTag(const String& name) const
{
	for(int i = 0; i < tags.GetCount(); i++)
		if(tags[i].name == name) return i;
	return -1;
}

// Upstream replaces an existing tag in place, but only when `testtags` is on;
// with it off the add is refused so a model's tags are all distinct.
bool Rt3Skeleton::AddTag(const String& name, int bone, const Rt3Matrix4x3& matrix)
{
	int idx = FindTag(name);
	if(idx >= 0)
	{
		if(!testTags) return false;
		tags[idx].bone = bone;
		tags[idx].matrix = matrix;
	}
	else
		tags.Add(Rt3Tag(name, bone, matrix));
	return true;
}

// skelmodel.h's `skeleton::calctags`. Note the composition order: the pose on
// the bone's base, and the tag's own matrix after both - a tag is expressed in
// the bone's space, so a bone that rotates takes its tag along with it.
void Rt3Skeleton::CalcTags(const Vector<Rt3DualQuat>* bdata, double scale,
                           Vector<Rt3Matrix4x3>& out) const
{
	out.SetCount(tags.GetCount());
	for(int i = 0; i < tags.GetCount(); i++)
	{
		const Rt3Tag& t = tags[i];
		// Upstream cannot reach this case: `settag` refuses a tag on a bone
		// the skeleton does not have, so `calctags` is only ever handed tags
		// that resolve. `AddTag` here is a plain call that validates nothing,
		// and a default-constructed `Rt3Matrix4x3` is all zeros - a tag that
		// flattened everything into the origin. The identity is the honest
		// answer for "this tag is nowhere".
		if(t.bone < 0 || t.bone >= bones.GetCount())
		{
			out[i].Identity();
			continue;
		}
		const Rt3BoneInfo& b = bones[t.bone];
		Rt3DualQuat q;
		if(bdata && b.interpindex >= 0) q.Mul((*bdata)[b.interpindex], b.base);
		else q = b.base;
		Rt3Matrix4x3 m(q);
		m.Mul(t.matrix);
		// Upstream's `m.d.mul(p->model->scale * sizescale)`: the translation
		// scales, the rotation does not.
		m.d.Mul(scale);
		out[i] = m;
	}
}

// skelmodel.h's `linkchildren`. `children` is the first child and `next` the
// next sibling, both singly linked off the bone's own index.
void Rt3Skeleton::LinkChildren()
{
	for(int i = 0; i < bones.GetCount(); i++)
	{
		Rt3BoneInfo& b = bones[i];
		b.children = -1;
		if(b.parent < 0) b.next = -1;
		else
		{
			b.next = bones[b.parent].children;
			bones[b.parent].children = i;
		}
	}
}

// skelmodel.h's `calcantipodes`. This is a *scheduling* pass, not a search:
// the `group` fields are pre-existing constraints (a bone with
// `group >= numbones` is scheduled first, a negative group resolves through
// `abs()`), bones are visited in work-list order, and every bone that
// conflicts with an already-scheduled one is recorded as an antipode and
// scheduled itself so the conflict is never revisited. The
// `i + 1 == schedule.length()` branch is upstream's guard against a bone
// blocking a conflict from ever being reached - the extra entry it schedules
// is part of the result, not an optimization.
void Rt3Skeleton::CalcAntipodes()
{
	antipodes.Clear();
	Vector<int> schedule;
	for(int i = 0; i < numbones; i++)
	{
		if(bones[i].group >= numbones)
		{
			bones[i].scheduled = schedule.GetCount();
			schedule.Add(i);
		}
		else bones[i].scheduled = -1;
	}
	for(int i = 0; i < schedule.GetCount(); i++)
	{
		int bone = schedule[i];
		const Rt3BoneInfo& info = bones[bone];
		for(int j = 0; j < numbones; j++)
			if(Upp::abs(bones[j].group) == bone && bones[j].scheduled < 0)
			{
				antipodes.Add(Rt3Antipode(info.interpindex, bones[j].interpindex));
				bones[j].scheduled = schedule.GetCount();
				schedule.Add(j);
			}
		if(i + 1 == schedule.GetCount())
		{
			// No bone was scheduled by the pass above, so a blocked conflict
			// would never be reached: pull the lowest such bone in now.
			int conflict = INT_MAX;
			for(int j = 0; j < numbones; j++)
				if(bones[j].group < numbones && bones[j].scheduled < 0)
					conflict = min(conflict, Upp::abs(bones[j].group));
			if(conflict < numbones)
			{
				bones[conflict].scheduled = schedule.GetCount();
				schedule.Add(conflict);
			}
		}
	}
}

// skelmodel.h's `addpitchdep`: walk from `bone` up through its parents,
// snapshotting each one's pose at `frame` into the bone-sorted dep list. The
// inner scan is a linear insert (upstream uses a `goto` to skip the insert
// when the bone is already there) - the sortedness is what `findpitchdep`
// later relies on for its early exit.
void Rt3Skeleton::AddPitchDep(int bone, int frame)
{
	for(; bone >= 0; bone = bones[bone].parent)
	{
		int pos = pitchdeps.GetCount();
		bool have = false;
		for(int j = 0; j < pitchdeps.GetCount(); j++)
			if(bone <= pitchdeps[j].bone)
			{
				if(bone == pitchdeps[j].bone) { have = true; break; }
				pos = j;
				break;
			}
		if(have) continue;
		Rt3PitchDep d;
		d.bone = bone;
		d.parent = -1;
		d.pose = framebones[frame * numbones + bone];
		pitchdeps.Insert(pos, pick(d));
	}
}

int Rt3Skeleton::FindPitchDep(int bone) const
{
	for(int i = 0; i < pitchdeps.GetCount(); i++)
		if(bone <= pitchdeps[i].bone) return bone == pitchdeps[i].bone ? i : -1;
	return -1;
}

int Rt3Skeleton::FindPitchCorrect(int bone) const
{
	for(int i = 0; i < pitchcorrects.GetCount(); i++)
		if(bone <= pitchcorrects[i].bone) return bone == pitchcorrects[i].bone ? i : -1;
	return -1;
}

// skelmodel.h's `initpitchdeps`, in its three ordered passes:
//  1. every target's `deps` starts unset and its whole parent chain is
//     snapshotted (the insert keeps the list bone-sorted, so a bone shared
//     by two targets is only stored once),
//  2. each dep is linked to its parent's dep and its pose is expressed in
//     the parent's space - note the operand order, the *parent* is the left
//     operand: `d.pose = parent.pose * d.pose`,
//  3. each target adopts its dep's pose and finds the nearest ancestor that
//     has a pitch correct, and each correct finds its own parent correct,
//     which is written back into the bone table as `correctindex`.
void Rt3Skeleton::InitPitchDeps()
{
	pitchdeps.Clear();
	if(pitchtargets.IsEmpty()) return;
	for(int i = 0; i < pitchtargets.GetCount(); i++)
	{
		pitchtargets[i].deps = -1;
		AddPitchDep(pitchtargets[i].bone, pitchtargets[i].frame);
	}
	for(int i = 0; i < pitchdeps.GetCount(); i++)
	{
		Rt3PitchDep& d = pitchdeps[i];
		int parent = bones[d.bone].parent;
		if(parent >= 0)
		{
			int j = FindPitchDep(parent);
			if(j >= 0)
			{
				d.parent = j;
				// `d.pose = parent.pose * d.pose`, with `d.pose` copied out
				// first because the product writes in place.
				Rt3DualQuat self = d.pose;
				d.pose.Mul(pitchdeps[j].pose, self);
			}
		}
	}
	for(int i = 0; i < pitchtargets.GetCount(); i++)
	{
		Rt3PitchTarget& t = pitchtargets[i];
		int j = FindPitchDep(t.bone);
		if(j >= 0)
		{
			t.deps = j;
			t.pose = pitchdeps[j].pose;
		}
		t.corrects = -1;
		for(int parent = t.bone; parent >= 0; parent = bones[parent].parent)
		{
			t.corrects = FindPitchCorrect(parent);
			if(t.corrects >= 0) break;
		}
	}
	for(int i = 0; i < pitchcorrects.GetCount(); i++)
	{
		Rt3PitchCorrect& c = pitchcorrects[i];
		bones[c.bone].correctindex = i;
		c.parent = -1;
		for(int parent = c.bone;;)
		{
			parent = bones[parent].parent;
			if(parent < 0) break;
			c.parent = FindPitchCorrect(parent);
			if(c.parent >= 0) break;
		}
	}
}

// skelmodel.h's `calcdeviation`, in degrees. `forward` is rotated by each
// pose, projected onto `axis` (removing the component along it) and
// normalized; the angle between the two results is the deviation, signed by
// the dot of the negated cross product with `axis`.
double Rt3Skeleton::CalcDeviation(const Rt3Vec3& axis, const Rt3Vec3& forward,
                                  const Rt3DualQuat& pose1, const Rt3DualQuat& pose2)
{
	Rt3Vec3 forward1 = pose1.TransformNormal(forward);
	forward1.Project(axis);
	forward1 = forward1.Normalized();
	Rt3Vec3 forward2 = pose2.TransformNormal(forward);
	forward2.Project(axis);
	forward2 = forward2.Normalized();
	// geom.h: `vec().cross(forward1, forward2)` writes into the receiver,
	// which starts as a null vector, so the sign is inverted.
	Rt3Vec3 daxis = -forward1.Cross(forward2);
	double dx = clamp(forward1.Dot(forward2), -1.0, 1.0), dy = clamp(daxis.Length(), -1.0, 1.0);
	if(daxis.Dot(axis) < 0) dy = -dy;
	return atan2(dy, dx) * 180.0 / M_PI;
}

// skelmodel.h's `calcpitchcorrects`, three ordered passes: every target
// measures how far it has already deviated, every correct's accumulated
// angle is zeroed, and then each target hands its remaining pitch to its
// corrects, deepest ancestor first. The clamp chain runs twice on purpose -
// once against the correct's own limits, once against what is left of the
// target's budget after its ancestors took theirs.
void Rt3Skeleton::CalcPitchCorrects(double pitch, const Rt3Vec3& axis, const Rt3Vec3& forward)
{
	for(int i = 0; i < pitchtargets.GetCount(); i++)
	{
		Rt3PitchTarget& t = pitchtargets[i];
		t.deviated = CalcDeviation(axis, forward, t.pose, pitchdeps[t.deps].pose);
	}
	for(int i = 0; i < pitchcorrects.GetCount(); i++)
	{
		pitchcorrects[i].pitchangle = pitchcorrects[i].pitchtotal = 0;
	}
	for(int j = 0; j < pitchtargets.GetCount(); j++)
	{
		Rt3PitchTarget& t = pitchtargets[j];
		double tpitch = pitch - t.deviated;
		for(int parent = t.corrects; parent >= 0; parent = pitchcorrects[parent].parent)
			tpitch -= pitchcorrects[parent].pitchangle;
		if(t.pitchmin || t.pitchmax) tpitch = clamp(tpitch, t.pitchmin, t.pitchmax);
		for(int i = 0; i < pitchcorrects.GetCount(); i++)
		{
			Rt3PitchCorrect& c = pitchcorrects[i];
			if(c.target != j) continue;
			double total = c.parent >= 0 ? pitchcorrects[c.parent].pitchtotal : 0,
			       avail = tpitch - total,
			       used = tpitch * c.pitchscale;
			if(c.pitchmin || c.pitchmax)
			{
				if(used < 0) used = clamp(c.pitchmin, used, 0.0);
				else used = clamp(c.pitchmax, 0.0, used);
			}
			if(used < 0) used = clamp(avail, used, 0.0);
			else used = clamp(avail, 0.0, used);
			c.pitchangle = used;
			c.pitchtotal = used + total;
		}
	}
}

// skelmodel.h's `expandbonemask`: set `val` on `bone` and everything below
// it, following `children`/`next` - so a mask names a bone and covers its
// whole subtree.
void Rt3Skeleton::ExpandBoneMask(Vector<int>& expansion, int bone, int val) const
{
	expansion[bone] = val;
	bone = bones[bone].children;
	while(bone >= 0)
	{
		ExpandBoneMask(expansion, bone, val);
		bone = bones[bone].next;
	}
}

// skelmodel.h's `applybonemask`: walk a bone-index stream, expand each entry
// over its subtree, and stamp the result into `partmask` as `partindex`. The
// stream starts from a base value of "set" unless its first entry has the
// NOT bit, which makes the first entry a clearing one.
void Rt3Skeleton::ApplyBoneMask(const Vector<word>& mask, Vector<byte>& partmask, int partindex) const
{
	if(mask.IsEmpty() || mask[0] == RT3_BONEMASK_END) return;
	Vector<int> expansion;
	expansion.SetCount(numbones);
	int base = (mask[0] & RT3_BONEMASK_NOT) ? 1 : 0;
	for(int i = 0; i < numbones; i++) expansion[i] = base;
	for(int i = 0; mask[i] != RT3_BONEMASK_END; i++)
		ExpandBoneMask(expansion, mask[i] & RT3_BONEMASK_BONE,
		               (mask[i] & RT3_BONEMASK_NOT) ? 0 : 1);
	for(int i = 0; i < numbones; i++)
		if(expansion[i]) partmask[i] = (byte)partindex;
}

// skelmodel.h's `remapbones`, in its four ordered parts:
//
//  1. every bone referenced by a vertex gets a compact index
//     (`interpindex`), numbered in the order the blend combos mention them,
//     and the `group` field is worked out at the same time. `group` is a
//     *constraint*: a bone whose parent (or grandparent) is also weighted to
//     the same vertex cannot be interpolated independently of it, so its group
//     is negated to point at the offending parent, and otherwise it points at
//     whichever bone in the combo shares a lineage with it. `CalcAntipodes`
//     reads these groups to find the pairs that must never be blended
//     together,
//  2. tags and then any bone that is a parent of an interpolated bone are
//     given indices too, so the two never collide, and `interpparent` is
//     filled in - the renderer walks the pose in `interpindex` order and
//     composes with the parent it was handed,
//  3. the ragdoll joints would be indexed here too; this port has no ragdoll,
//  4. and it ends with `CalcAntipodes`.
//
// Upstream's ragdoll block between 2 and 4 is the only thing skipped.
void Rt3Skeleton::RemapBones()
{
	for(int i = 0; i < bones.GetCount(); i++)
	{
		Rt3BoneInfo& info = bones[i];
		info.interpindex = -1;
		info.interpparent = -1;
	}
	numgpubones = 0;
	for(int i = 0; i < blendcombos.GetCount(); i++)
	{
		Rt3BlendCombo& c = blendcombos[i];
		for(int k = 0; k < 4; k++)
		{
			// A zero weight means this slot is unused; it reuses the previous
			// slot's index so the array can be read without branching.
			if(!c.weights[k])
			{
				c.interpbones[k] = k > 0 ? c.interpbones[k - 1] : 0;
				continue;
			}
			Rt3BoneInfo& info = bones[c.bones[k]];
			if(info.interpindex < 0) info.interpindex = numgpubones++;
			c.interpbones[k] = (byte)info.interpindex;
			if(info.group < 0) continue;
			for(int l = 0; l < 4; l++)
			{
				if(!c.weights[l]) break;
				if(l == k) continue;
				int parent = c.bones[l];
				// The other influence is this bone's parent (or its
				// grandparent, for a one-bone twist): the two cannot be
				// interpolated separately, so the constraint is recorded
				// against the parent.
				if(info.parent == parent || (info.parent >= 0 && info.parent == bones[parent].parent))
				{
					info.group = -info.parent;
					break;
				}
				if(info.group <= parent) continue;
				// Otherwise walk the other bone up to this bone's depth: if it
				// lands on this bone, they are the same one seen from
				// different depths and are constrained together.
				int child = c.bones[k];
				while(parent > child) parent = bones[parent].parent;
				if(parent != child) info.group = c.bones[l];
			}
		}
	}
	numinterpbones = numgpubones;
	for(int i = 0; i < tags.GetCount(); i++)
	{
		Rt3BoneInfo& info = bones[tags[i].bone];
		if(info.interpindex < 0) info.interpindex = numinterpbones++;
	}
	for(int i = 0; i < bones.GetCount(); i++)
	{
		Rt3BoneInfo& info = bones[i];
		if(info.interpindex < 0) continue;
		for(int parent = info.parent; parent >= 0 && bones[parent].interpindex < 0; parent = bones[parent].parent)
			bones[parent].interpindex = numinterpbones++;
	}
	for(int i = 0; i < bones.GetCount(); i++)
	{
		Rt3BoneInfo& info = bones[i];
		if(info.interpindex < 0) continue;
		info.interpparent = info.parent >= 0 ? bones[info.parent].interpindex : -1;
	}
	CalcAntipodes();
}

// skelmodel.h's `interpbones`, with upstream's INTERPBONE macro written out
// as a small lambda. The per-bone blend is
//
//   d = fr1 * ((1-t) * interp) + fr2 * (t * interp)
//       + pfr1 * ((1-prev.t) * (1-interp)) + pfr2 * (prev.t * (1-interp))
//
// i.e. the current keyframe pair at this part's weight, plus the previous
// pair at the weight the blend has left. `partmask[bone]` says which anim part
// drives the bone, so two parts can drive two halves of one skeleton, and it
// must therefore have one entry per **bone** (upstream's `animpartmask`) and
// every entry must be below `numanimparts` - both checked below, because a
// short mask reads whatever follows it and poses a bone with another part's
// animation.
void Rt3Skeleton::InterpBones(const Rt3AnimState* as, double pitch, const Rt3Vec3& axis,
                               const Rt3Vec3& forward, int numanimparts, const byte* partmask,
                               Vector<Rt3DualQuat>& bdata)
{
	bdata.Clear();
	if(numanimparts <= 0) return;
	bdata.SetCount(numinterpbones);

	struct Rt3FrameData
	{
		const Rt3DualQuat *fr1, *fr2, *pfr1, *pfr2;
	};
	for(int i = 0; i < bones.GetCount(); i++)
		ASSERT_(partmask[i] < numanimparts, "partmask entry " + Format("%d", (int)i) +
		        " names anim part " + Format("%d", (int)partmask[i]) + " of " +
		        Format("%d", numanimparts) + ", so the mask is not one entry per bone");
	Vector<Rt3FrameData> partframes(numanimparts);
	for(int i = 0; i < numanimparts; i++)
	{
		// Upstream's anim cache constructor seeds `fr1`/`fr2` with -1 so a
		// caller that never sampled an anim state trips over it; the U++
		// containers cannot do that, so the check is here instead. A frame
		// outside the keyframe array is always a bug in the caller, never a
		// value to clamp - a clamped frame would silently pose the wrong
		// bone.
		ASSERT_(as[i].cur.fr1 >= 0 && as[i].cur.fr2 >= 0 &&
			as[i].cur.fr1 < numframes && as[i].cur.fr2 < numframes &&
			(as[i].interp >= 1 || (as[i].prev.fr1 >= 0 && as[i].prev.fr2 >= 0 &&
				as[i].prev.fr1 < numframes && as[i].prev.fr2 < numframes)),
			"animation part " + Format("%d, frames %d/%d and %d/%d of %d", (int)i,
				(int)as[i].cur.fr1, (int)as[i].cur.fr2, (int)as[i].prev.fr1, (int)as[i].prev.fr2,
				(int)numframes) + " are out of range: the anim state was never sampled");
		partframes[i].fr1 = &framebones[as[i].cur.fr1 * numbones];
		partframes[i].fr2 = &framebones[as[i].cur.fr2 * numbones];
		if(as[i].interp < 1)
		{
			partframes[i].pfr1 = &framebones[as[i].prev.fr1 * numbones];
			partframes[i].pfr2 = &framebones[as[i].prev.fr2 * numbones];
		}
	}
	auto interpbone = [&](int bone) {
		const Rt3AnimState& s = as[partmask[bone]];
		const Rt3FrameData& f = partframes[partmask[bone]];
		Rt3DualQuat d = f.fr1[bone];
		d.Mul((1 - s.cur.t) * s.interp);
		d.Accumulate(f.fr2[bone], s.cur.t * s.interp);
		if(s.interp < 1)
		{
			d.Accumulate(f.pfr1[bone], (1 - s.prev.t) * (1 - s.interp));
			d.Accumulate(f.pfr2[bone], s.prev.t * (1 - s.interp));
		}
		return d;
	};

	// The pitch deps are re-blended first: the corrects measure the deviation
	// between the target's *blended* pose and its dep, so this has to happen
	// before `CalcPitchCorrects` runs.
	for(int i = 0; i < pitchdeps.GetCount(); i++)
	{
		Rt3PitchDep& p = pitchdeps[i];
		Rt3DualQuat d = interpbone(p.bone);
		d.Normalize();
		if(p.parent >= 0)
		{
			p.pose.Mul(pitchdeps[p.parent].pose, d);
		}
		else p.pose = d;
	}
	CalcPitchCorrects(pitch, axis, forward);

	for(int i = 0; i < bones.GetCount(); i++)
	{
		if(bones[i].interpindex < 0) continue;
		Rt3DualQuat d = interpbone(i);
		d.Normalize();
		const Rt3BoneInfo& b = bones[i];
		if(b.interpparent < 0) bdata[b.interpindex] = d;
		else bdata[b.interpindex].Mul(bdata[b.interpparent], d);

		// A bone is pitched either by its own scale/offset/limits, or by the
		// angle a pitch correct worked out for it. A bone with neither keeps
		// the pose it has.
		double angle;
		if(b.pitchscale)
		{
			angle = b.pitchscale * pitch + b.pitchoffset;
			if(b.pitchmin || b.pitchmax) angle = clamp(angle, b.pitchmin, b.pitchmax);
		}
		else if(b.correctindex >= 0) angle = pitchcorrects[b.correctindex].pitchangle;
		else continue;
		// RT3_ANIM_NOPITCH says "ignore the pitch", and while a blend is in
		// progress only the part that does not ignore it gets the full angle.
		if(as->cur.anim & RT3_ANIM_NOPITCH ||
		   (as->interp < 1 && as->prev.anim & RT3_ANIM_NOPITCH))
			angle *= (as->cur.anim & RT3_ANIM_NOPITCH ? 0 : as->interp) +
			         (as->interp < 1 && as->prev.anim & RT3_ANIM_NOPITCH ? 0 : 1 - as->interp);
		bdata[b.interpindex].MulOrient(Rt3Quat(axis, angle * M_PI / 180), b.base);
	}
	// An antipode must not be left holding the same rotation as its partner.
	for(int i = 0; i < antipodes.GetCount(); i++)
		bdata[antipodes[i].child].FixAntipodal(bdata[antipodes[i].parent]);
}

END_UPP_NAMESPACE

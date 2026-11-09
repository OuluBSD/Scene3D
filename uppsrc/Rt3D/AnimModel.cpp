#include "Rt3D.h"

NAMESPACE_UPP

// animmodel.h's `animpos::setframes`, the whole of this fork's clip sampling:
// walk forward in time, find the frame, and report the pair to interpolate
// between plus the progress between them.
//
// The order of the steps is upstream's and matters:
//  1. a range of one or less is a single frozen frame (fr1 = 0, t = 0),
//  2. otherwise the elapsed time is turned into a whole frame and a fraction.
//     `fr1` is truncated, not rounded, despite the comment - and `t` is what
//     is left over, so the two are consistent with each other,
//  3. then the frame number is turned into an index into the skeleton's
//     keyframes, wrapping when the animation loops and clamping when it
//     does not. Without a loop the last frame has no successor, so `fr2` is
//     clamped to it - which is what makes a non-looping animation settle
//     instead of interpolating back to its first frame,
//  4. and finally a reversed animation mirrors both frame numbers within its
//     range. The mirroring uses the already range-relative `fr1`, which is
//     why it has to come last.
void Rt3AnimPos::SetFrames(const Rt3AnimInfo& info, dword lastmillis)
{
	anim = info.anim;
	if(info.range <= 1)
	{
		fr1 = 0;
		t = 0;
	}
	else
	{
		dword time = (info.anim & RT3_ANIM_SETTIME) ? (dword)info.basetime : lastmillis - (dword)info.basetime;
		fr1 = (int)(time / info.speed);
		t = (time - fr1 * info.speed) / info.speed;
	}
	if(info.anim & RT3_ANIM_LOOP)
	{
		fr1 = fr1 % info.range + info.frame;
		fr2 = fr1 + 1;
		if(fr2 >= info.frame + info.range) fr2 = info.frame;
	}
	else
	{
		fr1 = min(fr1, info.range - 1) + info.frame;
		fr2 = min(fr1 + 1, info.frame + info.range - 1);
	}
	if(info.anim & RT3_ANIM_REVERSE)
	{
		fr1 = (info.frame + info.range - 1) - (fr1 - info.frame);
		fr2 = (info.frame + info.range - 1) - (fr2 - info.frame);
	}
}

END_UPP_NAMESPACE

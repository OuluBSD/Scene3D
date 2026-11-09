#ifndef _Rt3D_AnimModel_h_
#define _Rt3D_AnimModel_h_

// The animation sampling layer, ported from Tesseract-Sauerbraten's
// `src/engine/animmodel.h` (the `animspec`/`animpos`/`animstate` part) and
// `src/shared/ents.h` (`animinfo` and the `ANIM_*` flags).
//
// Upstream's `animmodel` is mostly GL render state; this is the part that
// turns "play animation N at time T" into a pair of keyframe indices and a
// blend factor, which is pure data and testable headless. The mesh/skin/
// shader half of `animmodel` is not ported (see Rt3D/AGENTS.md).
//
// This fork has no `clip`/`clipdata` at all: an animation is an
// (first frame, frame count) range into the skeleton's keyframe array plus a
// speed, and sampling it is `Rt3AnimPos::SetFrames`.

// shared/ents.h's animation flags. Only the ones the sampling and blending
// passes read are named here; the rest (`ANIM_NOSKIN`, `ANIM_FULLBRIGHT`,
// `ANIM_REUSE`, `ANIM_NORENDER`, `ANIM_RAGDOLL`, `ANIM_SETSPEED`,
// `ANIM_GHOST`) select render or game behavior that this port does not have,
// and `ANIM_FLAGS` is the mask of all of them.
enum
{
	RT3_ANIM_ALL     = 0x7F,
	RT3_ANIM_INDEX   = 0x7F,
	RT3_ANIM_LOOP    = 1 << 7,
	RT3_ANIM_START   = 1 << 8,
	RT3_ANIM_END     = 1 << 9,
	RT3_ANIM_REVERSE = 1 << 10,
	RT3_ANIM_CLAMP   = RT3_ANIM_START | RT3_ANIM_END,
	RT3_ANIM_DIR     = 0x780,
	RT3_ANIM_SECONDARY = 11,
	RT3_ANIM_SETTIME = 1 << 23,
	RT3_ANIM_NOPITCH = 1 << 29,
	RT3_ANIM_FLAGS   = 0x1FF << 22
};

// ents.h's `struct animinfo`: the request - which animation, from which frame
// for how many, at what speed, and the time it was started. `speed` is
// milliseconds per frame, so a speed of 100 is ten frames a second.
struct Rt3AnimInfo
{
	int anim = 0;
	int frame = 0;
	int range = 0;
	int basetime = 0;
	double speed = 100.0;
	dword varseed = 0;

	// Two requests are the same animation only if the frame range, the
	// direction/settime flags, the start time (unless the time is given
	// explicitly) and the speed all match.
	bool operator==(const Rt3AnimInfo& o) const
	{
		return frame == o.frame && range == o.range &&
		       (anim & (RT3_ANIM_SETTIME | RT3_ANIM_DIR)) == (o.anim & (RT3_ANIM_SETTIME | RT3_ANIM_DIR)) &&
		       (anim & RT3_ANIM_SETTIME || basetime == o.basetime) &&
		       speed == o.speed;
	}
	bool operator!=(const Rt3AnimInfo& o) const { return !operator==(o); }
};

// animmodel.h's `struct animspec`: an animation attached to a body part, with
// the priority used when several play at once.
struct Rt3AnimSpec
{
	int frame = 0;
	int range = 0;
	double speed = 0;
	int priority = 0;

	Rt3AnimSpec() = default;
	Rt3AnimSpec(int frame, int range, double speed, int priority = 0)
		: frame(frame), range(range), speed(speed), priority(priority) {}
};

// animmodel.h's `struct animpos`: the two keyframes an animation is between
// right now, and how far between them it is. `fr1 == fr2` means the animation
// has a single frame, and then `t` is 0 and does not matter.
struct Rt3AnimPos
{
	int anim = 0;
	int fr1 = -1, fr2 = -1;
	double t = 0;

	// animmodel.h's `setframes`. `lastmillis` is the current time and only
	// matters without RT3_ANIM_SETTIME, in which case the request's own
	// `basetime` is the origin.
	void SetFrames(const Rt3AnimInfo& info, dword lastmillis);

	bool operator==(const Rt3AnimPos& a) const
	{
		return fr1 == a.fr1 && fr2 == a.fr2 && (fr1 == fr2 || t == a.t);
	}
	bool operator!=(const Rt3AnimPos& a) const { return !operator==(a); }
};

// animmodel.h's `struct animstate`: the position an animation part is at now,
// plus the one it was at for the previous frame and how much weight that one
// still has. `interp` of 1 means no blending with the previous position.
struct Rt3AnimState
{
	Rt3AnimPos cur, prev;
	double interp = 1;

	Rt3AnimState() = default;
	Rt3AnimState(const Rt3AnimPos& cur, const Rt3AnimPos& prev, double interp)
		: cur(cur), prev(prev), interp(interp) {}

	bool operator==(const Rt3AnimState& a) const
	{
		return cur == a.cur && (interp < 1 ? interp == a.interp && prev == a.prev : a.interp >= 1);
	}
	bool operator!=(const Rt3AnimState& a) const { return !operator==(a); }
};

#endif

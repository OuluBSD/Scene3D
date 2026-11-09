#include "Core.h"

NAMESPACE_UPP

PointcloudLocalizationResult PointcloudLocalizerStub::Locate(const PointcloudReference& reference,
                                                            const PointcloudObservation& observation) {
	PointcloudLocalizationResult out;
	out.pose = PointcloudPose::MakeIdentity();
	out.ok = !reference.points.IsEmpty();
	out.note = "stub";
	if (observation.has_ground_truth) {
		out.pose = observation.ground_truth;
		out.ok = true;
		out.note = "stub: ground-truth";
	}
	return out;
}

ControllerPoseResult ControllerPatternTrackerStub::Track(const ControllerPattern& pattern,
                                                         const ControllerObservation& observation) {
	ControllerPoseResult out;
	out.id = pattern.id;
	out.pose = PointcloudPose::MakeIdentity();
	out.ok = !observation.points.IsEmpty();
	out.note = "stub";
	if (observation.has_ground_truth) {
		out.pose = observation.ground_truth;
		out.ok = true;
		out.note = "stub: ground-truth";
	}
	return out;
}

ControllerPoseResult ControllerFusionStub::Fuse(const ControllerPoseResult& vision,
                                                const ControllerFusionSample& sample) const {
	if (!vision.ok)
		return vision;
	ControllerPoseResult out = vision;
	if (sample.has_orientation) {
		out.pose.orientation = sample.orientation;
		out.note << (out.note.IsEmpty() ? "" : "; ") << "fused-orient";
	}
	if (sample.has_imu)
		out.note << (out.note.IsEmpty() ? "" : "; ") << "fused-imu";
	return out;
}

END_UPP_NAMESPACE

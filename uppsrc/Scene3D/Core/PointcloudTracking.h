#ifndef _Scene3D_Core_PointcloudTracking_h_
#define _Scene3D_Core_PointcloudTracking_h_

struct PointcloudPose : Moveable<PointcloudPose> {
	vec3 position = vec3(0, 0, 0);
	quat orientation = Upp::Identity<quat>();

	static PointcloudPose MakeIdentity() { return PointcloudPose(); }
};

struct PointcloudReference {
	Vector<vec3> points;
	Vector<int> ids;

	void Clear() {
		points.Clear();
		ids.Clear();
	}
};

struct PointcloudObservation : Moveable<PointcloudObservation> {
	Vector<vec3> points;
	Vector<int> ids;
	PointcloudPose ground_truth;
	bool has_ground_truth = false;

	void Clear() {
		points.Clear();
		ids.Clear();
		has_ground_truth = false;
		ground_truth = PointcloudPose::MakeIdentity();
	}
};

struct PointcloudLocalizationResult {
	bool ok = false;
	PointcloudPose pose;
	String note;
};

class PointcloudLocalizer {
public:
	virtual ~PointcloudLocalizer() = default;
	virtual PointcloudLocalizationResult Locate(const PointcloudReference& reference,
	                                            const PointcloudObservation& observation) = 0;
};

class PointcloudLocalizerStub : public PointcloudLocalizer {
public:
	PointcloudLocalizationResult Locate(const PointcloudReference& reference,
	                                    const PointcloudObservation& observation) override;
};

struct ControllerPattern : Moveable<ControllerPattern> {
	String id;
	Vector<vec3> dots_local;
};

struct ControllerObservation : Moveable<ControllerObservation> {
	Vector<vec3> points;
	Vector<int> ids;
	PointcloudPose ground_truth;
	bool has_ground_truth = false;

	void Clear() {
		points.Clear();
		ids.Clear();
		has_ground_truth = false;
		ground_truth = PointcloudPose::MakeIdentity();
	}
};

struct ControllerPoseResult {
	bool ok = false;
	String id;
	PointcloudPose pose;
	String note;
};

class ControllerPatternTracker {
public:
	virtual ~ControllerPatternTracker() = default;
	virtual ControllerPoseResult Track(const ControllerPattern& pattern,
	                                   const ControllerObservation& observation) = 0;
};

class ControllerPatternTrackerStub : public ControllerPatternTracker {
public:
	ControllerPoseResult Track(const ControllerPattern& pattern,
	                           const ControllerObservation& observation) override;
};

struct ControllerFusionSample {
	quat orientation = Upp::Identity<quat>();
	vec3 accel = vec3(0, 0, 0);
	vec3 gyro = vec3(0, 0, 0);
	bool has_orientation = false;
	bool has_imu = false;
};

class ControllerFusionStub {
public:
	ControllerPoseResult Fuse(const ControllerPoseResult& vision,
	                          const ControllerFusionSample& sample) const;
};

#endif

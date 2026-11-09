#ifndef _Scene3D_IO_PointcloudSim_h_
#define _Scene3D_IO_PointcloudSim_h_

struct SyntheticPointcloudConfig {
	int random_points = 2000;
	vec3 bounds_min = vec3(-2.0f, -1.0f, -2.0f);
	vec3 bounds_max = vec3(2.0f, 1.0f, 2.0f);
	int controller_dots_per_ring = 12;
	float controller_radius = 0.05f;
	float controller_height = 0.03f;
	float hmd_fov_deg = 90.0f;
	float hmd_min_dist = 0.2f;
	float max_range = 3.0f;
	int seed = 1337;
};

struct SyntheticPointcloudState {
	PointcloudReference reference;
	Vector<ControllerPattern> controllers;
	Vector<PointcloudPose> controller_poses_world;
	PointcloudPose hmd_pose_world;
};

SyntheticPointcloudState BuildSyntheticPointcloud(const SyntheticPointcloudConfig& cfg);
PointcloudObservation SimulateHmdObservation(const SyntheticPointcloudState& state,
                                             const SyntheticPointcloudConfig& cfg);
Vector<ControllerObservation> SimulateControllerObservations(const SyntheticPointcloudState& state,
                                                             const SyntheticPointcloudConfig& cfg);
bool RunSyntheticPointcloudSim(String& log, const SyntheticPointcloudConfig& cfg = SyntheticPointcloudConfig());

#endif

#include "Render.h"

NAMESPACE_UPP

bool ParseVec3Arg(const String& s, vec3& out) {
	Vector<String> parts = Split(s, ',');
	if (parts.GetCount() != 3)
		return false;
	for (int i = 0; i < 3; i++)
		out[i] = (float)StrDbl(parts[i]);
	return true;
}

AABB ComputeModelAABB(const Model& mdl) {
	vec3 minv(FLT_MAX);
	vec3 maxv(-FLT_MAX);
	for (const Mesh& mesh : mdl.meshes) {
		for (const Vertex& v : mesh.vertices) {
			vec3 p = v.position.Splice();
			minv[0] = min(minv[0], p[0]);
			minv[1] = min(minv[1], p[1]);
			minv[2] = min(minv[2], p[2]);
			maxv[0] = max(maxv[0], p[0]);
			maxv[1] = max(maxv[1], p[1]);
			maxv[2] = max(maxv[2], p[2]);
		}
	}
	if (minv[0] == FLT_MAX)
		return AABB(vec3(0), vec3(0));
	return FromMinMax(minv, maxv);
}

AABB TransformAABB(const AABB& aabb, const mat4& world) {
	vec3 mn = GetMin(aabb);
	vec3 mx = GetMax(aabb);
	vec3 corners[8] = {
		vec3(mn[0], mn[1], mn[2]),
		vec3(mx[0], mn[1], mn[2]),
		vec3(mn[0], mx[1], mn[2]),
		vec3(mx[0], mx[1], mn[2]),
		vec3(mn[0], mn[1], mx[2]),
		vec3(mx[0], mn[1], mx[2]),
		vec3(mn[0], mx[1], mx[2]),
		vec3(mx[0], mx[1], mx[2])
	};
	vec3 minv(FLT_MAX);
	vec3 maxv(-FLT_MAX);
	for (int i = 0; i < 8; i++) {
		vec3 p = (world * corners[i].Embed()).Splice();
		minv[0] = min(minv[0], p[0]);
		minv[1] = min(minv[1], p[1]);
		minv[2] = min(minv[2], p[2]);
		maxv[0] = max(maxv[0], p[0]);
		maxv[1] = max(maxv[1], p[1]);
		maxv[2] = max(maxv[2], p[2]);
	}
	return FromMinMax(minv, maxv);
}

void DumpFrustum(const Frustum& frustum, const GeomCamera& cam, Camera& cppcam, Stream& out) {
	out << "FrustumDump: cam_pos " << cam.position[0] << " " << cam.position[1] << " " << cam.position[2] << "\n";
	out << "FrustumDump: cam_orient " << cam.orientation[0] << " " << cam.orientation[1] << " "
	    << cam.orientation[2] << " " << cam.orientation[3] << "\n";
	out << "FrustumDump: fov " << cam.fov << " scale " << cam.scale << "\n";
	mat4 world = cppcam.GetWorldMatrix();
	mat4 proj = cppcam.GetProjectionMatrix();
	mat4 view = cppcam.GetViewMatrix();
	out << "FrustumDump: world\n";
	for (int r = 0; r < 4; r++) {
		for (int c = 0; c < 4; c++)
			out << world[r][c] << (c == 3 ? '\n' : ' ');
	}
	out << "FrustumDump: proj\n";
	for (int r = 0; r < 4; r++) {
		for (int c = 0; c < 4; c++)
			out << proj[r][c] << (c == 3 ? '\n' : ' ');
	}
	out << "FrustumDump: view\n";
	for (int r = 0; r < 4; r++) {
		for (int c = 0; c < 4; c++)
			out << view[r][c] << (c == 3 ? '\n' : ' ');
	}
	for (int i = 0; i < Frustum::PLANE_COUNT; i++) {
		const Plane& p = frustum[i];
		out << "FrustumPlane " << i << " n=" << p.normal[0] << "," << p.normal[1] << "," << p.normal[2]
		    << " d=" << p.distance << "\n";
	}
}

END_UPP_NAMESPACE

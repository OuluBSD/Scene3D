#ifndef _Scene3D_Render_HeadlessTools_h_
#define _Scene3D_Render_HeadlessTools_h_

bool ParseVec3Arg(const String& s, vec3& out);
AABB ComputeModelAABB(const Model& mdl);
AABB TransformAABB(const AABB& aabb, const mat4& world);
void DumpFrustum(const Frustum& frustum, const GeomCamera& cam, Camera& cppcam, Stream& out);

#endif

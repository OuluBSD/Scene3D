#include "Render.h"

NAMESPACE_UPP


static Color Blend2DLayerColor(Color base, Color tex, int mode) {
	auto clamp = [](int v) { return (byte)Clamp(v, 0, 255); };
	switch (mode) {
	case 1: { // Add
		return Color(clamp((int)base.GetR() + (int)tex.GetR()),
		             clamp((int)base.GetG() + (int)tex.GetG()),
		             clamp((int)base.GetB() + (int)tex.GetB()));
	}
	case 2: { // Multiply
		return Color(clamp((int)base.GetR() * (int)tex.GetR() / 255),
		             clamp((int)base.GetG() * (int)tex.GetG() / 255),
		             clamp((int)base.GetB() * (int)tex.GetB() / 255));
	}
	default: { // Normal (average)
		return Color(clamp(((int)base.GetR() + (int)tex.GetR()) / 2),
		             clamp(((int)base.GetG() + (int)tex.GetG()) / 2),
		             clamp(((int)base.GetB() + (int)tex.GetB()) / 2));
	}
	}
}

static String ResolveTexturePath(const String& ref) {
	if (ref.IsEmpty())
		return String();
	String path = ref;
	if (!FileExists(path)) {
		String base = GetCurrentDirectory();
		if (!base.IsEmpty()) {
			String alt = AppendFileName(base, ref);
			if (FileExists(alt))
				path = alt;
		}
		if (!FileExists(path)) {
			String share = RealizeShareFile(ref);
			if (FileExists(share))
				path = share;
		}
	}
	if (!FileExists(path))
		return String();
	return path;
}

static bool GetTextureAverageColor(const String& ref, Color& out) {
	String path = ResolveTexturePath(ref);
	if (path.IsEmpty())
		return false;
	if (!FileExists(path))
		return false;
	static VectorMap<String, Color> cache;
	int ci = cache.Find(path);
	if (ci >= 0) {
		out = cache[ci];
		return true;
	}
	Image img = StreamRaster::LoadFileAny(path);
	if (img.IsEmpty())
		return false;
	Size sz = img.GetSize();
	if (sz.cx <= 0 || sz.cy <= 0)
		return false;
	int step = max(1, min(sz.cx, sz.cy) / 64);
	long long r = 0, g = 0, b = 0, count = 0;
	for (int y = 0; y < sz.cy; y += step) {
		const RGBA* s = img[y];
		for (int x = 0; x < sz.cx; x += step) {
			r += s[x].r;
			g += s[x].g;
			b += s[x].b;
			count++;
		}
	}
	if (count <= 0)
		return false;
	out = Color((byte)Clamp(r / count, 0LL, 255LL),
	            (byte)Clamp(g / count, 0LL, 255LL),
	            (byte)Clamp(b / count, 0LL, 255LL));
	cache.Add(path, out);
	return true;
}

static bool GetTextureImage(const String& ref, Image& out) {
	String path = ResolveTexturePath(ref);
	if (path.IsEmpty())
		return false;
	static VectorMap<String, Image> cache;
	int ci = cache.Find(path);
	if (ci >= 0) {
		out = cache[ci];
		return !out.IsEmpty();
	}
	Image img = StreamRaster::LoadFileAny(path);
	if (img.IsEmpty())
		return false;
	cache.Add(path, img);
	out = img;
	return true;
}

static float WrapUV(float u, int mode) {
	switch (mode) {
	case 1: // Repeat
		u = u - floor(u);
		break;
	case 2: { // Mirror
		float t = u - floor(u);
		int k = (int)floor(u);
		if (k & 1)
			u = 1.0f - t;
		else
			u = t;
		break;
	}
	default:
		break;
	}
	return Clamp(u, 0.0f, 1.0f);
}

static Color SampleTextureColor(const Image& img, float u, float v, int wrap) {
	if (img.IsEmpty())
		return Color(255, 255, 255);
	u = WrapUV(u, wrap);
	v = WrapUV(v, wrap);
	Size sz = img.GetSize();
	int x = Clamp((int)floor(u * (sz.cx - 1)), 0, sz.cx - 1);
	int y = Clamp((int)floor(v * (sz.cy - 1)), 0, sz.cy - 1);
	RGBA px = img[y][x];
	return Color(px);
}

static vec2 ApplyTexUV(const Geom2DLayer& layer, float u, float v, float repeat_x, float repeat_y) {
	float sx = repeat_x;
	float sy = repeat_y;
	if (sx == 0)
		sx = 1.0f;
	if (sy == 0)
		sy = 1.0f;
	float ox = layer.tex_offset_x;
	float oy = layer.tex_offset_y;
	float rad = layer.tex_rotate * (float)M_PI / 180.0f;
	float cs = cos(rad);
	float sn = sin(rad);
	float x = u - 0.5f;
	float y = v - 0.5f;
	x *= sx;
	y *= sy;
	float rx = x * cs - y * sn;
	float ry = x * sn + y * cs;
	return vec2(rx + 0.5f + ox, ry + 0.5f + oy);
}

void DrawRect(Size sz, Draw& d, const mat4& view, const vec3& p, Size rect_sz, const Color& c, bool z_cull) {
	vec3 pp = VecMul(view, p);
	/*if (z_cull && pp[2] * SCALAR_FWD_Z > 0)
		return;*/
	float x = (pp[0] + 1) * 0.5 * sz.cx - rect_sz.cx / 2;
	float y = (-pp[1] + 1) * 0.5 * sz.cy - rect_sz.cy / 2;
	d.DrawRect(x, y, rect_sz.cx, rect_sz.cy, c);
}

void DrawLine(Size sz, Draw& d, const mat4& view, const vec3& a, const vec3& b, int line_width, const Color& c, bool z_cull) {
	vec3 ap = VecMul(view, a);
	vec3 bp = VecMul(view, b);
	/*if (z_cull && (ap[2] * SCALAR_FWD_Z > 0 || bp[2] * SCALAR_FWD_Z > 0))
		return;*/
	if ((ap[0] < -1 || ap[0] > +1) && (ap[1] < -1 || ap[1] > +1) &&
		(bp[0] < -1 || bp[0] > +1) && (bp[0] < -1 || bp[0] > +1))
		return;
	float x0 = (ap[0] + 1) * 0.5 * sz.cx;
	float x1 = (bp[0] + 1) * 0.5 * sz.cx;
	float y0 = (-ap[1] + 1) * 0.5 * sz.cy;
	float y1 = (-bp[1] + 1) * 0.5 * sz.cy;
	d.DrawLine(x0, y0, x1, y1, line_width, c);
}

void DrawLine(Size sz, Draw& d, const mat4& view, const Vertex& a, const Vertex& b, int line_width, const Color& c, bool z_cull) {
	DrawLine(sz, d, view, a.position.Splice(), b.position.Splice(), line_width, c, z_cull);
}

void DrawCameraGizmo(Size sz, Draw& d, const mat4& view, const vec3& pos, const quat& orient,
                     float fov_deg, float scale, const Color& clr, bool z_cull) {
	vec3 fwd = VectorTransform(VEC_FWD, orient);
	vec3 right = VectorTransform(VEC_RIGHT, orient);
	vec3 up = VectorTransform(VEC_UP, orient);
	float axis_len = scale * 0.25f;
	DrawLine(sz, d, view, pos, pos + right * axis_len, 1, LtRed(), z_cull);
	DrawLine(sz, d, view, pos, pos + up * axis_len, 1, LtGreen(), z_cull);
	DrawLine(sz, d, view, pos, pos + fwd * axis_len, 1, LtBlue(), z_cull);
	
	float near_d = scale * 0.2f;
	float far_d = scale * 0.7f;
	float half = (fov_deg * 0.5f) * (float)M_PI / 180.0f;
	float near_h = tan(half) * near_d;
	float far_h = tan(half) * far_d;
	vec3 near_c = pos + fwd * near_d;
	vec3 far_c = pos + fwd * far_d;
	vec3 n0 = near_c - right * near_h - up * near_h;
	vec3 n1 = near_c + right * near_h - up * near_h;
	vec3 n2 = near_c + right * near_h + up * near_h;
	vec3 n3 = near_c - right * near_h + up * near_h;
	vec3 f0 = far_c - right * far_h - up * far_h;
	vec3 f1 = far_c + right * far_h - up * far_h;
	vec3 f2 = far_c + right * far_h + up * far_h;
	vec3 f3 = far_c - right * far_h + up * far_h;
	
	DrawLine(sz, d, view, n0, n1, 1, clr, z_cull);
	DrawLine(sz, d, view, n1, n2, 1, clr, z_cull);
	DrawLine(sz, d, view, n2, n3, 1, clr, z_cull);
	DrawLine(sz, d, view, n3, n0, 1, clr, z_cull);
	
	DrawLine(sz, d, view, f0, f1, 1, clr, z_cull);
	DrawLine(sz, d, view, f1, f2, 1, clr, z_cull);
	DrawLine(sz, d, view, f2, f3, 1, clr, z_cull);
	DrawLine(sz, d, view, f3, f0, 1, clr, z_cull);
	
	DrawLine(sz, d, view, n0, f0, 1, clr, z_cull);
	DrawLine(sz, d, view, n1, f1, 1, clr, z_cull);
	DrawLine(sz, d, view, n2, f2, 1, clr, z_cull);
	DrawLine(sz, d, view, n3, f3, 1, clr, z_cull);
}

void DrawSelectionGizmo(Size sz, Draw& d, const mat4& view, const vec3& pos, bool z_cull, float scale) {
	float axis_len = max(0.15f, scale);
	DrawLine(sz, d, view, pos, pos + vec3(axis_len, 0, 0), 2, LtRed(), z_cull);
	DrawLine(sz, d, view, pos, pos + vec3(0, axis_len, 0), 2, LtGreen(), z_cull);
	DrawLine(sz, d, view, pos, pos + vec3(0, 0, axis_len), 2, LtBlue(), z_cull);
	DrawRect(sz, d, view, pos, Size(4, 4), Color(255, 220, 120), z_cull);
}

static void CountGizmoPixels(Scene3DRenderContext& ctx, Size sz, const mat4& view, const vec3& pos) {
	vec4 p4 = view * pos.Embed();
	if (p4[3] == 0)
		return;
	vec3 p = p4.Splice() / p4[3];
	vec2 ndc(p[0], p[1]);
	if (ndc[0] < -1 || ndc[0] > 1 || ndc[1] < -1 || ndc[1] > 1)
		return;
	int x = (int)floor((ndc[0] + 1) * 0.5 * (float)(sz.cx - 1) + 0.5f);
	int y = (int)floor((-ndc[1] + 1) * 0.5 * (float)(sz.cy - 1) + 0.5f);
	if (x < 0 || x >= sz.cx || y < 0 || y >= sz.cy)
		return;
	ctx.gizmo_pixels += 1;
}

static String GetUiTypeLower(const GeomObject& go) {
	if (!go.ui_type.IsEmpty())
		return ToLower(go.ui_type);
	if (const GeomDynamicProperties* props = go.FindDynamicProperties()) {
		int ti = props->props.Find("type");
		if (ti >= 0) {
			String t = AsString(props->props[ti]);
			if (!t.IsEmpty())
				return ToLower(t);
		}
	}
	return String();
}

static bool GetDynamicString(const GeomObject& go, const String& key, String& out) {
	const GeomDynamicProperties* props = go.FindDynamicProperties();
	if (!props)
		return false;
	int i = props->props.Find(key);
	if (i < 0)
		return false;
	out = AsString(props->props[i]);
	return !out.IsEmpty();
}

static bool GetDynamicFloat(const GeomObject& go, const String& key, float& out) {
	const GeomDynamicProperties* props = go.FindDynamicProperties();
	if (!props)
		return false;
	int i = props->props.Find(key);
	if (i < 0)
		return false;
	Value v = props->props[i];
	if (v.Is<double>() || v.Is<int>() || v.Is<int64>()) {
		out = (float)v;
		return true;
	}
	return false;
}

struct UiSceneLight : Moveable<UiSceneLight> {
	bool directional = true;
	vec3 position = vec3(0);
	vec3 direction = vec3(0, -1, 0);
	float intensity = 1.0f;
	float range = 8.0f;
};

static void CollectUiLights(const Scene3DRenderContext* ctx, Vector<UiSceneLight>& out) {
	out.Clear();
	if (!ctx || !ctx->state)
		return;
	for (const GeomObjectState& os : ctx->state->objs) {
		if (!os.obj || !os.obj->is_visible)
			continue;
		const GeomObject& go = *os.obj;
		String t = GetUiTypeLower(go);
		if (t == "light.directional") {
			UiSceneLight& l = out.Add();
			l.directional = true;
			l.direction = VectorTransform(VEC_FWD, os.orientation);
			if (l.direction.GetLength() == 0)
				l.direction = vec3(0, -1, 0);
			else
				l.direction.Normalize();
			GetDynamicFloat(go, "intensity", l.intensity);
			continue;
		}
		if (t == "light" || t == "light.point") {
			UiSceneLight& l = out.Add();
			l.directional = false;
			l.position = os.position;
			GetDynamicFloat(go, "intensity", l.intensity);
			GetDynamicFloat(go, "range", l.range);
			if (l.range <= 0)
				l.range = 8.0f;
			continue;
		}
	}
}

static void DrawUiTypeOverlay(Size sz, Draw& d, const mat4& view, const GeomObjectState& os, bool z_cull, Scene3DRenderContext* ctx) {
	const GeomObject& go = *os.obj;
	String t = GetUiTypeLower(go);
	if (t.IsEmpty())
		return;
	vec3 pos = os.position;
	auto draw_cross = [&](const vec3& p, float r, Color c, int w = 1) {
		DrawLine(sz, d, view, p + vec3(-r, 0, 0), p + vec3(+r, 0, 0), w, c, z_cull);
		DrawLine(sz, d, view, p + vec3(0, -r, 0), p + vec3(0, +r, 0), w, c, z_cull);
		DrawLine(sz, d, view, p + vec3(0, 0, -r), p + vec3(0, 0, +r), w, c, z_cull);
	};
	auto draw_box = [&](const vec3& p, float sx, float sy, float szl, Color c, int w = 1) {
		vec3 p000 = p + vec3(-sx, -sy, -szl);
		vec3 p001 = p + vec3(-sx, -sy, +szl);
		vec3 p010 = p + vec3(-sx, +sy, -szl);
		vec3 p011 = p + vec3(-sx, +sy, +szl);
		vec3 p100 = p + vec3(+sx, -sy, -szl);
		vec3 p101 = p + vec3(+sx, -sy, +szl);
		vec3 p110 = p + vec3(+sx, +sy, -szl);
		vec3 p111 = p + vec3(+sx, +sy, +szl);
		DrawLine(sz, d, view, p000, p001, w, c, z_cull);
		DrawLine(sz, d, view, p000, p010, w, c, z_cull);
		DrawLine(sz, d, view, p000, p100, w, c, z_cull);
		DrawLine(sz, d, view, p111, p110, w, c, z_cull);
		DrawLine(sz, d, view, p111, p101, w, c, z_cull);
		DrawLine(sz, d, view, p111, p011, w, c, z_cull);
		DrawLine(sz, d, view, p001, p011, w, c, z_cull);
		DrawLine(sz, d, view, p001, p101, w, c, z_cull);
		DrawLine(sz, d, view, p010, p011, w, c, z_cull);
		DrawLine(sz, d, view, p010, p110, w, c, z_cull);
		DrawLine(sz, d, view, p100, p101, w, c, z_cull);
		DrawLine(sz, d, view, p100, p110, w, c, z_cull);
	};
	if (t == "light" || t == "light.point") {
		Color c(255, 235, 130);
		draw_cross(pos, 0.2f, c, 2);
		DrawRect(sz, d, view, pos, Size(7, 7), c, z_cull);
		return;
	}
	if (t == "light.directional") {
		Color c(255, 220, 120);
		vec3 dir = VectorTransform(VEC_FWD, os.orientation);
		if (dir.GetLength() == 0)
			dir = vec3(0, -1, 0);
		else
			dir.Normalize();
		vec3 tip = pos + dir * 0.9f;
		DrawLine(sz, d, view, pos, tip, 2, c, z_cull);
		vec3 side = Cross(dir, VEC_UP);
		if (side.GetLength() == 0)
			side = vec3(1, 0, 0);
		else
			side.Normalize();
		DrawLine(sz, d, view, tip, tip - dir * 0.2f + side * 0.15f, 2, c, z_cull);
		DrawLine(sz, d, view, tip, tip - dir * 0.2f - side * 0.15f, 2, c, z_cull);
		return;
	}
	if (t == "skybox") {
		float e = max(1.5f, max(fabs(os.scale[0]), max(fabs(os.scale[1]), fabs(os.scale[2]))));
		draw_box(pos, e, e, e, Color(120, 170, 255), 1);
		String display, spec, irr;
		GetDynamicString(go, "display", display);
		GetDynamicString(go, "specular", spec);
		GetDynamicString(go, "irradiance", irr);
		String info = "Skybox";
		if (!display.IsEmpty() || !spec.IsEmpty() || !irr.IsEmpty())
			info << " [d/s/i]";
		d.DrawText(6, 6, info, StdFont(), Color(160, 200, 255));
		return;
	}
	if (t == "billboard" || t == "billboard_vertical") {
		GeomCamera* cam = nullptr;
		if (ctx && ctx->state)
			cam = &ctx->state->GetProgram();
		vec3 right(1, 0, 0);
		vec3 up(0, 1, 0);
		if (cam) {
			vec3 to_cam = cam->position - pos;
			if (to_cam.GetLength() > 0)
				to_cam.Normalize();
			if (t == "billboard_vertical") {
				up = vec3(0, 1, 0);
				right = Cross(up, to_cam);
			}
			else {
				right = VectorTransform(VEC_RIGHT, cam->orientation);
				up = VectorTransform(VEC_UP, cam->orientation);
			}
			if (right.GetLength() > 0)
				right.Normalize();
			if (up.GetLength() > 0)
				up.Normalize();
		}
		float hx = 0.45f, hy = 0.45f;
		vec3 p0 = pos - right * hx - up * hy;
		vec3 p1 = pos + right * hx - up * hy;
		vec3 p2 = pos + right * hx + up * hy;
		vec3 p3 = pos - right * hx + up * hy;
		Color c = t == "billboard_vertical" ? Color(120, 230, 160) : Color(120, 210, 255);
		DrawLine(sz, d, view, p0, p1, 1, c, z_cull);
		DrawLine(sz, d, view, p1, p2, 1, c, z_cull);
		DrawLine(sz, d, view, p2, p3, 1, c, z_cull);
		DrawLine(sz, d, view, p3, p0, 1, c, z_cull);
		DrawLine(sz, d, view, p0, p2, 1, c, z_cull);
		return;
	}
	if (t == "particle_system") {
		draw_cross(pos, 0.18f, Color(255, 180, 120), 1);
		int frame = (ctx && ctx->anim) ? ctx->anim->position : 0;
		for (int i = 0; i < 12; i++) {
			float a = ((float)i / 12.0f) * 2.0f * (float)M_PI + (float)frame * 0.12f;
			float r = 0.22f + 0.06f * (float)((i + frame) % 5);
			vec3 p = pos + vec3(cos(a) * r, 0.08f * (float)(i % 4), sin(a) * r);
			DrawRect(sz, d, view, p, Size(2, 2), Color(255, 160, 120), z_cull);
		}
		return;
	}
	if (t == "overlay2d" || t == "touch2d") {
		int slot = (int)(go.key % 6);
		int x = max(6, sz.cx - 188);
		int y = 16 + slot * 30;
		Color bg = (t == "touch2d") ? Color(90, 130, 90) : Color(90, 90, 130);
		d.DrawRect(x, y, 172, 22, bg);
		d.DrawRect(x + 1, y + 1, 170, 20, Color(35, 35, 40));
		d.DrawText(x + 6, y + 4, t == "touch2d" ? "Touch Overlay Hook" : "Overlay 2D Layer",
			StdFont(), LtGray());
		return;
	}
	if (t == "sound3d") {
		Color c(170, 210, 255);
		draw_cross(pos, 0.12f, c, 1);
		const int steps = 20;
		for (int ring = 1; ring <= 2; ring++) {
			float r = 0.18f * ring;
			for (int i = 1; i <= steps; i++) {
				float a0 = ((float)(i - 1) / (float)steps) * 2.0f * (float)M_PI;
				float a1 = ((float)i / (float)steps) * 2.0f * (float)M_PI;
				vec3 p0 = pos + vec3(cos(a0) * r, sin(a0) * r * 0.5f, 0);
				vec3 p1 = pos + vec3(cos(a1) * r, sin(a1) * r * 0.5f, 0);
				DrawLine(sz, d, view, p0, p1, 1, c, z_cull);
			}
		}
		return;
	}
	if (t == "path") {
		draw_cross(pos, 0.15f, Color(160, 255, 160), 1);
		if (ctx && ctx->state) {
			Vector<vec3> pts;
			for (const GeomObjectState& other : ctx->state->objs) {
				if (!other.obj || !other.obj->is_visible)
					continue;
				if (GetUiTypeLower(*other.obj) != "path_node")
					continue;
				String owner;
				if (!GetDynamicString(*other.obj, "path_owner", owner))
					continue;
				if (owner != go.name)
					continue;
				pts.Add(other.position);
			}
			for (int i = 1; i < pts.GetCount(); i++)
				DrawLine(sz, d, view, pts[i - 1], pts[i], 2, Color(120, 235, 120), z_cull);
			for (const vec3& p : pts)
				DrawRect(sz, d, view, p, Size(4, 4), Color(140, 255, 140), z_cull);
		}
		return;
	}
	if (t == "path_node") {
		DrawRect(sz, d, view, pos, Size(6, 6), Color(170, 255, 170), z_cull);
		DrawLine(sz, d, view, pos, pos + vec3(0, 0.2f, 0), 1, Color(170, 255, 170), z_cull);
		return;
	}
}

struct UiLightEval {
	float diffuse = 0.0f;
	float specular = 0.0f;
	float ambient = 0.25f;
};

static UiLightEval EvaluateUiLighting(const Vector<UiSceneLight>& lights, const vec3& world_pos,
                                      const vec3& normal_world, const vec3& view_dir, float shininess) {
	UiLightEval ev;
	if (lights.IsEmpty()) {
		vec3 dir(0.4f, 0.7f, 0.5f);
		dir.Normalize();
		float diff = max(0.0f, Dot(normal_world, dir));
		vec3 refl = normal_world * (2.0f * Dot(normal_world, dir)) - dir;
		if (refl.GetLength() > 0)
			refl.Normalize();
		float spec = 0.0f;
		if (diff > 0 && view_dir.GetLength() > 0)
			spec = pow(max(0.0f, Dot(refl, view_dir)), shininess);
		ev.diffuse = diff;
		ev.specular = spec;
		return ev;
	}
	float diff = 0;
	float spec = 0;
	float ambient = 0.15f;
	for (const UiSceneLight& l : lights) {
		vec3 to_light;
		float att = 1.0f;
		if (l.directional) {
			to_light = -l.direction;
		}
		else {
			to_light = l.position - world_pos;
			float dist = to_light.GetLength();
			if (dist > 0)
				to_light /= dist;
			float k = l.range > 0 ? dist / l.range : 0;
			att = Clamp(1.0f - k * k, 0.0f, 1.0f);
		}
		if (to_light.GetLength() == 0)
			continue;
		float d = max(0.0f, Dot(normal_world, to_light));
		vec3 refl = normal_world * (2.0f * Dot(normal_world, to_light)) - to_light;
		if (refl.GetLength() > 0)
			refl.Normalize();
		float s = 0.0f;
		if (d > 0 && view_dir.GetLength() > 0)
			s = pow(max(0.0f, Dot(refl, view_dir)), shininess);
		diff += d * l.intensity * att;
		spec += s * l.intensity * att;
		ambient += 0.05f * l.intensity * (l.directional ? 1.0f : att);
	}
	ev.diffuse = diff;
	ev.specular = spec;
	ev.ambient = ambient;
	return ev;
}

void DrawEditableMeshOverlay(Size sz, Draw& d, const mat4& view, const GeomObjectState& os,
                             const GeomEditableMesh& mesh, bool z_cull,
                             const Vector<float>* weights = nullptr, bool show_weights = false,
                             const Vector<int>* sel_points = nullptr,
                             const Vector<int>* sel_lines = nullptr,
                             const Vector<int>* sel_faces = nullptr) {
	Color pt_clr(220, 220, 255);
	Color line_clr(200, 200, 200);
	Color face_clr(160, 200, 240);
	Color sel_pt_clr(255, 220, 120);
	Color sel_line_clr(255, 180, 120);
	Color sel_face_clr(255, 200, 160);
	auto is_sel = [&](const Vector<int>* list, int id) -> bool {
		return list && FindIndex(*list, id) >= 0;
	};
	mat4 o_world = (QuatMat(os.orientation) * Translate(os.position) * Scale(os.scale)).GetInverse();
	mat4 o_view = view * o_world;
	for (const GeomFace& f : mesh.faces) {
		if (f.a < 0 || f.b < 0 || f.c < 0 || f.a >= mesh.points.GetCount() || f.b >= mesh.points.GetCount() || f.c >= mesh.points.GetCount())
			continue;
		const vec3& p0 = mesh.points[f.a];
		const vec3& p1 = mesh.points[f.b];
		const vec3& p2 = mesh.points[f.c];
		Color clr = is_sel(sel_faces, &f - mesh.faces.Begin()) ? sel_face_clr : face_clr;
		int lw = is_sel(sel_faces, &f - mesh.faces.Begin()) ? 2 : 1;
		DrawLine(sz, d, o_view, p0, p1, lw, clr, z_cull);
		DrawLine(sz, d, o_view, p1, p2, lw, clr, z_cull);
		DrawLine(sz, d, o_view, p2, p0, lw, clr, z_cull);
	}
	for (const GeomEdge& e : mesh.lines) {
		if (e.a < 0 || e.b < 0 || e.a >= mesh.points.GetCount() || e.b >= mesh.points.GetCount())
			continue;
		Color clr = is_sel(sel_lines, &e - mesh.lines.Begin()) ? sel_line_clr : line_clr;
		int lw = is_sel(sel_lines, &e - mesh.lines.Begin()) ? 2 : 1;
		DrawLine(sz, d, o_view, mesh.points[e.a], mesh.points[e.b], lw, clr, z_cull);
	}
	for (int i = 0; i < mesh.points.GetCount(); i++) {
		Color clr = pt_clr;
		int szp = 3;
		if (show_weights && weights && i < weights->GetCount()) {
			float w = (*weights)[i];
			w = Clamp(w, 0.0f, 1.0f);
			clr = Blend(Color(60, 80, 180), Color(220, 80, 80), w);
		}
		if (is_sel(sel_points, i)) {
			clr = sel_pt_clr;
			szp = 5;
		}
		DrawRect(sz, d, o_view, mesh.points[i], Size(szp, szp), clr, z_cull);
	}
}

void Draw2DLayerOverlay(Size sz, Draw& d, const mat4& view, const GeomObjectState& os,
                        const Geom2DLayer& layer, bool z_cull,
                        const Vector<int>* sel_shapes = nullptr) {
	if (!layer.visible)
		return;
	Image tex_img;
	bool has_tex = layer.use_layer_style && GetTextureImage(layer.texture_ref, tex_img);
	auto apply_opacity = [&](Color c) {
		RGBA r = c;
		float a = Clamp(layer.opacity, 0.0f, 1.0f);
		r.r = (byte)Clamp(r.r * a, 0.0f, 255.0f);
		r.g = (byte)Clamp(r.g * a, 0.0f, 255.0f);
		r.b = (byte)Clamp(r.b * a, 0.0f, 255.0f);
		return Color(r);
	};
	auto local_to_world = [&](const vec2& p) {
		vec3 local(p[0] * os.scale[0], p[1] * os.scale[1], 0);
		return os.position + VectorTransform(local, os.orientation);
	};
	auto draw_round = [&](const vec2& center, float radius, const Color& clr) {
		const int steps = 12;
		Vector<vec2> pts;
		pts.SetCount(steps);
		for (int i = 0; i < steps; i++) {
			float a = (float)i / (float)steps * 2.0f * (float)M_PI;
			pts[i] = center + vec2(cos(a), sin(a)) * radius;
		}
		for (int i = 1; i < pts.GetCount(); i++) {
			DrawLine(sz, d, view, local_to_world(pts[i - 1]), local_to_world(pts[i]), 1, clr, z_cull);
		}
		if (pts.GetCount() >= 2)
			DrawLine(sz, d, view, local_to_world(pts.Top()), local_to_world(pts[0]), 1, clr, z_cull);
	};
	auto draw_square_cap = [&](const vec2& center, const vec2& dir, float width, const Color& clr) {
		vec2 n = dir;
		float len = sqrt(Dot(n, n));
		if (len <= 1e-6f)
			return;
		n /= len;
		vec2 perp(-n[1], n[0]);
		float h = width * 0.5f;
		vec2 p0 = center + perp * h + n * h;
		vec2 p1 = center - perp * h + n * h;
		vec2 p2 = center - perp * h - n * h;
		vec2 p3 = center + perp * h - n * h;
		Vector<vec2> pts;
		pts << p0 << p1 << p2 << p3;
		for (int i = 1; i < pts.GetCount(); i++)
			DrawLine(sz, d, view, local_to_world(pts[i - 1]), local_to_world(pts[i]), 1, clr, z_cull);
		DrawLine(sz, d, view, local_to_world(pts.Top()), local_to_world(pts[0]), 1, clr, z_cull);
	};
	auto draw_poly = [&](const Vector<vec2>& pts, const Color& clr, float width, bool closed, int cap, int join) {
		if (pts.GetCount() < 2)
			return;
		for (int i = 1; i < pts.GetCount(); i++) {
			DrawLine(sz, d, view, local_to_world(pts[i - 1]), local_to_world(pts[i]), (int)width, clr, z_cull);
		}
		if (closed)
			DrawLine(sz, d, view, local_to_world(pts.Top()), local_to_world(pts[0]), (int)width, clr, z_cull);
		if (!closed && cap != 0) {
			vec2 d0 = pts[1] - pts[0];
			vec2 d1 = pts.Top() - pts[pts.GetCount() - 2];
			float r = max(1.0f, width * 0.5f);
			if (cap == 1) {
				draw_round(pts[0], r, clr);
				draw_round(pts.Top(), r, clr);
			}
			else if (cap == 2) {
				draw_square_cap(pts[0], d0, width, clr);
				draw_square_cap(pts.Top(), d1, width, clr);
			}
		}
		if (closed && join == 1) {
			float r = max(1.0f, width * 0.5f);
			for (const vec2& p : pts)
				draw_round(p, r, clr);
		}
	};
	auto get_bbox = [&](const Vector<vec2>& pts, vec2& bmin, vec2& bmax) {
		if (pts.IsEmpty()) {
			bmin = vec2(0, 0);
			bmax = vec2(1, 1);
			return;
		}
		bmin = pts[0];
		bmax = pts[0];
		for (int i = 1; i < pts.GetCount(); i++) {
			bmin[0] = min(bmin[0], pts[i][0]);
			bmin[1] = min(bmin[1], pts[i][1]);
			bmax[0] = max(bmax[0], pts[i][0]);
			bmax[1] = max(bmax[1], pts[i][1]);
		}
	};
	auto uv_from_point = [&](const vec2& p, const vec2& bmin, const vec2& bmax) {
		float w = bmax[0] - bmin[0];
		float h = bmax[1] - bmin[1];
		float u = (w != 0) ? (p[0] - bmin[0]) / w : 0.5f;
		float v = (h != 0) ? (p[1] - bmin[1]) / h : 0.5f;
		return vec2(u, v);
	};
	auto blend_tex = [&](Color base, float u, float v, int wrap, float repeat_x, float repeat_y) {
		if (!has_tex)
			return base;
		vec2 uv = ApplyTexUV(layer, u, v, repeat_x, repeat_y);
		Color tex = SampleTextureColor(tex_img, uv[0], uv[1], wrap);
		return Blend2DLayerColor(base, tex, layer.blend_mode);
	};
	for (int si = 0; si < layer.shapes.GetCount(); si++) {
		const Geom2DShape& shape = layer.shapes[si];
		bool style = layer.use_layer_style;
		Color stroke_base = style ? layer.stroke : shape.stroke;
		float w = layer.use_layer_style ? layer.width : shape.width;
		if (w <= 0)
			w = 1.0f;
		int cap = (shape.stroke_cap >= 0) ? shape.stroke_cap : layer.stroke_cap;
		int join = (shape.stroke_join >= 0) ? shape.stroke_join : layer.stroke_join;
		Color fill_base = layer.fill;
		int wrap = (shape.tex_wrap >= 0) ? shape.tex_wrap : layer.tex_wrap;
		int stroke_uv_mode = (shape.stroke_uv_mode >= 0) ? shape.stroke_uv_mode : layer.stroke_uv_mode;
		float repeat_x = (shape.tex_repeat_x > 0) ? shape.tex_repeat_x : layer.tex_repeat_x;
		float repeat_y = (shape.tex_repeat_y > 0) ? shape.tex_repeat_y : layer.tex_repeat_y;
		Color fill = apply_opacity(fill_base);
		Color stroke = apply_opacity(stroke_base);
		vec2 bmin, bmax;
		if (shape.type == Geom2DShape::S_LINE || shape.type == Geom2DShape::S_POLY) {
			get_bbox(shape.points, bmin, bmax);
		}
		float total_len = 0;
		Vector<float> seg_len;
		if ((shape.type == Geom2DShape::S_LINE || shape.type == Geom2DShape::S_POLY) && shape.points.GetCount() >= 2) {
			seg_len.SetCount(shape.points.GetCount() - 1);
			for (int i = 1; i < shape.points.GetCount(); i++) {
				vec2 d2 = shape.points[i] - shape.points[i - 1];
				float l = sqrt(Dot(d2, d2));
				seg_len[i - 1] = l;
				total_len += l;
			}
			if (shape.closed && shape.points.GetCount() >= 2) {
				vec2 d2 = shape.points[0] - shape.points.Top();
				total_len += sqrt(Dot(d2, d2));
			}
		}
		switch (shape.type) {
		case Geom2DShape::S_LINE:
			if (shape.points.GetCount() >= 2) {
				if (style && has_tex) {
					if (stroke_uv_mode == 1) {
						stroke = apply_opacity(blend_tex(stroke_base, 0.5f, 0.5f, wrap, repeat_x, repeat_y));
					}
					else {
						vec2 mid = (shape.points[0] + shape.points[1]) * 0.5f;
						vec2 uv = uv_from_point(mid, bmin, bmax);
						stroke = apply_opacity(blend_tex(stroke_base, uv[0], uv[1], wrap, repeat_x, repeat_y));
					}
				}
				Vector<vec2> line_pts;
				line_pts << shape.points[0] << shape.points[1];
				draw_poly(line_pts, stroke, w, false, cap, join);
			}
			break;
		case Geom2DShape::S_RECT:
			if (shape.points.GetCount() >= 2) {
				vec2 a = shape.points[0];
				vec2 b = shape.points[1];
				vec2 p0(min(a[0], b[0]), min(a[1], b[1]));
				vec2 p1(max(a[0], b[0]), min(a[1], b[1]));
				vec2 p2(max(a[0], b[0]), max(a[1], b[1]));
				vec2 p3(min(a[0], b[0]), max(a[1], b[1]));
				if (layer.use_layer_style) {
					const int steps = 20;
					for (int i = 0; i <= steps; i++) {
						float t = (float)i / (float)steps;
						vec2 s0 = Lerp(p0, p3, t);
						vec2 s1 = Lerp(p1, p2, t);
						if (has_tex) {
							const int segs = 48;
							for (int j = 0; j < segs; j++) {
								float u0 = (float)j / (float)segs;
								float u1 = (float)(j + 1) / (float)segs;
								float um = (u0 + u1) * 0.5f;
								Color col = apply_opacity(blend_tex(fill_base, um, t, wrap, repeat_x, repeat_y));
								vec2 l0 = Lerp(s0, s1, u0);
								vec2 l1 = Lerp(s0, s1, u1);
								DrawLine(sz, d, view, local_to_world(l0), local_to_world(l1), 1, col, z_cull);
							}
						}
						else {
							DrawLine(sz, d, view, local_to_world(s0), local_to_world(s1), 1, fill, z_cull);
						}
					}
				}
				Vector<vec2> rect_pts;
				rect_pts << p0 << p1 << p2 << p3;
				get_bbox(rect_pts, bmin, bmax);
				auto draw_seg = [&](const vec2& a0, const vec2& a1, float t0, float t1) {
					if (style && has_tex) {
						if (stroke_uv_mode == 1) {
							float um = (t0 + t1) * 0.5f;
							stroke = apply_opacity(blend_tex(stroke_base, um, 0.5f, wrap, repeat_x, repeat_y));
						}
						else {
							vec2 mid = (a0 + a1) * 0.5f;
							vec2 uv = uv_from_point(mid, bmin, bmax);
							stroke = apply_opacity(blend_tex(stroke_base, uv[0], uv[1], wrap, repeat_x, repeat_y));
						}
					}
					DrawLine(sz, d, view, local_to_world(a0), local_to_world(a1), (int)w, stroke, z_cull);
				};
				float perim = 2.0f * ((p1[0] - p0[0]) + (p3[1] - p0[1]));
				float t = 0;
				float e0 = (p1[0] - p0[0]) / perim;
				float e1 = (p2[1] - p1[1]) / perim;
				float e2 = (p2[0] - p3[0]) / perim;
				float e3 = (p3[1] - p0[1]) / perim;
				draw_seg(p0, p1, t, t + e0); t += e0;
				draw_seg(p1, p2, t, t + e1); t += e1;
				draw_seg(p2, p3, t, t + e2); t += e2;
				draw_seg(p3, p0, t, t + e3);
			}
			break;
		case Geom2DShape::S_CIRCLE:
			if (shape.points.GetCount() >= 1) {
				vec2 c = shape.points[0];
				float r = shape.radius;
				if (r <= 0 && shape.points.GetCount() >= 2) {
					vec2 d2 = shape.points[1] - shape.points[0];
					r = sqrt(Dot(d2, d2));
				}
				if (r > 0) {
					if (layer.use_layer_style) {
						const int steps = 20;
						for (int i = 0; i <= steps; i++) {
							float y = -r + 2.0f * r * ((float)i / (float)steps);
							float x = sqrt(max(0.0f, r * r - y * y));
							if (has_tex) {
								const int segs = 48;
								for (int j = 0; j < segs; j++) {
									float u0 = (float)j / (float)segs;
									float u1 = (float)(j + 1) / (float)segs;
									float xm0 = -x + (2.0f * x) * u0;
									float xm1 = -x + (2.0f * x) * u1;
									float xm = (xm0 + xm1) * 0.5f;
									float u = (xm + r) / (2.0f * r);
									float v = (y + r) / (2.0f * r);
									Color col = apply_opacity(blend_tex(fill_base, u, v, wrap, repeat_x, repeat_y));
									DrawLine(sz, d, view, local_to_world(c + vec2(xm0, y)),
									         local_to_world(c + vec2(xm1, y)), 1, col, z_cull);
								}
							}
							else {
								DrawLine(sz, d, view, local_to_world(c + vec2(-x, y)), local_to_world(c + vec2(x, y)), 1, fill, z_cull);
							}
						}
					}
					const int steps = 32;
					Vector<vec2> pts;
					pts.SetCount(steps);
					for (int i = 0; i < steps; i++) {
						float a = (float)i / (float)steps * 2.0f * (float)M_PI;
						pts[i] = c + vec2(cos(a), sin(a)) * r;
					}
					if (style && has_tex) {
						if (stroke_uv_mode == 1) {
							int cnt = pts.GetCount();
							for (int i = 0; i < cnt; i++) {
								int j = (i + 1) % cnt;
								float u0 = (float)i / (float)cnt;
								float u1 = (float)j / (float)cnt;
								float um = (u0 + u1) * 0.5f;
								stroke = apply_opacity(blend_tex(stroke_base, um, 0.5f, wrap, repeat_x, repeat_y));
								DrawLine(sz, d, view, local_to_world(pts[i]), local_to_world(pts[j]), (int)w, stroke, z_cull);
							}
						}
						else {
							for (int i = 0; i < pts.GetCount(); i++) {
								int j = (i + 1) % pts.GetCount();
								vec2 mid = (pts[i] + pts[j]) * 0.5f;
								float ang = atan2(mid[1] - c[1], mid[0] - c[0]);
								float u = (ang + (float)M_PI) / (2.0f * (float)M_PI);
								float v = 0.5f;
								stroke = apply_opacity(blend_tex(stroke_base, u, v, wrap, repeat_x, repeat_y));
								DrawLine(sz, d, view, local_to_world(pts[i]), local_to_world(pts[j]), (int)w, stroke, z_cull);
							}
						}
					}
					else {
						draw_poly(pts, stroke, w, true, cap, join);
					}
				}
			}
			break;
		case Geom2DShape::S_POLY:
			if (style && has_tex && shape.points.GetCount() >= 2) {
				float t = 0;
				for (int i = 1; i < shape.points.GetCount(); i++) {
					if (stroke_uv_mode == 1 && total_len > 0) {
						float len = seg_len[i - 1];
						float um = (t + (t + len)) * 0.5f / total_len;
						stroke = apply_opacity(blend_tex(stroke_base, um, 0.5f, wrap, repeat_x, repeat_y));
						t += len;
					}
					else {
						vec2 mid = (shape.points[i - 1] + shape.points[i]) * 0.5f;
						vec2 uv = uv_from_point(mid, bmin, bmax);
						stroke = apply_opacity(blend_tex(stroke_base, uv[0], uv[1], wrap, repeat_x, repeat_y));
					}
					DrawLine(sz, d, view, local_to_world(shape.points[i - 1]), local_to_world(shape.points[i]), (int)w, stroke, z_cull);
				}
				if (shape.closed) {
					if (stroke_uv_mode == 1 && total_len > 0) {
						vec2 d2 = shape.points[0] - shape.points.Top();
						float len = sqrt(Dot(d2, d2));
						float um = (t + (t + len)) * 0.5f / total_len;
						stroke = apply_opacity(blend_tex(stroke_base, um, 0.5f, wrap, repeat_x, repeat_y));
					}
					else {
						vec2 mid = (shape.points.Top() + shape.points[0]) * 0.5f;
						vec2 uv = uv_from_point(mid, bmin, bmax);
						stroke = apply_opacity(blend_tex(stroke_base, uv[0], uv[1], wrap, repeat_x, repeat_y));
					}
					DrawLine(sz, d, view, local_to_world(shape.points.Top()), local_to_world(shape.points[0]), (int)w, stroke, z_cull);
				}
			}
			else {
				draw_poly(shape.points, stroke, w, shape.closed, cap, join);
			}
			break;
		default:
			break;
		}
		if (sel_shapes && FindIndex(*sel_shapes, si) >= 0) {
			Color hilite(255, 220, 120);
			float hw = w + 2;
			if (shape.type == Geom2DShape::S_LINE && shape.points.GetCount() >= 2) {
				Vector<vec2> line_pts;
				line_pts << shape.points[0] << shape.points[1];
				draw_poly(line_pts, hilite, hw, false, cap, join);
			}
			else if (shape.type == Geom2DShape::S_RECT && shape.points.GetCount() >= 2) {
				vec2 a = shape.points[0];
				vec2 b = shape.points[1];
				vec2 p0(min(a[0], b[0]), min(a[1], b[1]));
				vec2 p1(max(a[0], b[0]), min(a[1], b[1]));
				vec2 p2(max(a[0], b[0]), max(a[1], b[1]));
				vec2 p3(min(a[0], b[0]), max(a[1], b[1]));
				Vector<vec2> rect_pts;
				rect_pts << p0 << p1 << p2 << p3;
				draw_poly(rect_pts, hilite, hw, true, cap, join);
			}
			else if (shape.type == Geom2DShape::S_CIRCLE && shape.points.GetCount() >= 1) {
				vec2 c = shape.points[0];
				float r = shape.radius;
				if (r <= 0 && shape.points.GetCount() >= 2) {
					vec2 d2 = shape.points[1] - shape.points[0];
					r = sqrt(Dot(d2, d2));
				}
				if (r > 0) {
					const int steps = 32;
					Vector<vec2> pts;
					pts.SetCount(steps);
					for (int i = 0; i < steps; i++) {
						float a = (float)i / (float)steps * 2.0f * (float)M_PI;
						pts[i] = c + vec2(cos(a), sin(a)) * r;
					}
					draw_poly(pts, hilite, hw, true, cap, join);
				}
			}
			else if (shape.type == Geom2DShape::S_POLY && shape.points.GetCount() >= 2) {
				draw_poly(shape.points, hilite, hw, shape.closed, cap, join);
			}
		}
	}
}

void DrawSkeletonRecursive(Size sz, Draw& d, const mat4& view, const vec3& parent_pos,
                           const quat& parent_ori, const vec3& parent_scale, VfsValue& bone_node,
                           VfsValue* selected, bool z_cull) {
	if (!IsVfsType(bone_node, AsTypeHash<GeomBone>()))
		return;
	GeomBone& bone = bone_node.GetExt<GeomBone>();
	vec3 local = bone.position;
	vec3 scaled(local[0] * parent_scale[0], local[1] * parent_scale[1], local[2] * parent_scale[2]);
	vec3 pos = parent_pos + VectorTransform(scaled, parent_ori);
	quat ori = parent_ori * bone.orientation;
	float avg_scale = (parent_scale[0] + parent_scale[1] + parent_scale[2]) / 3.0f;
	Color clr = (&bone_node == selected) ? Color(255, 220, 120) : Color(200, 200, 255);
	DrawLine(sz, d, view, parent_pos, pos, 1, clr, z_cull);
	DrawRect(sz, d, view, pos, Size(3, 3), clr, z_cull);
	vec3 tip = pos + VectorTransform(VEC_FWD, ori) * (bone.length * avg_scale);
	DrawLine(sz, d, view, pos, tip, 1, clr, z_cull);
	for (auto& sub : bone_node.sub) {
		if (IsVfsType(sub, AsTypeHash<GeomBone>()))
			DrawSkeletonRecursive(sz, d, view, pos, ori, parent_scale, sub, selected, z_cull);
	}
}

Color CameraColor(const GeomObject& go) {
	String name = ToLower(go.name);
	if (name.Find("fake") >= 0)
		return Color(120, 220, 120);
	if (name.Find("localized") >= 0)
		return Color(220, 120, 120);
	if (name.Find("hmd") >= 0)
		return Color(120, 180, 240);
	return Color(220, 220, 120);
}

bool ClipLineNdc(vec2& a, vec2& b) {
	vec2 d2 = b - a;
	float p[4] = {-d2[0], d2[0], -d2[1], d2[1]};
	float q[4] = {a[0] + 1.0f, 1.0f - a[0], a[1] + 1.0f, 1.0f - a[1]};
	float u1 = 0.0f;
	float u2 = 1.0f;
	for (int i = 0; i < 4; i++) {
		if (p[i] == 0.0f) {
			if (q[i] < 0.0f)
				return false;
		}
		else {
			float t = q[i] / p[i];
			if (p[i] < 0.0f)
				u1 = max(u1, t);
			else
				u2 = min(u2, t);
			if (u1 > u2)
				return false;
		}
	}
	vec2 a0 = a;
	a = a0 + d2 * u1;
	b = a0 + d2 * u2;
	return true;
}

bool ClipLineClipSpace(vec4& a, vec4& b, float* out_u1 = nullptr, float* out_u2 = nullptr) {
	vec4 d = b - a;
	float u1 = 0.0f;
	float u2 = 1.0f;
	auto clip = [&](float fa, float fb) -> bool {
		float da = fa;
		float db = fb;
		float p = db - da;
		if (p == 0.0f) {
			return da >= 0.0f;
		}
		float t = da / (da - db);
		if (p < 0.0f)
			u2 = min(u2, t);
		else
			u1 = max(u1, t);
		return u1 <= u2;
	};
	// planes: x + w >= 0, -x + w >= 0, y + w >= 0, -y + w >= 0
	if (!clip(a[0] + a[3], b[0] + b[3])) return false;
	if (!clip(-a[0] + a[3], -b[0] + b[3])) return false;
	if (!clip(a[1] + a[3], b[1] + b[3])) return false;
	if (!clip(-a[1] + a[3], -b[1] + b[3])) return false;
	// ZO clip space: 0 <= z <= w
	if (!clip(a[2], b[2])) return false;
	if (!clip(-a[2] + a[3], -b[2] + b[3])) return false;
	if (out_u1)
		*out_u1 = u1;
	if (out_u2)
		*out_u2 = u2;
	vec4 a0 = a;
	a = a0 + d * u1;
	b = a0 + d * u2;
	return true;
}

void DrawGroundGrid(Size sz, Draw& d, const mat4& proj, const mat4& cam_world, const vec3& cam_pos,
                    const Scene3DRenderConfig& conf, bool z_cull) {
	if (!conf.show_grid)
		return;
	float major = conf.grid_major_step;
	if (major <= 0.0001f)
		return;
	int divs = conf.grid_minor_divs;
	if (divs < 1)
		divs = 1;
	float minor = major / (float)divs;
	float extent = conf.grid_extent;
	if (extent < major)
		extent = major;
	float start_x = floor((cam_pos[0] - extent) / minor) * minor;
	float end_x = ceil((cam_pos[0] + extent) / minor) * minor;
	float start_z = floor((cam_pos[2] - extent) / minor) * minor;
	float end_z = ceil((cam_pos[2] + extent) / minor) * minor;
	auto draw_grid_line = [&](const vec3& a, const vec3& b, const Color& clr) {
		vec3 ap_cam = VecMul(cam_world, a);
		vec3 bp_cam = VecMul(cam_world, b);
		vec3 a_world = a;
		vec3 b_world = b;
		if (z_cull) {
			float za = ap_cam[2] * SCALAR_FWD_Z;
			float zb = bp_cam[2] * SCALAR_FWD_Z;
			if (za < 0 && zb < 0)
				return;
			if ((za < 0 && zb >= 0) || (zb < 0 && za >= 0)) {
				float t = za / (za - zb);
				t = Clamp(t, 0.0f, 1.0f);
				vec3 cut = a_world + (b_world - a_world) * t;
				if (za < 0)
					a_world = cut;
				else
					b_world = cut;
			}
		}
		vec4 ap4 = proj * (cam_world * a_world.Embed());
		vec4 bp4 = proj * (cam_world * b_world.Embed());
		if (!ClipLineClipSpace(ap4, bp4))
			return;
		if (ap4[3] == 0 || bp4[3] == 0)
			return;
		vec3 ap = ap4.Splice() / ap4[3];
		vec3 bp = bp4.Splice() / bp4[3];
		vec2 a2(ap[0], ap[1]);
		vec2 b2(bp[0], bp[1]);
		if (!ClipLineNdc(a2, b2))
			return;
		float x0 = (a2[0] + 1) * 0.5 * sz.cx;
		float x1 = (b2[0] + 1) * 0.5 * sz.cx;
		float y0 = (-a2[1] + 1) * 0.5 * sz.cy;
		float y1 = (-b2[1] + 1) * 0.5 * sz.cy;
		d.DrawLine(x0, y0, x1, y1, 1, clr);
	};
	for (float x = start_x; x <= end_x + minor * 0.5f; x += minor) {
		int idx = (int)floor(x / minor + 0.5f);
		int mod = idx % divs;
		if (mod < 0)
			mod += divs;
		bool is_major = (mod == 0);
		Color clr = is_major ? conf.grid_major_clr : conf.grid_minor_clr;
		draw_grid_line(vec3(x, 0, start_z), vec3(x, 0, end_z), clr);
	}
	for (float z = start_z; z <= end_z + minor * 0.5f; z += minor) {
		int idx = (int)floor(z / minor + 0.5f);
		int mod = idx % divs;
		if (mod < 0)
			mod += divs;
		bool is_major = (mod == 0);
		Color clr = is_major ? conf.grid_major_clr : conf.grid_minor_clr;
		draw_grid_line(vec3(start_x, 0, z), vec3(end_x, 0, z), clr);
	}
}








EditRendererBase::EditRendererBase()
	: cam_override(cam_override_node) {
	SetFrame(BlackFrame());
	WantFocus();
	
}

EditRendererV1::EditRendererV1() {}

void EditRendererV1::PaintObject(Draw& d, const GeomObjectState& os, const mat4& view, const Frustum& frustum) {
	GeomObject& go = *os.obj;
	if (!go.is_visible)
		return;
	Size sz = GetSize();
	Color clr = White();
	int lw = 1;
	bool z_cull = view_mode == VIEWMODE_PERSPECTIVE;
	
	mat4 o_world = (QuatMat(os.orientation) * Translate(os.position) * Scale(os.scale)).GetInverse();
	mat4 o_view = view * o_world;
	
	if (go.IsModel() && go.mdl) {
		const Model& mdl = *go.mdl;
		for (const Mesh& mesh : mdl.meshes) {
			int tri_count = mesh.indices.GetCount() / 3;
			const auto* tri_idx = mesh.indices.Begin();
			for(int i = 0; i < tri_count; i++) {
				const Vertex& v0 = mesh.vertices[tri_idx[0]];
				const Vertex& v1 = mesh.vertices[tri_idx[1]];
				const Vertex& v2 = mesh.vertices[tri_idx[2]];
				
				bool b0 = frustum.Intersects(v0.position.Splice());
				bool b1 = frustum.Intersects(v1.position.Splice());
				bool b2 = frustum.Intersects(v2.position.Splice());
				
				if (b0 || b1) DrawLine(sz, d, o_view, v0, v1, lw, clr, z_cull);
				if (b1 || b2) DrawLine(sz, d, o_view, v1, v2, lw, clr, z_cull);
				if (b2 || b0) DrawLine(sz, d, o_view, v2, v0, lw, clr, z_cull);
				
				tri_idx += 3;
			}
		}
		
	}
	else if (go.IsOctree()) {
		Octree& o = go.octree_ptr ? *go.octree_ptr : go.octree.octree;
		OctreeFrustumIterator iter = o.GetFrustumIterator(frustum);
		vec3 fx_pos = vec3(0);
		quat fx_ori = Identity<quat>();
		Vector<GeomPointcloudEffectTransform*> effects;
		go.GetPointcloudEffects(effects);
		auto apply_local = [](vec3& pos, quat& ori, const vec3& lpos, const quat& lori) {
			pos = pos + VectorTransform(lpos, ori);
			ori = ori * lori;
		};
		for (GeomPointcloudEffectTransform* fx : effects) {
			if (!fx || !fx->enabled)
				continue;
			apply_local(fx_pos, fx_ori, fx->position, fx->orientation);
		}
		
		while (iter) {
			const OctreeNode& n = *iter;
			
			for (const auto& one_obj : n.objs) {
				
				const OctreeObject& obj = *one_obj;
				vec3 pos = obj.GetPosition();
				vec3 world = VectorTransform(pos, fx_ori) + fx_pos;
				world = VectorTransform(world * os.scale, os.orientation) + os.position;
				
				if (!frustum.Intersects(world))
					continue;
				
				vec3 cam_pos = VecMul(view, world);
				
				float x = (cam_pos[0] + 1) * 0.5 * sz.cx;
				float y = (-cam_pos[1] + 1) * 0.5 * sz.cy;
				
				d.DrawRect(x, y, 1, 1, clr);
			}
			
			iter++;
		}
	}
	else if (go.IsCamera()) {
		float fov_deg = 90.0f;
		float scale = 0.8f;
		if (ctx && ctx->state) {
			GeomCamera& ref_cam = ctx->state->GetProgram();
			fov_deg = ref_cam.fov;
			scale = Max(0.2f, ref_cam.scale * 0.5f);
		}
		DrawCameraGizmo(sz, d, view, os.position, os.orientation, fov_deg, scale, CameraColor(go), z_cull);
	}
	DrawUiTypeOverlay(sz, d, view, os, z_cull, ctx);
	if (GeomEditableMesh* mesh = go.FindEditableMesh()) {
		if (!mesh->points.IsEmpty() || !mesh->lines.IsEmpty() || !mesh->faces.IsEmpty()) {
			const Vector<float>* weights = nullptr;
			if (ctx && ctx->show_weights && !ctx->weight_bone.IsEmpty()) {
				if (GeomSkinWeights* sw = go.FindSkinWeights()) {
					int wi = sw->weights.Find(ctx->weight_bone);
					if (wi >= 0)
						weights = &sw->weights[wi];
				}
			}
			DrawEditableMeshOverlay(sz, d, view, os, *mesh, z_cull, weights, ctx && ctx->show_weights,
				ctx ? &ctx->selected_mesh_points : nullptr,
				ctx ? &ctx->selected_mesh_lines : nullptr,
				ctx ? &ctx->selected_mesh_faces : nullptr);
		}
	}
	if (Geom2DLayer* layer = go.Find2DLayer()) {
		if (!layer->shapes.IsEmpty())
			Draw2DLayerOverlay(sz, d, view, os, *layer, z_cull,
				ctx ? &ctx->selected_2d_shapes : nullptr);
	}
	if (GeomSkeleton* sk = go.FindSkeleton()) {
		for (auto& sub : sk->val.sub) {
			if (IsVfsType(sub, AsTypeHash<GeomBone>()))
				DrawSkeletonRecursive(sz, d, view, os.position, os.orientation, os.scale, sub,
					ctx ? ctx->selected_bone : nullptr, z_cull);
		}
	}
}

void EditRendererV1::Paint(Draw& d) {
	Size sz = GetSize();
	if (!ctx || !ctx->state || !ctx->conf)
		return;
	d.DrawRect(sz, ctx->conf->background_clr);
	
	GeomWorldState& state = *ctx->state;
	GeomScene& scene = state.GetActiveScene();
	GeomCamera& camera = GetGeomCamera();
	
	Camera cam;
	camera.LoadCamera(view_mode, cam, sz);
	Frustum frustum = cam.GetFrustum();
	mat4 view = cam.GetViewMatrix();
	bool z_cull = view_mode == VIEWMODE_PERSPECTIVE;

	mat4 cam_world = cam.GetWorldMatrix();
	mat4 proj = cam.GetProjectionMatrix();
	DrawGroundGrid(sz, d, proj, cam_world, camera.position, *ctx->conf, z_cull);

	int visible_models = 0;
	int visible_triangles = 0;
	auto count_model = [&](GeomObject& go) {
		if (!go.is_visible || !go.IsModel() || !go.mdl)
			return;
		visible_models++;
		const Model& mdl = *go.mdl;
		for (const Mesh& mesh : mdl.meshes)
			visible_triangles += mesh.indices.GetCount() / 3;
	};
	if (ctx->anim && ctx->anim->is_playing) {
		for (GeomObjectState& os : state.objs)
			if (os.obj) count_model(*os.obj);
	}
	else {
		GeomObjectCollection iter(scene);
		for (GeomObject& go : iter)
			count_model(go);
	}
	{
		static bool printed = false;
		if (!printed) {
			Cout() << "RenderStatsV1: models=" << visible_models
			       << " triangles=" << visible_triangles << "\n";
			if (visible_models == 0 || visible_triangles == 0)
				Cout() << "RenderStatsV1: no visible model triangles rendered\n";
			Cout().Flush();
			printed = true;
		}
	}
	
	/*if (view_mode == VIEWMODE_PERSPECTIVE) {
		mat4 world = cam.GetWorldMatrix();
		mat4 proj = cam.GetProjectionMatrix();
		//proj.SetPerspectiveRH_NO(DEG2RAD(120/2), (float)sz.cx / sz.cy, 0.1, 100.0);
		view = proj * world;
	}*/
	
	if (cam_src == CAMSRC_VIDEOIMPORT_FOCUS ||
		cam_src == CAMSRC_VIDEOIMPORT_PROGRAM) {
		if (!ctx->anim || !ctx->video)
			return;
		int frame_i = ctx->anim->position;
		if (frame_i < 0 || frame_i >= ctx->video->uncam.frames.GetCount())
			return;
		UncameraFrame& frame = ctx->video->uncam.frames[frame_i];
		
		GeomObjectState os;
		VfsValue temp_node;
		GeomObject& go = temp_node.CreateExt<GeomObject>();
		go.type = GeomObject::O_OCTREE;
		os.obj = &go;
		os.position = vec3(0);
		os.orientation = Identity<quat>();
		os.scale = vec3(1);
		go.octree_ptr = &frame.otree;
		PaintObject(d, os, view, frustum);
	}
	if (ctx->anim && ctx->anim->is_playing) {
		for (GeomObjectState& os : state.objs) {
			PaintObject(d, os, view, frustum);
		}
	}
	else {
		GeomObjectCollection iter(scene);
		GeomObjectState os;
		for (GeomObject& go : iter) {
			os.obj = &go;
			os.position = vec3(0);
			os.orientation = Identity<quat>();
			os.scale = vec3(1);
			if (GeomTransform* tr = go.FindTransform()) {
				os.position = tr->position;
				os.orientation = tr->orientation;
				os.scale = tr->scale;
			}
			PaintObject(d, os, view, frustum);
		}
	}
	auto draw_frustum = [&](const vec3& pos, const quat& orient, float fov_deg, float scale, Color clr) {
		Vector<vec3> corners;
		{
			Camera cam;
			float aspect = (float)sz.cx / (float)sz.cy;
			cam.SetPerspective(fov_deg, aspect, 0.1, 3.0);
			cam.SetWorld(pos, orient, scale);
			Frustum frustum = cam.GetFrustum();
			corners.SetCount(8);
			frustum.GetCorners(corners.Begin());
		}
		DrawRect(sz, d, view, pos, Size(2,2), clr, z_cull);
		int lw = 1;
		DrawLine(sz, d, view, corners[0], corners[1], lw, clr, z_cull);
		DrawLine(sz, d, view, corners[2], corners[3], lw, clr, z_cull);
		DrawLine(sz, d, view, corners[4], corners[5], lw, clr, z_cull);
		DrawLine(sz, d, view, corners[6], corners[7], lw, clr, z_cull);
		DrawLine(sz, d, view, corners[0], corners[2], lw, clr, z_cull);
		DrawLine(sz, d, view, corners[1], corners[3], lw, clr, z_cull);
		DrawLine(sz, d, view, corners[4], corners[6], lw, clr, z_cull);
		DrawLine(sz, d, view, corners[5], corners[7], lw, clr, z_cull);
		DrawLine(sz, d, view, corners[0], corners[4], lw, clr, z_cull);
		DrawLine(sz, d, view, corners[1], corners[5], lw, clr, z_cull);
		DrawLine(sz, d, view, corners[2], corners[6], lw, clr, z_cull);
		DrawLine(sz, d, view, corners[3], corners[7], lw, clr, z_cull);
	};

	GeomCamera& program = state.GetProgram();
	GeomCamera& focus = state.GetFocus();
	if (state.focus_mode == 1) {
		GeomObject* foc = state.FindObjectByKey(state.focus_object_key);
		if (foc && foc->IsCamera() && foc->is_visible) {
			vec3 pos = vec3(0);
			quat ori = Identity<quat>();
			if (GeomTransform* tr = foc->FindTransform()) {
				pos = tr->position;
				ori = tr->orientation;
			}
			else if (const GeomObjectState* os = state.FindObjectStateByKey(state.focus_object_key)) {
				pos = os->position;
				ori = os->orientation;
			}
			draw_frustum(pos, ori, program.fov, program.scale, Color(255, 255, 172));
		}
	}
	if (state.focus_mode == 2 && state.program_visible)
		draw_frustum(program.position, program.orientation, program.fov, program.scale, Color(220, 120, 120));
	if (state.focus_mode == 3 && state.focus_visible)
		draw_frustum(focus.position, focus.orientation, focus.fov, focus.scale, Color(120, 220, 120));

	if (ctx && ctx->selection_gizmo_enabled && ctx->selection_center_valid) {
		DrawSelectionGizmo(sz, d, view, ctx->selection_center_world, z_cull, 0.4f);
		CountGizmoPixels(*ctx, sz, view, ctx->selection_center_world);
	}
	
	// Overlay: active camera axes and forward vector
	{
		vec3 fwd = VectorTransform(VEC_FWD, camera.orientation);
		vec3 right = VectorTransform(VEC_RIGHT, camera.orientation);
		vec3 up = VectorTransform(VEC_UP, camera.orientation);
		String info = Format("cam fwd=(%.2f %.2f %.2f) right=(%.2f %.2f %.2f) up=(%.2f %.2f %.2f)",
			fwd[0], fwd[1], fwd[2], right[0], right[1], right[2], up[0], up[1], up[2]);
		int y = 4;
		d.DrawText(4, y, info, StdFont().Bold(), LtGray());
		y += StdFont().Bold().Info().GetHeight() + 2;
		if (ctx && ctx->show_hud) {
			for (const String& line : ctx->hud_lines) {
				d.DrawText(4, y, line, StdFont(), LtGray());
				y += StdFont().Info().GetHeight() + 1;
			}
			if (ctx->show_hud_help && !ctx->hud_help.IsEmpty()) {
				y += 6;
				for (const String& line : ctx->hud_help) {
					d.DrawText(4, y, line, StdFont(), Gray());
					y += StdFont().Info().GetHeight() + 1;
				}
			}
		}
	}
	
	// Draw red-green-blue unit-vectors
	if (1) {
		vec3 axes[3] = {vec3(1,0,0), vec3(0,1,0), vec3(0,0,1)};
		Color clr[3] = {LtRed(), LtGreen(), LtBlue()};
		int pad = 20;
		Point pt0(pad, sz.cy - pad);
		float len = 20;
		mat4 view_orient = QuatMat(MatQuat(view));
		for(int i = 0; i < 3; i++) {
			vec3 orig = VecMul(view_orient, vec3(0));
			vec3 ax = VecMul(view_orient, axes[i]);
			vec3 diff = ax - orig;
				diff[1] *= -1; // 3d space top is +1 and bottom is -1 and 2d space top is 0 and bottom is sz.cy
			diff.Normalize();
			if (view_mode != VIEWMODE_PERSPECTIVE)
				diff *= -1;
			Point pt1(pt0.x - diff[0] * len, pt0.y - diff[1] * len);
			d.DrawLine(pt0, pt1, 1, clr[i]);
		}
	}
	//LOG("");
	
}

EditRendererV2::EditRendererV2() {}
EditRendererV2_Ogl::EditRendererV2_Ogl() {}

namespace {

struct SoftSurface {
	Size sz;
	ImageBuffer ib;
	Vector<float> zbuf;
	int pixels_written = 0;
	
	SoftSurface(Size s, Color bg) : sz(s), ib(s), zbuf(s.cx * s.cy, 1e9f) {
		for (int y = 0; y < sz.cy; y++) {
			RGBA* line = ib[y];
			for (int x = 0; x < sz.cx; x++) {
				line[x] = bg;
			}
		}
	}
	
	void SetPixel(int x, int y, float z, const Color& c) {
		if (x < 0 || y < 0 || x >= sz.cx || y >= sz.cy)
			return;
		int idx = y * sz.cx + x;
		if (z >= zbuf[idx])
			return;
		zbuf[idx] = z;
		ib[y][x] = c;
		pixels_written++;
	}
	
	void DrawLine2D(int x0, int y0, int x1, int y1, const Color& c) {
		int dx = abs(x1 - x0);
		int sx = x0 < x1 ? 1 : -1;
		int dy = -abs(y1 - y0);
		int sy = y0 < y1 ? 1 : -1;
		int err = dx + dy;
		while (true) {
			if (x0 >= 0 && y0 >= 0 && x0 < sz.cx && y0 < sz.cy) {
				ib[y0][x0] = c;
				pixels_written++;
			}
			if (x0 == x1 && y0 == y1)
				break;
			int e2 = 2 * err;
			if (e2 >= dy) {
				err += dy;
				x0 += sx;
			}
			if (e2 <= dx) {
				err += dx;
				y0 += sy;
			}
		}
	}
};

struct TriProj {
	vec2 p;
	float z = 0;
};

struct GridLineDump : Moveable<GridLineDump> {
	vec3 a_world;
	vec3 b_world;
	vec3 a_world_clipped;
	vec3 b_world_clipped;
	vec3 a_cam;
	vec3 b_cam;
	vec3 a_ndc;
	vec3 b_ndc;
	vec2 a_clip;
	vec2 b_clip;
	bool culled = false;
	bool clipped = false;
};

float Edge2D(const vec2& a, const vec2& b, const vec2& c) {
	return (c[0] - a[0]) * (b[1] - a[1]) - (c[1] - a[1]) * (b[0] - a[0]);
}

}

void EditRendererV2::Paint(Draw& d) {
	Size sz = GetSize();
	if (!ctx || !ctx->state || !ctx->conf)
		return;
	
	GeomWorldState& state = *ctx->state;
	GeomScene& scene = state.GetActiveScene();
	GeomCamera& camera = GetGeomCamera();
	
	Camera cam;
	camera.LoadCamera(view_mode, cam, sz);
	Frustum frustum = cam.GetFrustum();
	mat4 view = cam.GetViewMatrix();
	mat4 cam_world = cam.GetWorldMatrix();
	mat4 proj = cam.GetProjectionMatrix();
	bool z_cull = view_mode == VIEWMODE_PERSPECTIVE;
	
	SoftSurface surf(sz, ctx->conf->background_clr);
	Vector<GridLineDump> grid_dumps;
	bool dump_grid = !ctx->conf->dump_grid_path.IsEmpty();
	auto draw_grid_line = [&](const vec3& a, const vec3& b, const Color& clr) {
		GridLineDump dump;
		dump.a_world = a;
		dump.b_world = b;
		vec3 a_world = a;
		vec3 b_world = b;
		vec4 ap4 = proj * (cam_world * a_world.Embed());
		vec4 bp4 = proj * (cam_world * b_world.Embed());
		float u1 = 0.0f;
		float u2 = 1.0f;
		if (!ClipLineClipSpace(ap4, bp4, &u1, &u2)) {
			dump.culled = true;
			if (dump_grid) grid_dumps.Add(dump);
			return;
		}
		if (ap4[3] == 0 || bp4[3] == 0) {
			dump.culled = true;
			if (dump_grid) grid_dumps.Add(dump);
			return;
		}
		vec3 ap = ap4.Splice() / ap4[3];
		vec3 bp = bp4.Splice() / bp4[3];
		dump.a_world_clipped = a_world + (b_world - a_world) * u1;
		dump.b_world_clipped = a_world + (b_world - a_world) * u2;
		dump.a_cam = VecMul(cam_world, a_world);
		dump.b_cam = VecMul(cam_world, b_world);
		dump.a_ndc = ap;
		dump.b_ndc = bp;
		vec2 a2(ap[0], ap[1]);
		vec2 b2(bp[0], bp[1]);
		dump.a_clip = a2;
		dump.b_clip = b2;
		if (!ClipLineNdc(a2, b2)) {
			dump.clipped = false;
			if (dump_grid) grid_dumps.Add(dump);
			return;
		}
		dump.clipped = true;
		dump.a_clip = a2;
		dump.b_clip = b2;
		if (dump_grid) grid_dumps.Add(dump);
		int x0 = (int)floor((a2[0] + 1) * 0.5 * (float)(sz.cx - 1) + 0.5f);
		int y0 = (int)floor((-a2[1] + 1) * 0.5 * (float)(sz.cy - 1) + 0.5f);
		int x1 = (int)floor((b2[0] + 1) * 0.5 * (float)(sz.cx - 1) + 0.5f);
		int y1 = (int)floor((-b2[1] + 1) * 0.5 * (float)(sz.cy - 1) + 0.5f);
		surf.DrawLine2D(x0, y0, x1, y1, clr);
	};
	float grid_major = ctx->conf->grid_major_step;
	int grid_divs = ctx->conf->grid_minor_divs;
	if (grid_major <= 0.0001f)
		grid_major = 1.0f;
	if (grid_divs < 1)
		grid_divs = 1;
	float grid_minor = grid_major / (float)grid_divs;
	float grid_extent = ctx->conf->grid_extent;
	if (grid_extent < grid_major)
		grid_extent = grid_major;
	if (ctx->conf->show_grid) {
		float major = grid_major;
		int divs = grid_divs;
		float minor = grid_minor;
		float extent = grid_extent;
		if (extent < major)
			extent = major;
		float start_x = floor((camera.position[0] - extent) / minor) * minor;
		float end_x = ceil((camera.position[0] + extent) / minor) * minor;
		float start_z = floor((camera.position[2] - extent) / minor) * minor;
		float end_z = ceil((camera.position[2] + extent) / minor) * minor;
		for (float x = start_x; x <= end_x + minor * 0.5f; x += minor) {
			int idx = (int)floor(x / minor + 0.5f);
			int mod = idx % divs;
			if (mod < 0)
				mod += divs;
			bool is_major = (mod == 0);
			Color clr = is_major ? ctx->conf->grid_major_clr : ctx->conf->grid_minor_clr;
			draw_grid_line(vec3(x, 0, start_z), vec3(x, 0, end_z), clr);
		}
		for (float z = start_z; z <= end_z + minor * 0.5f; z += minor) {
			int idx = (int)floor(z / minor + 0.5f);
			int mod = idx % divs;
			if (mod < 0)
				mod += divs;
			bool is_major = (mod == 0);
			Color clr = is_major ? ctx->conf->grid_major_clr : ctx->conf->grid_minor_clr;
			draw_grid_line(vec3(start_x, 0, z), vec3(end_x, 0, z), clr);
		}
	}
	if (dump_grid && !ctx->conf->dump_grid_done) {
		ctx->conf->dump_grid_done = true;
		FileOut out(ctx->conf->dump_grid_path);
		if (out.IsOpen()) {
			auto wvec3 = [&](const vec3& v) {
				out << v[0] << " " << v[1] << " " << v[2];
			};
			auto wvec2 = [&](const vec2& v) {
				out << v[0] << " " << v[1];
			};
			out << "size " << sz.cx << " " << sz.cy << "\n";
			out << "cam_pos "; wvec3(camera.position); out << "\n";
			out << "cam_orient " << camera.orientation[0] << " "
			    << camera.orientation[1] << " "
			    << camera.orientation[2] << " "
			    << camera.orientation[3] << "\n";
			out << "cam_scale " << camera.scale << " fov " << camera.fov << "\n";
			out << "grid major " << grid_major << " minor " << grid_minor
			    << " extent " << grid_extent << " divs " << grid_divs << "\n";
			out << "world\n";
			for (int r = 0; r < 4; r++) {
				for (int c = 0; c < 4; c++)
					out << cam_world[r][c] << (c == 3 ? '\n' : ' ');
			}
			out << "proj\n";
			for (int r = 0; r < 4; r++) {
				for (int c = 0; c < 4; c++)
					out << proj[r][c] << (c == 3 ? '\n' : ' ');
			}
			out << "view\n";
			for (int r = 0; r < 4; r++) {
				for (int c = 0; c < 4; c++)
					out << view[r][c] << (c == 3 ? '\n' : ' ');
			}
			out << "lines " << grid_dumps.GetCount() << "\n";
			for (const auto& ln : grid_dumps) {
				out << "line ";
				wvec3(ln.a_world); out << " ";
				wvec3(ln.b_world); out << " ";
				wvec3(ln.a_world_clipped); out << " ";
				wvec3(ln.b_world_clipped); out << " ";
				wvec3(ln.a_cam); out << " ";
				wvec3(ln.b_cam); out << " ";
				wvec3(ln.a_ndc); out << " ";
				wvec3(ln.b_ndc); out << " ";
				wvec2(ln.a_clip); out << " ";
				wvec2(ln.b_clip); out << " ";
				out << (ln.culled ? 1 : 0) << " " << (ln.clipped ? 1 : 0) << "\n";
			}
		}
	}
	
	auto draw_line_ndc = [&](const vec3& a, const vec3& b, const Color& clr) {
		vec2 a2(a[0], a[1]);
		vec2 b2(b[0], b[1]);
		if (!ClipLineNdc(a2, b2))
			return;
		int x0 = (int)floor((a2[0] + 1) * 0.5 * (float)(sz.cx - 1) + 0.5f);
		int y0 = (int)floor((-a2[1] + 1) * 0.5 * (float)(sz.cy - 1) + 0.5f);
		int x1 = (int)floor((b2[0] + 1) * 0.5 * (float)(sz.cx - 1) + 0.5f);
		int y1 = (int)floor((-b2[1] + 1) * 0.5 * (float)(sz.cy - 1) + 0.5f);
		surf.DrawLine2D(x0, y0, x1, y1, clr);
	};
	auto clip_poly = [&](const Vector<vec4>& input, bool near_plane) {
		Vector<vec4> output;
		if (input.IsEmpty())
			return output;
		auto eval = [&](const vec4& v) -> float {
			return near_plane ? (v[2] + v[3]) : (v[3] - v[2]);
		};
		vec4 s = input.Top();
		float ds = eval(s);
		for (const vec4& e : input) {
			float de = eval(e);
			bool ins = ds >= 0;
			bool ine = de >= 0;
			if (ins && ine) {
				output.Add(e);
			} else if (ins && !ine) {
				float t = ds / (ds - de);
				output.Add(s + (e - s) * t);
			} else if (!ins && ine) {
				float t = ds / (ds - de);
				output.Add(s + (e - s) * t);
				output.Add(e);
			}
			s = e;
			ds = de;
		}
		return output;
	};
	auto draw_triangle_raw = [&](const vec4& c0, const vec4& c1, const vec4& c2, const Color& clr) {
		if (wireframe_only) {
			vec4 e0 = c0, e1 = c1;
			if (ClipLineClipSpace(e0, e1) && e0[3] != 0 && e1[3] != 0) {
				vec3 n0 = e0.Splice() / e0[3];
				vec3 n1 = e1.Splice() / e1[3];
				draw_line_ndc(n0, n1, clr);
			}
			e0 = c1; e1 = c2;
			if (ClipLineClipSpace(e0, e1) && e0[3] != 0 && e1[3] != 0) {
				vec3 n0 = e0.Splice() / e0[3];
				vec3 n1 = e1.Splice() / e1[3];
				draw_line_ndc(n0, n1, clr);
			}
			e0 = c2; e1 = c0;
			if (ClipLineClipSpace(e0, e1) && e0[3] != 0 && e1[3] != 0) {
				vec3 n0 = e0.Splice() / e0[3];
				vec3 n1 = e1.Splice() / e1[3];
				draw_line_ndc(n0, n1, clr);
			}
			return;
		}
		if (c0[3] == 0 || c1[3] == 0 || c2[3] == 0)
			return;
		vec3 n0 = c0.Splice() / c0[3];
		vec3 n1 = c1.Splice() / c1[3];
		vec3 n2 = c2.Splice() / c2[3];
		if (n0[2] < -1 && n1[2] < -1 && n2[2] < -1)
			return;
		if (n0[2] > 1 && n1[2] > 1 && n2[2] > 1)
			return;
		
		TriProj tp[3];
		vec3 ndc[3] = {n0, n1, n2};
		for (int i = 0; i < 3; i++) {
			tp[i].p[0] = (ndc[i][0] + 1.0f) * 0.5f * (float)(sz.cx - 1);
			tp[i].p[1] = (-ndc[i][1] + 1.0f) * 0.5f * (float)(sz.cy - 1);
			tp[i].z = ndc[i][2];
		}
		
		float area = Edge2D(tp[0].p, tp[1].p, tp[2].p);
		if (area == 0)
			return;
		int minx = (int)floor(min(tp[0].p[0], min(tp[1].p[0], tp[2].p[0])));
		int maxx = (int)ceil(max(tp[0].p[0], max(tp[1].p[0], tp[2].p[0])));
		int miny = (int)floor(min(tp[0].p[1], min(tp[1].p[1], tp[2].p[1])));
		int maxy = (int)ceil(max(tp[0].p[1], max(tp[1].p[1], tp[2].p[1])));
		minx = max(minx, 0);
		miny = max(miny, 0);
		maxx = min(maxx, sz.cx - 1);
		maxy = min(maxy, sz.cy - 1);
		for (int y = miny; y <= maxy; y++) {
			for (int x = minx; x <= maxx; x++) {
				vec2 p((float)x + 0.5f, (float)y + 0.5f);
				float w0 = Edge2D(tp[1].p, tp[2].p, p);
				float w1 = Edge2D(tp[2].p, tp[0].p, p);
				float w2 = Edge2D(tp[0].p, tp[1].p, p);
				if ((area > 0 && (w0 < 0 || w1 < 0 || w2 < 0)) ||
				    (area < 0 && (w0 > 0 || w1 > 0 || w2 > 0)))
					continue;
				w0 /= area;
				w1 /= area;
				w2 /= area;
				float z = w0 * tp[0].z + w1 * tp[1].z + w2 * tp[2].z;
				surf.SetPixel(x, y, z, clr);
			}
		}
	};
	auto draw_triangle = [&](const vec4& c0, const vec4& c1, const vec4& c2, const Color& clr) {
		Vector<vec4> poly;
		poly.Add(c0);
		poly.Add(c1);
		poly.Add(c2);
		poly = clip_poly(poly, true);
		if (poly.GetCount() < 3)
			return;
		poly = clip_poly(poly, false);
		if (poly.GetCount() < 3)
			return;
		for (int i = 1; i + 1 < poly.GetCount(); i++)
			draw_triangle_raw(poly[0], poly[i], poly[i + 1], clr);
	};
	
	int models_painted = 0;
	int triangles_submitted = 0;
	Vector<UiSceneLight> ui_lights;
	CollectUiLights(ctx, ui_lights);
	auto paint_model = [&](const GeomObjectState& os, const Model& mdl) {
		models_painted++;
		mat4 o_world = Translate(os.position) * QuatMat(os.orientation) * Scale(os.scale);
		mat4 o_view = view * o_world;
		for (const Mesh& mesh : mdl.meshes) {
			vec3 base_clr(1, 1, 1);
			vec3 emissive(0, 0, 0);
			vec3 diffuse(1, 1, 1);
			vec3 specular(0.2f, 0.2f, 0.2f);
			vec3 ambient(0.1f, 0.1f, 0.1f);
			float shininess = 16.0f;
			if (mesh.material >= 0 && mdl.materials.Find(mesh.material) >= 0) {
				const Material& mat = mdl.materials.Get(mesh.material);
				base_clr = mat.params->base_clr_factor.Splice();
				emissive = mat.params->emissive_factor;
				diffuse = mat.params->diffuse;
				specular = mat.params->specular;
				ambient = mat.params->ambient;
				shininess = mat.params->shininess;
				if (diffuse.GetLength() == 0)
					diffuse = vec3(1, 1, 1);
				if (specular.GetLength() == 0)
					specular = vec3(0.2f, 0.2f, 0.2f);
				if (ambient.GetLength() == 0)
					ambient = vec3(0.1f, 0.1f, 0.1f);
				if (shininess <= 0)
					shininess = 16.0f;
			}
			vec3 base = base_clr * diffuse;
			const auto* tri_idx = mesh.indices.Begin();
			int tri_count = mesh.indices.GetCount() / 3;
			for (int i = 0; i < tri_count; i++) {
				const Vertex& v0 = mesh.vertices[tri_idx[0]];
				const Vertex& v1 = mesh.vertices[tri_idx[1]];
				const Vertex& v2 = mesh.vertices[tri_idx[2]];
				vec3 n = Cross(v1.position.Splice() - v0.position.Splice(),
				               v2.position.Splice() - v0.position.Splice());
				if (n.GetLength() == 0) {
					tri_idx += 3;
					continue;
				}
				n.Normalize();
				vec3 n_world = VectorTransform(n, os.orientation);
				vec3 p0 = (o_world * v0.position).Splice();
				vec3 p1 = (o_world * v1.position).Splice();
				vec3 p2 = (o_world * v2.position).Splice();
				vec3 center = (p0 + p1 + p2) / 3.0f;
				vec3 view_dir = camera.position - center;
				if (view_dir.GetLength() > 0)
					view_dir.Normalize();
				UiLightEval ev = EvaluateUiLighting(ui_lights, center, n_world, view_dir, shininess);
				vec3 shaded = base * (ev.ambient + ev.diffuse * 0.75f)
				            + base_clr * ambient + specular * ev.specular + emissive;
				shaded[0] = Clamp(shaded[0], 0.0f, 1.0f);
				shaded[1] = Clamp(shaded[1], 0.0f, 1.0f);
				shaded[2] = Clamp(shaded[2], 0.0f, 1.0f);
				int r = (int)Clamp(shaded[0] * 255.0f, 0.0f, 255.0f);
				int g = (int)Clamp(shaded[1] * 255.0f, 0.0f, 255.0f);
				int b = (int)Clamp(shaded[2] * 255.0f, 0.0f, 255.0f);
				Color clr(r, g, b);
				vec4 c0 = o_view * v0.position;
				vec4 c1 = o_view * v1.position;
				vec4 c2 = o_view * v2.position;
				draw_triangle(c0, c1, c2, clr);
				triangles_submitted++;
				tri_idx += 3;
			}
		}
	};
	
	if (ctx->anim && ctx->anim->is_playing) {
		for (GeomObjectState& os : state.objs) {
			GeomObject& go = *os.obj;
			if (!go.is_visible)
				continue;
			if (go.IsModel() && go.mdl)
				paint_model(os, *go.mdl);
		}
	}
	else {
		GeomObjectCollection iter(scene);
		GeomObjectState os;
		for (GeomObject& go : iter) {
			if (!go.is_visible)
				continue;
			if (go.IsModel() && go.mdl) {
				os.obj = &go;
				os.position = vec3(0);
				os.orientation = Identity<quat>();
				os.scale = vec3(1);
				if (GeomTransform* tr = go.FindTransform()) {
					os.position = tr->position;
					os.orientation = tr->orientation;
					os.scale = tr->scale;
				}
				paint_model(os, *go.mdl);
			}
		}
	}
	
	d.DrawImage(0, 0, surf.ib);
	// Camera gizmos + overlays (match V1 behavior)
	auto draw_camera_gizmos = [&] {
		auto draw_one = [&](const vec3& pos, const quat& ori, float fov_deg, float scale, Color clr) {
			DrawCameraGizmo(sz, d, view, pos, ori, fov_deg, scale, clr, z_cull);
		};
		if (ctx->anim && ctx->anim->is_playing) {
			for (GeomObjectState& os : state.objs) {
				if (!os.obj || !os.obj->IsCamera() || !os.obj->is_visible)
					continue;
				float fov_deg = 90.0f;
				float scale = 0.8f;
				if (ctx && ctx->state) {
					GeomCamera& ref_cam = ctx->state->GetProgram();
					fov_deg = ref_cam.fov;
					scale = Max(0.2f, ref_cam.scale * 0.5f);
				}
				draw_one(os.position, os.orientation, fov_deg, scale, CameraColor(*os.obj));
			}
		}
		else {
			GeomObjectCollection iter(scene);
			for (GeomObject& go : iter) {
				if (!go.IsCamera() || !go.is_visible)
					continue;
				vec3 pos = vec3(0);
				quat ori = Identity<quat>();
				if (GeomTransform* tr = go.FindTransform()) {
					pos = tr->position;
					ori = tr->orientation;
				}
				float fov_deg = 90.0f;
				float scale = 0.8f;
				if (ctx && ctx->state) {
					GeomCamera& ref_cam = ctx->state->GetProgram();
					fov_deg = ref_cam.fov;
					scale = Max(0.2f, ref_cam.scale * 0.5f);
				}
				draw_one(pos, ori, fov_deg, scale, CameraColor(go));
			}
		}
	};
	draw_camera_gizmos();
	auto draw_ui_overlays = [&] {
		if (ctx->anim && ctx->anim->is_playing) {
			for (const GeomObjectState& os : state.objs) {
				if (!os.obj || !os.obj->is_visible)
					continue;
				DrawUiTypeOverlay(sz, d, view, os, z_cull, ctx);
			}
		}
		else {
			GeomObjectCollection iter(scene);
			GeomObjectState os;
			for (GeomObject& go : iter) {
				if (!go.is_visible)
					continue;
				os.obj = &go;
				os.position = vec3(0);
				os.orientation = Identity<quat>();
				os.scale = vec3(1);
				if (GeomTransform* tr = go.FindTransform()) {
					os.position = tr->position;
					os.orientation = tr->orientation;
					os.scale = tr->scale;
				}
				DrawUiTypeOverlay(sz, d, view, os, z_cull, ctx);
			}
		}
	};
	draw_ui_overlays();
	if (ctx && ctx->selection_gizmo_enabled && ctx->selection_center_valid) {
		DrawSelectionGizmo(sz, d, view, ctx->selection_center_world, z_cull, 0.4f);
		CountGizmoPixels(*ctx, sz, view, ctx->selection_center_world);
	}
	// Skeleton overlay
	if (ctx && ctx->state) {
		if (ctx->anim && ctx->anim->is_playing) {
			for (GeomObjectState& os : state.objs) {
				if (!os.obj || !os.obj->is_visible)
					continue;
				if (GeomSkeleton* sk = os.obj->FindSkeleton()) {
					for (auto& sub : sk->val.sub) {
						if (IsVfsType(sub, AsTypeHash<GeomBone>()))
							DrawSkeletonRecursive(sz, d, view, os.position, os.orientation, os.scale, sub,
								ctx ? ctx->selected_bone : nullptr, z_cull);
					}
				}
			}
		}
		else {
			GeomObjectCollection iter(scene);
			GeomObjectState os;
			for (GeomObject& go : iter) {
				if (!go.is_visible)
					continue;
				if (!go.FindSkeleton())
					continue;
				os.obj = &go;
				os.position = vec3(0);
				os.orientation = Identity<quat>();
				os.scale = vec3(1);
				if (GeomTransform* tr = go.FindTransform()) {
					os.position = tr->position;
					os.orientation = tr->orientation;
					os.scale = tr->scale;
				}
				GeomSkeleton* sk = go.FindSkeleton();
				for (auto& sub : sk->val.sub) {
					if (IsVfsType(sub, AsTypeHash<GeomBone>()))
						DrawSkeletonRecursive(sz, d, view, os.position, os.orientation, os.scale, sub,
							ctx ? ctx->selected_bone : nullptr, z_cull);
				}
			}
		}
	}

	auto draw_frustum = [&](const vec3& pos, const quat& orient, float fov_deg, float scale, Color clr) {
		Vector<vec3> corners;
		{
			Camera cam;
			float aspect = (float)sz.cx / (float)sz.cy;
			cam.SetPerspective(fov_deg, aspect, 0.1, 3.0);
			cam.SetWorld(pos, orient, scale);
			Frustum frustum = cam.GetFrustum();
			corners.SetCount(8);
			frustum.GetCorners(corners.Begin());
		}
		DrawRect(sz, d, view, pos, Size(2,2), clr, z_cull);
		int lw = 1;
		DrawLine(sz, d, view, corners[0], corners[1], lw, clr, z_cull);
		DrawLine(sz, d, view, corners[2], corners[3], lw, clr, z_cull);
		DrawLine(sz, d, view, corners[4], corners[5], lw, clr, z_cull);
		DrawLine(sz, d, view, corners[6], corners[7], lw, clr, z_cull);
		DrawLine(sz, d, view, corners[0], corners[2], lw, clr, z_cull);
		DrawLine(sz, d, view, corners[1], corners[3], lw, clr, z_cull);
		DrawLine(sz, d, view, corners[4], corners[6], lw, clr, z_cull);
		DrawLine(sz, d, view, corners[5], corners[7], lw, clr, z_cull);
		DrawLine(sz, d, view, corners[0], corners[4], lw, clr, z_cull);
		DrawLine(sz, d, view, corners[1], corners[5], lw, clr, z_cull);
		DrawLine(sz, d, view, corners[2], corners[6], lw, clr, z_cull);
		DrawLine(sz, d, view, corners[3], corners[7], lw, clr, z_cull);
	};

	GeomCamera& program = state.GetProgram();
	GeomCamera& focus = state.GetFocus();
	if (state.focus_mode == 1) {
		GeomObject* foc = state.FindObjectByKey(state.focus_object_key);
		if (foc && foc->IsCamera() && foc->is_visible) {
			vec3 pos = vec3(0);
			quat ori = Identity<quat>();
			if (GeomTransform* tr = foc->FindTransform()) {
				pos = tr->position;
				ori = tr->orientation;
			}
			else if (const GeomObjectState* os = state.FindObjectStateByKey(state.focus_object_key)) {
				pos = os->position;
				ori = os->orientation;
			}
			draw_frustum(pos, ori, program.fov, program.scale, Color(255, 255, 172));
		}
	}
	if (state.focus_mode == 2 && state.program_visible)
		draw_frustum(program.position, program.orientation, program.fov, program.scale, Color(220, 120, 120));
	if (state.focus_mode == 3 && state.focus_visible)
		draw_frustum(focus.position, focus.orientation, focus.fov, focus.scale, Color(120, 220, 120));

	{
		vec3 fwd = VectorTransform(VEC_FWD, camera.orientation);
		vec3 right = VectorTransform(VEC_RIGHT, camera.orientation);
		vec3 up = VectorTransform(VEC_UP, camera.orientation);
		String info = Format("cam fwd=(%.2f %.2f %.2f) right=(%.2f %.2f %.2f) up=(%.2f %.2f %.2f)",
			fwd[0], fwd[1], fwd[2], right[0], right[1], right[2], up[0], up[1], up[2]);
		int y = 4;
		d.DrawText(4, y, info, StdFont().Bold(), LtGray());
		y += StdFont().Bold().Info().GetHeight() + 2;
		if (ctx && ctx->show_hud) {
			for (const String& line : ctx->hud_lines) {
				d.DrawText(4, y, line, StdFont(), LtGray());
				y += StdFont().Info().GetHeight() + 1;
			}
			if (ctx->show_hud_help && !ctx->hud_help.IsEmpty()) {
				y += 6;
				for (const String& line : ctx->hud_help) {
					d.DrawText(4, y, line, StdFont(), Gray());
					y += StdFont().Info().GetHeight() + 1;
				}
			}
		}
	}
	if (1) {
		vec3 axes[3] = {vec3(1,0,0), vec3(0,1,0), vec3(0,0,1)};
		Color clr[3] = {LtRed(), LtGreen(), LtBlue()};
		int pad = 20;
		Point pt0(pad, sz.cy - pad);
		float len = 20;
		mat4 view_orient = QuatMat(MatQuat(view));
		for(int i = 0; i < 3; i++) {
			vec3 orig = VecMul(view_orient, vec3(0));
			vec3 ax = VecMul(view_orient, axes[i]);
			vec3 diff = ax - orig;
			diff[1] *= -1;
			diff.Normalize();
			if (view_mode != VIEWMODE_PERSPECTIVE)
				diff *= -1;
			Point pt1(pt0.x - diff[0] * len, pt0.y - diff[1] * len);
			d.DrawLine(pt0, pt1, 1, clr[i]);
		}
	}
	if (ctx->anim && ctx->anim->is_playing) {
		for (GeomObjectState& os : state.objs) {
			if (!os.obj || !os.obj->is_visible)
				continue;
			if (GeomEditableMesh* mesh = os.obj->FindEditableMesh()) {
				if (!mesh->points.IsEmpty() || !mesh->lines.IsEmpty() || !mesh->faces.IsEmpty()) {
					const Vector<float>* weights = nullptr;
					if (ctx && ctx->show_weights && !ctx->weight_bone.IsEmpty()) {
						if (GeomSkinWeights* sw = os.obj->FindSkinWeights()) {
							int wi = sw->weights.Find(ctx->weight_bone);
							if (wi >= 0)
								weights = &sw->weights[wi];
						}
					}
					DrawEditableMeshOverlay(sz, d, view, os, *mesh, z_cull, weights, ctx && ctx->show_weights,
						ctx ? &ctx->selected_mesh_points : nullptr,
						ctx ? &ctx->selected_mesh_lines : nullptr,
						ctx ? &ctx->selected_mesh_faces : nullptr);
				}
			}
		}
	}
	else {
		GeomObjectCollection iter(scene);
		GeomObjectState os;
		for (GeomObject& go : iter) {
			if (!go.is_visible)
				continue;
			if (!go.FindEditableMesh())
				continue;
			os.obj = &go;
			os.position = vec3(0);
			os.orientation = Identity<quat>();
			os.scale = vec3(1);
			if (GeomTransform* tr = go.FindTransform()) {
				os.position = tr->position;
				os.orientation = tr->orientation;
				os.scale = tr->scale;
			}
			const Vector<float>* weights = nullptr;
			if (ctx && ctx->show_weights && !ctx->weight_bone.IsEmpty()) {
				if (GeomSkinWeights* sw = go.FindSkinWeights()) {
					int wi = sw->weights.Find(ctx->weight_bone);
					if (wi >= 0)
						weights = &sw->weights[wi];
				}
			}
			DrawEditableMeshOverlay(sz, d, view, os, *go.FindEditableMesh(), z_cull, weights, ctx && ctx->show_weights,
				ctx ? &ctx->selected_mesh_points : nullptr,
				ctx ? &ctx->selected_mesh_lines : nullptr,
				ctx ? &ctx->selected_mesh_faces : nullptr);
		}
	}

	{
		static bool printed = false;
		if (!printed) {
			Cout() << "RenderStatsV2: models=" << models_painted
			       << " triangles=" << triangles_submitted
			       << " pixels=" << surf.pixels_written << "\n";
			Cout() << "RenderStatsV2: rendered=" << (surf.pixels_written > 0 ? 1 : 0) << "\n";
			if (models_painted == 0 || triangles_submitted == 0 || surf.pixels_written == 0)
				Cout() << "RenderStatsV2: no visible model triangles rendered\n";
			Cout().Flush();
			printed = true;
		}
	}
}

bool RenderSceneV2Headless(Scene3DRenderContext& ctx, Size sz, Scene3DRenderStats* out_stats,
                           Image* out_image, String* out_debug, bool dump_first_tri,
                           ViewMode view_mode, const GeomCamera* cam_override, bool wireframe_only) {
	if (!ctx.conf || !ctx.state)
		return false;
	ctx.gizmo_pixels = 0;
	Scene3DRenderConfig& conf = *ctx.conf;
	GeomWorldState& state = *ctx.state;
	GeomScene& scene = state.GetActiveScene();
	GeomCamera& camera = cam_override ? const_cast<GeomCamera&>(*cam_override) : state.GetProgram();
	Camera cam;
	camera.LoadCamera(view_mode, cam, sz);
	mat4 view = cam.GetViewMatrix();
	mat4 cam_world = cam.GetWorldMatrix();
	mat4 proj = cam.GetProjectionMatrix();
	bool z_cull = true;
	
	SoftSurface surf(sz, conf.background_clr);
	auto draw_grid_line = [&](const vec3& a, const vec3& b, const Color& clr) {
		vec4 ap4 = proj * (cam_world * a.Embed());
		vec4 bp4 = proj * (cam_world * b.Embed());
		if (!ClipLineClipSpace(ap4, bp4))
			return;
		if (ap4[3] == 0 || bp4[3] == 0)
			return;
		vec3 ap = ap4.Splice() / ap4[3];
		vec3 bp = bp4.Splice() / bp4[3];
		vec2 a2(ap[0], ap[1]);
		vec2 b2(bp[0], bp[1]);
		if (!ClipLineNdc(a2, b2))
			return;
		int x0 = (int)floor((a2[0] + 1) * 0.5 * (float)(sz.cx - 1) + 0.5f);
		int y0 = (int)floor((-a2[1] + 1) * 0.5 * (float)(sz.cy - 1) + 0.5f);
		int x1 = (int)floor((b2[0] + 1) * 0.5 * (float)(sz.cx - 1) + 0.5f);
		int y1 = (int)floor((-b2[1] + 1) * 0.5 * (float)(sz.cy - 1) + 0.5f);
		surf.DrawLine2D(x0, y0, x1, y1, clr);
	};
	if (conf.show_grid) {
		float grid_major = conf.grid_major_step <= 0.0001f ? 1.0f : conf.grid_major_step;
		int grid_divs = conf.grid_minor_divs < 1 ? 1 : conf.grid_minor_divs;
		float grid_minor = grid_major / (float)grid_divs;
		float grid_extent = conf.grid_extent < grid_major ? grid_major : conf.grid_extent;
		float start_x = floor((camera.position[0] - grid_extent) / grid_minor) * grid_minor;
		float end_x = ceil((camera.position[0] + grid_extent) / grid_minor) * grid_minor;
		float start_z = floor((camera.position[2] - grid_extent) / grid_minor) * grid_minor;
		float end_z = ceil((camera.position[2] + grid_extent) / grid_minor) * grid_minor;
		for (float x = start_x; x <= end_x + grid_minor * 0.5f; x += grid_minor) {
			int idx = (int)floor(x / grid_minor + 0.5f);
			int mod = idx % grid_divs;
			if (mod < 0)
				mod += grid_divs;
			Color clr = (mod == 0) ? conf.grid_major_clr : conf.grid_minor_clr;
			draw_grid_line(vec3(x, 0, start_z), vec3(x, 0, end_z), clr);
		}
		for (float z = start_z; z <= end_z + grid_minor * 0.5f; z += grid_minor) {
			int idx = (int)floor(z / grid_minor + 0.5f);
			int mod = idx % grid_divs;
			if (mod < 0)
				mod += grid_divs;
			Color clr = (mod == 0) ? conf.grid_major_clr : conf.grid_minor_clr;
			draw_grid_line(vec3(start_x, 0, z), vec3(end_x, 0, z), clr);
		}
	}
	
	auto draw_line_ndc = [&](const vec3& a, const vec3& b, const Color& clr) {
		vec2 a2(a[0], a[1]);
		vec2 b2(b[0], b[1]);
		if (!ClipLineNdc(a2, b2))
			return;
		int x0 = (int)floor((a2[0] + 1) * 0.5 * (float)(sz.cx - 1) + 0.5f);
		int y0 = (int)floor((-a2[1] + 1) * 0.5 * (float)(sz.cy - 1) + 0.5f);
		int x1 = (int)floor((b2[0] + 1) * 0.5 * (float)(sz.cx - 1) + 0.5f);
		int y1 = (int)floor((-b2[1] + 1) * 0.5 * (float)(sz.cy - 1) + 0.5f);
		surf.DrawLine2D(x0, y0, x1, y1, clr);
	};
	auto clip_poly = [&](const Vector<vec4>& input, bool near_plane) {
		Vector<vec4> output;
		if (input.IsEmpty())
			return output;
		auto eval = [&](const vec4& v) -> float {
			return near_plane ? (v[2] + v[3]) : (v[3] - v[2]);
		};
		vec4 s = input.Top();
		float ds = eval(s);
		for (const vec4& e : input) {
			float de = eval(e);
			bool ins = ds >= 0;
			bool ine = de >= 0;
			if (ins && ine) {
				output.Add(e);
			} else if (ins && !ine) {
				float t = ds / (ds - de);
				output.Add(s + (e - s) * t);
			} else if (!ins && ine) {
				float t = ds / (ds - de);
				output.Add(s + (e - s) * t);
				output.Add(e);
			}
			s = e;
			ds = de;
		}
		return output;
	};
	auto draw_triangle_raw = [&](const vec4& c0, const vec4& c1, const vec4& c2, const Color& clr) {
		if (c0[3] == 0 || c1[3] == 0 || c2[3] == 0)
			return;
		if (wireframe_only) {
			vec4 e0 = c0, e1 = c1;
			if (ClipLineClipSpace(e0, e1) && e0[3] != 0 && e1[3] != 0) {
				vec3 n0 = e0.Splice() / e0[3];
				vec3 n1 = e1.Splice() / e1[3];
				draw_line_ndc(n0, n1, clr);
			}
			e0 = c1; e1 = c2;
			if (ClipLineClipSpace(e0, e1) && e0[3] != 0 && e1[3] != 0) {
				vec3 n0 = e0.Splice() / e0[3];
				vec3 n1 = e1.Splice() / e1[3];
				draw_line_ndc(n0, n1, clr);
			}
			e0 = c2; e1 = c0;
			if (ClipLineClipSpace(e0, e1) && e0[3] != 0 && e1[3] != 0) {
				vec3 n0 = e0.Splice() / e0[3];
				vec3 n1 = e1.Splice() / e1[3];
				draw_line_ndc(n0, n1, clr);
			}
			return;
		}
		vec3 n0 = c0.Splice() / c0[3];
		vec3 n1 = c1.Splice() / c1[3];
		vec3 n2 = c2.Splice() / c2[3];
		if (n0[2] < -1 && n1[2] < -1 && n2[2] < -1)
			return;
		if (n0[2] > 1 && n1[2] > 1 && n2[2] > 1)
			return;
		TriProj tp[3];
		vec3 ndc[3] = {n0, n1, n2};
		for (int i = 0; i < 3; i++) {
			tp[i].p[0] = (ndc[i][0] + 1.0f) * 0.5f * (float)(sz.cx - 1);
			tp[i].p[1] = (-ndc[i][1] + 1.0f) * 0.5f * (float)(sz.cy - 1);
			tp[i].z = ndc[i][2];
		}
		float area = Edge2D(tp[0].p, tp[1].p, tp[2].p);
		if (area == 0)
			return;
		int minx = (int)floor(min(tp[0].p[0], min(tp[1].p[0], tp[2].p[0])));
		int maxx = (int)ceil(max(tp[0].p[0], max(tp[1].p[0], tp[2].p[0])));
		int miny = (int)floor(min(tp[0].p[1], min(tp[1].p[1], tp[2].p[1])));
		int maxy = (int)ceil(max(tp[0].p[1], max(tp[1].p[1], tp[2].p[1])));
		minx = max(minx, 0);
		miny = max(miny, 0);
		maxx = min(maxx, sz.cx - 1);
		maxy = min(maxy, sz.cy - 1);
		for (int y = miny; y <= maxy; y++) {
			for (int x = minx; x <= maxx; x++) {
				vec2 p((float)x + 0.5f, (float)y + 0.5f);
				float w0 = Edge2D(tp[1].p, tp[2].p, p);
				float w1 = Edge2D(tp[2].p, tp[0].p, p);
				float w2 = Edge2D(tp[0].p, tp[1].p, p);
				if ((area > 0 && (w0 < 0 || w1 < 0 || w2 < 0)) ||
				    (area < 0 && (w0 > 0 || w1 > 0 || w2 > 0)))
					continue;
				w0 /= area;
				w1 /= area;
				w2 /= area;
				float z = w0 * tp[0].z + w1 * tp[1].z + w2 * tp[2].z;
				surf.SetPixel(x, y, z, clr);
			}
		}
	};
	auto draw_triangle = [&](const vec4& c0, const vec4& c1, const vec4& c2, const Color& clr) {
		Vector<vec4> poly;
		poly.Add(c0);
		poly.Add(c1);
		poly.Add(c2);
		poly = clip_poly(poly, true);
		if (poly.GetCount() < 3)
			return;
		poly = clip_poly(poly, false);
		if (poly.GetCount() < 3)
			return;
		for (int i = 1; i + 1 < poly.GetCount(); i++)
			draw_triangle_raw(poly[0], poly[i], poly[i + 1], clr);
	};
	
	int models_painted = 0;
	int triangles_submitted = 0;
	bool debug_captured = false;
	String debug_dump;
	Vector<UiSceneLight> ui_lights;
	CollectUiLights(&ctx, ui_lights);
	auto paint_model = [&](const GeomObjectState& os, const Model& mdl) {
		models_painted++;
		mat4 o_world = Translate(os.position) * QuatMat(os.orientation) * Scale(os.scale);
		mat4 o_view = view * o_world;
		for (const Mesh& mesh : mdl.meshes) {
			vec3 base_clr(1, 1, 1);
			vec3 emissive(0, 0, 0);
			vec3 diffuse(1, 1, 1);
			vec3 specular(0.2f, 0.2f, 0.2f);
			vec3 ambient(0.1f, 0.1f, 0.1f);
			float shininess = 16.0f;
			if (mesh.material >= 0 && mdl.materials.Find(mesh.material) >= 0) {
				const Material& mat = mdl.materials.Get(mesh.material);
				base_clr = mat.params->base_clr_factor.Splice();
				emissive = mat.params->emissive_factor;
				diffuse = mat.params->diffuse;
				specular = mat.params->specular;
				ambient = mat.params->ambient;
				shininess = mat.params->shininess;
				if (diffuse.GetLength() == 0)
					diffuse = vec3(1, 1, 1);
				if (specular.GetLength() == 0)
					specular = vec3(0.2f, 0.2f, 0.2f);
				if (ambient.GetLength() == 0)
					ambient = vec3(0.1f, 0.1f, 0.1f);
				if (shininess <= 0)
					shininess = 16.0f;
			}
			vec3 base = base_clr * diffuse;
			const auto* tri_idx = mesh.indices.Begin();
			int tri_count = mesh.indices.GetCount() / 3;
			for (int i = 0; i < tri_count; i++) {
				const Vertex& v0 = mesh.vertices[tri_idx[0]];
				const Vertex& v1 = mesh.vertices[tri_idx[1]];
				const Vertex& v2 = mesh.vertices[tri_idx[2]];
				vec3 n = Cross(v1.position.Splice() - v0.position.Splice(),
				               v2.position.Splice() - v0.position.Splice());
				if (n.GetLength() == 0) {
					tri_idx += 3;
					continue;
				}
				n.Normalize();
				vec3 n_world = VectorTransform(n, os.orientation);
				vec3 p0 = (o_world * v0.position).Splice();
				vec3 p1 = (o_world * v1.position).Splice();
				vec3 p2 = (o_world * v2.position).Splice();
				vec3 center = (p0 + p1 + p2) / 3.0f;
				vec3 view_dir = camera.position - center;
				if (view_dir.GetLength() > 0)
					view_dir.Normalize();
				UiLightEval ev = EvaluateUiLighting(ui_lights, center, n_world, view_dir, shininess);
				vec3 shaded = base * (ev.ambient + ev.diffuse * 0.75f)
				            + base_clr * ambient + specular * ev.specular + emissive;
				shaded[0] = Clamp(shaded[0], 0.0f, 1.0f);
				shaded[1] = Clamp(shaded[1], 0.0f, 1.0f);
				shaded[2] = Clamp(shaded[2], 0.0f, 1.0f);
				Color clr((int)Clamp(shaded[0] * 255.0f, 0.0f, 255.0f),
				          (int)Clamp(shaded[1] * 255.0f, 0.0f, 255.0f),
				          (int)Clamp(shaded[2] * 255.0f, 0.0f, 255.0f));
				vec4 c0 = o_view * v0.position;
				vec4 c1 = o_view * v1.position;
				vec4 c2 = o_view * v2.position;
				if (dump_first_tri && !debug_captured) {
					debug_captured = true;
					auto dump_vec4 = [&](const char* name, const vec4& v) {
						auto dump_val = [&](double val) {
							if (IsNull(val))
								debug_dump << "null";
							else
								debug_dump << val;
						};
						debug_dump << name << "=(";
						dump_val(v[0]); debug_dump << ",";
						dump_val(v[1]); debug_dump << ",";
						dump_val(v[2]); debug_dump << ",";
						dump_val(v[3]); debug_dump << ")\n";
					};
					auto dump_vec3 = [&](const char* name, const vec3& v) {
						auto dump_val = [&](double val) {
							if (IsNull(val))
								debug_dump << "null";
							else
								debug_dump << val;
						};
						debug_dump << name << "=(";
						dump_val(v[0]); debug_dump << ",";
						dump_val(v[1]); debug_dump << ",";
						dump_val(v[2]); debug_dump << ")\n";
					};
					debug_dump << "first_tri\n";
					dump_vec3("cam_pos", camera.position);
					auto dump_quat = [&](const char* name, const quat& q) {
						vec4 v(q[0], q[1], q[2], q[3]);
						dump_vec4(name, v);
					};
					dump_quat("cam_ori", camera.orientation);
					auto dump_mat4 = [&](const char* name, const mat4& m) {
						debug_dump << name << "=[";
						for (int r = 0; r < 4; r++) {
							for (int c = 0; c < 4; c++) {
								double val = m[r][c];
								if (IsNull(val))
									debug_dump << "null";
								else
									debug_dump << val;
								if (!(r == 3 && c == 3))
									debug_dump << ",";
							}
						}
						debug_dump << "]\n";
					};
					dump_mat4("view", view);
					dump_mat4("o_world", o_world);
					dump_mat4("o_view", o_view);
					dump_vec4("v0", v0.position);
					dump_vec4("v1", v1.position);
					dump_vec4("v2", v2.position);
					dump_vec4("c0", c0);
					dump_vec4("c1", c1);
					dump_vec4("c2", c2);
					if (c0[3] != 0 && c1[3] != 0 && c2[3] != 0) {
						dump_vec3("n0", c0.Splice() / c0[3]);
						dump_vec3("n1", c1.Splice() / c1[3]);
						dump_vec3("n2", c2.Splice() / c2[3]);
					}
				}
				draw_triangle(c0, c1, c2, clr);
				triangles_submitted++;
				tri_idx += 3;
			}
		}
	};
	
	if (ctx.anim && ctx.anim->is_playing) {
		for (GeomObjectState& os : state.objs) {
			GeomObject& go = *os.obj;
			if (!go.is_visible)
				continue;
			if (go.IsModel() && go.mdl)
				paint_model(os, *go.mdl);
		}
	}
	else {
		GeomObjectCollection iter(scene);
		GeomObjectState os;
		for (GeomObject& go : iter) {
			if (!go.is_visible)
				continue;
			if (go.IsModel() && go.mdl) {
				os.obj = &go;
				os.position = vec3(0);
				os.orientation = Identity<quat>();
				os.scale = vec3(1);
				if (GeomTransform* tr = go.FindTransform()) {
					os.position = tr->position;
					os.orientation = tr->orientation;
					os.scale = tr->scale;
				}
				paint_model(os, *go.mdl);
			}
		}
	}
	
	if (out_stats) {
		out_stats->models = models_painted;
		out_stats->triangles = triangles_submitted;
		out_stats->pixels = surf.pixels_written;
		out_stats->rendered = surf.pixels_written > 0;
	}
	if (out_debug && dump_first_tri && debug_captured)
		*out_debug = debug_dump;
	if (out_image)
		*out_image = Image(surf.ib);
	return true;
}

void EditRendererV2_Ogl::Paint(Draw& d) {
	if (!ctx || !ctx->conf || !ctx->state)
		return;
	Size sz = GetSize();
	if (sz.cx <= 0 || sz.cy <= 0)
		return;
	// See GLCtrl::IsGLWindowCreated's own comment: this can be painted
	// before its own GL drawable exists yet when opened as a second
	// top-level window from an already-running app - ExecuteGL below
	// would otherwise crash the process with a fatal X GLXBadDrawable
	// error, not a catchable C++ exception.
	if (!IsGLWindowCreated())
		return;
	Image img;
	Scene3DRenderStats stats;
	GeomCamera& camera = GetGeomCamera();
	RenderSceneV2Headless(*ctx, sz, &stats, &img, nullptr, false, view_mode, &camera, wireframe_only);
	if (img.IsEmpty())
		return;
	ExecuteGL([&] {
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_BLEND);
		glViewport(0, 0, (GLsizei)sz.cx, (GLsizei)sz.cy);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0, sz.cx, 0, sz.cy, -1, 1);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glRasterPos2i(0, sz.cy);
		glPixelZoom(1, -1);
		ImageBuffer ib(img);
		glDrawPixels(sz.cx, sz.cy, GL_RGBA, GL_UNSIGNED_BYTE, ib.Begin());
		glPixelZoom(1, 1);
		glFlush();
	}, true);
}

void EditRendererBase::LeftDown(Point p, dword keyflags) {
	GeomCamera& camera = GetGeomCamera();
	if (WhenInput)
		WhenInput("mouseDown", p, keyflags, 0);
	if (!camera_input_enabled) {
		SetFocus();
		return;
	}
	
	cap_mouse_pos = p;
	is_captured_mouse = true;
	cap_begin_pos = camera.position;
	cap_begin_orientation = camera.orientation;
	
	bool is_shift = keyflags & K_SHIFT;
	bool is_ctrl = keyflags & K_CTRL;
	if (is_shift)
		cap_mode = CAPMODE_MOVE_YZ;
	else if (is_ctrl)
		cap_mode = CAPMODE_ROTATE;
	else
		cap_mode = CAPMODE_MOVE_XY;
	
	SetCapture();
	
	SetFocus();
}

void EditRendererBase::LeftUp(Point p, dword keyflags) {
	if (WhenInput)
		WhenInput("mouseUp", p, keyflags, 0);
	if (!camera_input_enabled) {
		is_captured_mouse = false;
		ReleaseCapture();
		return;
	}
	is_captured_mouse = false;
	
	ReleaseCapture();
}

void EditRendererBase::MouseMove(Point p, dword keyflags) {
	GeomCamera& camera = GetGeomCamera();
	if (WhenInput)
		WhenInput("mouseMove", p, keyflags, 0);
	if (!camera_input_enabled)
		return;
	
	if (is_captured_mouse) {
		Point diff = p - cap_mouse_pos;
		if (!ctx || !ctx->conf)
			return;
		float s = ctx->conf->mouse_move_sensitivity * camera.scale;
		switch (cap_mode) {
			case CAPMODE_MOVE_XY: MoveRel(vec3(-diff.x * s, diff.y * s, 0)); break;
			case CAPMODE_MOVE_YZ: MoveRel(vec3(-diff.x * s, 0, -diff.y * s * SCALAR_FWD_Z)); break;
			case CAPMODE_ROTATE: RotateRel(vec3(-diff.x * s, -diff.y * s, 0)); break;
			default: break;
		}
	}
}

void EditRendererBase::Move(const vec3& v) {
	GeomCamera& camera = GetGeomCamera();
	
	switch (view_mode) {
		
	case VIEWMODE_YZ:
		camera.position += VecMul(YRotation(M_PI/2), v);
		break;
		
	case VIEWMODE_XZ:
		camera.position += VecMul(XRotation(-M_PI/2), v);
		break;
		
	case VIEWMODE_XY:
		camera.position += VecMul(YRotation(0), v);
		break;
		
	case VIEWMODE_PERSPECTIVE:
		camera.position += VecMul(QuatMat(camera.orientation), v);
		break;
		
	}
	
	WhenChanged();
}

void EditRendererBase::MoveRel(const vec3& v) {
	GeomCamera& camera = GetGeomCamera();
	
	switch (view_mode) {
		
	case VIEWMODE_YZ:
		camera.position = cap_begin_pos + VecMul(YRotation(M_PI/2), v);
		break;
		
	case VIEWMODE_XZ:
		camera.position = cap_begin_pos + VecMul(XRotation(-M_PI/2), v);
		break;
		
	case VIEWMODE_XY:
		camera.position = cap_begin_pos + VecMul(YRotation(0), v);
		break;
		
	case VIEWMODE_PERSPECTIVE:
		camera.position = cap_begin_pos + VecMul(QuatMat(camera.orientation), v);
		break;
		
	}
	
	WhenChanged();
}

void EditRendererBase::Rotate(const axes3& v) {
	GeomCamera& camera = GetGeomCamera();
	
	camera.orientation = MatQuat(QuatMat(camera.orientation) * AxesMat(v));
	
	WhenChanged();
}

void EditRendererBase::RotateRel(const axes3& v) {
	GeomCamera& camera = GetGeomCamera();
	
	camera.orientation = MatQuat(QuatMat(cap_begin_orientation) * AxesMat(v));
	
	WhenChanged();
}

void EditRendererBase::MouseWheel(Point p, int zdelta, dword keyflags) {
	if (WhenInput)
		WhenInput("mouseWheel", p, keyflags, zdelta);
	if (!camera_input_enabled)
		return;
	const double scale = 0.75;
	GeomCamera& camera = GetGeomCamera();
	
	if (zdelta < 0) {
		camera.scale /= scale;
	}
	else {
		camera.scale *= scale;
	}
	
	WhenChanged();
}

void EditRendererBase::RightDown(Point p, dword keyflags) {
	if (WhenInput)
		WhenInput("mouseRightDown", p, keyflags, 0);
	if (WhenMenu)
		MenuBar::Execute(WhenMenu, GetMousePos());
}

GeomCamera& EditRendererBase::GetGeomCamera() const {
	switch (cam_src) {
	case CAMSRC_OBJECT:
		if (ctx && ctx->state) {
			GeomCamera& base = ctx->state->GetProgram();
			cam_override.position = base.position;
			cam_override.orientation = base.orientation;
			cam_override.distance = base.distance;
			cam_override.fov = base.fov;
			cam_override.scale = base.scale;
			GeomObject* obj = ctx->state->FindObjectByKey(cam_object_key);
			if (obj) {
				if (GeomTransform* tr = obj->FindTransform()) {
					cam_override.position = tr->position;
					cam_override.orientation = tr->orientation;
				}
				else if (const GeomObjectState* os = ctx->state->FindObjectStateByKey(obj->key)) {
					cam_override.position = os->position;
					cam_override.orientation = os->orientation;
				}
			}
			return cam_override;
		}
		break;
		
	case CAMSRC_FOCUS:
	case CAMSRC_VIDEOIMPORT_FOCUS:
		ASSERT(ctx && ctx->state);
		return ctx->state->GetFocus();
		break;
		
	case CAMSRC_PROGRAM:
	case CAMSRC_VIDEOIMPORT_PROGRAM:
		ASSERT(ctx && ctx->state);
		return ctx->state->GetProgram();
		break;
		
	}
	if (ctx && ctx->state)
		return ctx->state->GetProgram();
	Panic("Invalid view mode in EditRendererBase");
	NEVER();
	return cam_override;
}

bool EditRendererBase::Key(dword key, int count) {
	bool is_release = key & K_KEYUP;
	if (WhenInput)
		WhenInput(is_release ? "keyUp" : "keyDown", Point(0, 0), 0, (int)key);
	if (!camera_input_enabled)
		return false;
	GeomCamera& camera = GetGeomCamera();
	float step = camera.scale * 0.1;
	
	bool is_shift = key & K_SHIFT;
	bool is_ctrl = key & K_CTRL;
	key &= 0xFFFF | K_DELTA;
	
	if (is_shift) {
		if (key == K_LEFT) {
			Move(VEC_LEFT * step);
			return true;
		}
		else if (key == K_RIGHT) {
			Move(VEC_RIGHT * step);
			return true;
		}
		else if (key == K_UP) {
			Move(VEC_FWD * step);
			return true;
		}
		else if (key == K_DOWN) {
			Move(VEC_BWD * step);
			return true;
		}
	}
	else if (is_ctrl) {
		step = 0.1;
		if (key == K_LEFT) {
			Rotate(VEC_ROT_LEFT * step);
			return true;
		}
		else if (key == K_RIGHT) {
			Rotate(VEC_ROT_RIGHT * step);
			return true;
		}
		else if (key == K_UP) {
			Rotate(VEC_ROT_UP * step);
			return true;
		}
		else if (key == K_DOWN) {
			Rotate(VEC_ROT_DOWN * step);
			return true;
		}
	}
	else {
		if (key == K_LEFT) {
			Move(VEC_LEFT * step);
			return true;
		}
		else if (key == K_RIGHT) {
			Move(VEC_RIGHT * step);
			return true;
		}
		else if (key == K_UP) {
			Move(VEC_UP * step);
			return true;
		}
		else if (key == K_DOWN) {
			Move(VEC_DOWN * step);
			return true;
		}
	}
	
	return false;
}


END_UPP_NAMESPACE

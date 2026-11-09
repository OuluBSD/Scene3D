#include "Core.h"

NAMESPACE_UPP

void GeomScript__initializer();

static void EnsureGeomScriptRegistered() {
	GeomScript__initializer();
}


static bool IsGeomDirectoryType(const VfsValue& v) {
	return IsVfsType(v, AsTypeHash<GeomDirectory>()) ||
	       IsVfsType(v, AsTypeHash<GeomScene>());
}

static void ApplyMeshAnimation(GeomObject& obj, int position, double time, int kps);
static void Apply2DAnimation(GeomObject& obj, int position, double time, int kps);

GeomObjectIterator::GeomObjectIterator(GeomDirectory& d) {
	addr[0] = &d.val;
	pos[0] = -1;
	level = 0;
}

GeomObjectCollection::GeomObjectCollection(GeomDirectory& d) {
	iter = GeomObjectIterator(d);
	iter.Next();
}

bool GeomObjectIterator::Next() {
	obj = 0;
	while (level >= 0) {
		ASSERT(addr[level]);
		VfsValue& node = *addr[level];
		int& p = pos[level];
		while (++p < node.sub.GetCount()) {
			VfsValue& child = node.sub[p];
			if (IsVfsType(child, AsTypeHash<GeomObject>())) {
				obj = &child.GetExt<GeomObject>();
				return true;
			}
			if (IsGeomDirectoryType(child)) {
				level++;
				ASSERT(level < MAX_LEVELS);
				addr[level] = &child;
				pos[level] = -1;
				break;
			}
		}
		if (p >= node.sub.GetCount())
			level--;
	}
	return false;
}

GeomObjectIterator::operator bool() const {
	return obj != 0;
}

GeomObject& GeomObjectIterator::operator*() {
	ASSERT(obj);
	return *obj;
}

GeomObject* GeomObjectIterator::operator->() {
	return obj;
}

void GeomKeypoint::Visit(Vis& v) {
	v VIS_(frame_id)
	  VISN(position)
	  VISN(orientation)
	  VIS_(has_position)
	  VIS_(has_orientation)
	  VIS_(position_ease)
	  VIS_(orientation_ease);
}

void GeomTimeline::Visit(Vis& v) {
	v VISM(keypoints);
}

void GeomSceneTimeline::Reset() {
	is_playing = false;
	position = 0;
	time = 0;
}

void GeomSceneTimeline::Play(int scene_length) {
	if (scene_length > 0)
		length = scene_length;
	if (length > 0 && (position < 0 || position >= length))
		Reset();
	is_playing = true;
}

void GeomSceneTimeline::Pause() {
	is_playing = false;
}

void GeomSceneTimeline::Update(GeomWorldState& state, double dt) {
	if (!is_playing || !state.prj)
		return;
	if (length <= 0)
		length = state.GetActiveScene().length;
	if (length <= 0)
		return;
	time += dt * speed;
	double frame_time = 1.0 / state.prj->kps;
	position = time / frame_time;
	if (position < 0 || position >= length) {
		if (repeat) {
			Reset();
			is_playing = true;
		}
		else {
			is_playing = false;
		}
		return;
	}
	ApplyAtPosition(state, position, time);
}

void GeomSceneTimeline::ApplyAtPosition(GeomWorldState& state, int pos, double t) {
	if (!state.prj)
		return;
	GeomProject& prj = *state.prj;
	bool has_solo = false;
	for (int i = 0; i < state.objs.GetCount(); i++) {
		if (state.objs[i].obj && state.objs[i].obj->timeline_solo)
			has_solo = true;
	}
	auto apply_ease = [](int mode, float f) {
		if (mode == 1) // step
			return 0.0f;
		if (mode == 2) { // smooth
			return f * f * (3.0f - 2.0f * f);
		}
		return f;
	};
	for (GeomObjectState& os : state.objs) {
		GeomObject& o = *os.obj;
		if (!o.read_enabled)
			continue;
		if (o.timeline_muted)
			continue;
		if (has_solo && !o.timeline_solo)
			continue;
		GeomTimeline* tl = o.FindTimeline();
		if (tl && !tl->keypoints.IsEmpty()) {
			int pre_i = tl->FindPrePosition(pos);
			int post_i = tl->FindPostPosition(pos);
			if (pre_i >= 0 && post_i >= 0) {
				ASSERT(pre_i < post_i);
				GeomKeypoint& pre = tl->keypoints[pre_i];
				GeomKeypoint& post = tl->keypoints[post_i];
				float pre_time = pre.frame_id / (float)prj.kps;
				float post_time = post.frame_id / (float)prj.kps;
				float f = (post_time > pre_time) ? (t - pre_time) / (post_time - pre_time) : 0.0f;
				f = min(max(f, 0.0f), 1.0f);
				f = apply_ease(pre.position_ease, f);
				os.position = Lerp(pre.position, post.position, f);
			}
			else if (pre_i >= 0) {
				GeomKeypoint& pre = tl->keypoints[pre_i];
				os.position = pre.position;
			}
			else if (post_i >= 0) {
				GeomKeypoint& post = tl->keypoints[post_i];
				os.position = post.position;
			}

			pre_i = tl->FindPreOrientation(pos);
			post_i = tl->FindPostOrientation(pos);
			if (pre_i >= 0 && post_i >= 0) {
				ASSERT(pre_i < post_i);
				GeomKeypoint& pre = tl->keypoints[pre_i];
				GeomKeypoint& post = tl->keypoints[post_i];
				float pre_time = pre.frame_id / (float)prj.kps;
				float post_time = post.frame_id / (float)prj.kps;
				float f = (post_time > pre_time) ? (t - pre_time) / (post_time - pre_time) : 0.0f;
				f = min(max(f, 0.0f), 1.0f);
				f = apply_ease(pre.orientation_ease, f);
				os.orientation = Slerp(pre.orientation, post.orientation, f);
			}
			else if (pre_i >= 0) {
				GeomKeypoint& pre = tl->keypoints[pre_i];
				os.orientation = pre.orientation;
			}
			else if (post_i >= 0) {
				GeomKeypoint& post = tl->keypoints[post_i];
				os.orientation = post.orientation;
			}
		}
		Apply2DAnimation(o, pos, t, prj.kps);
		ApplyMeshAnimation(o, pos, t, prj.kps);
	}
	if (state.active_camera_obj_i >= 0) {
		GeomObjectState& os = state.objs[state.active_camera_obj_i];
		ASSERT(os.obj->IsCamera());
		if (os.obj->read_enabled) {
			GeomCamera& cam = state.GetProgram();
			cam.position = os.position;
			cam.orientation = os.orientation;
		}
	}
}

void GeomSceneTimeline::Visit(Vis& v) {
	v VIS_(time)
	  VIS_(position)
	  VIS_(length)
	  VIS_(is_playing)
	  VIS_(repeat)
	  VIS_(speed);
}

void GeomTransform::Visit(Vis& v) {
	v VISN(position)
	  VISN(orientation)
	  VISN(scale);
}

void GeomScript::Visit(Vis& v) {
	v VIS_(file)
	  VIS_(enabled)
	  VIS_(run_on_load)
	  VIS_(run_every_frame);
}

void GeomDynamicProperties::Visit(Vis& v) {
	if (v.mode == Vis::MODE_JSON) {
		if (v.IsLoading()) {
			props.Clear();
			const Value& prop_va = v.json->Get()["entries"];
			for (int i = 0; i < prop_va.GetCount(); i++) {
				String key;
				LoadFromJsonValue(key, prop_va[i]["key"]);
				Value val = prop_va[i]["value"];
				props.Add(key, val);
			}
		}
		else {
			Vector<Value> prop_values;
			for (int i = 0; i < props.GetCount(); i++) {
				ValueMap item;
				item.Add("key", StoreAsJsonValue(props.GetKey(i)));
				item.Add("value", props[i]);
				prop_values.Add(item);
			}
			v.json->Set("entries", ValueArray(pick(prop_values)));
		}
	}
	else {
		int prop_count = props.GetCount();
		v VIS_(prop_count);
		if (v.IsLoading()) {
			props.Clear();
			for (int i = 0; i < prop_count; i++) {
				String key;
				Value val;
				v VIS_(key)
				  VIS_(val);
				props.Add(key, val);
			}
		}
		else {
			for (int i = 0; i < prop_count; i++) {
				String key = props.GetKey(i);
				Value val = props[i];
				v VIS_(key)
				  VIS_(val);
			}
		}
	}
}

void GeomEdge::Visit(Vis& v) {
	v VIS_(a)
	  VIS_(b);
}

void GeomFace::Visit(Vis& v) {
	v VIS_(a)
	  VIS_(b)
	  VIS_(c);
}

static Value Vec3ToJsonValue(const vec3& p) {
	ValueArray a;
	a.Add(p[0]);
	a.Add(p[1]);
	a.Add(p[2]);
	return a;
}

static vec3 JsonValueToVec3(const Value& v) {
	vec3 out(0);
	if (v.GetCount() >= 3) {
		out[0] = v[0];
		out[1] = v[1];
		out[2] = v[2];
	}
	return out;
}

static Value Vec2ToJsonValue(const vec2& p) {
	ValueArray a;
	a.Add(p[0]);
	a.Add(p[1]);
	return a;
}

static vec2 JsonValueToVec2(const Value& v) {
	vec2 out(0);
	if (v.GetCount() >= 2) {
		out[0] = v[0];
		out[1] = v[1];
	}
	return out;
}

void GeomEditableMesh::Visit(Vis& v) {
	if (v.mode == Vis::MODE_JSON) {
		if (v.IsLoading()) {
			points.Clear();
			lines.Clear();
			faces.Clear();
			const Value& pts = v.json->Get()["points"];
			for (int i = 0; i < pts.GetCount(); i++)
				points.Add(JsonValueToVec3(pts[i]));
			const Value& ln = v.json->Get()["lines"];
			for (int i = 0; i < ln.GetCount(); i++) {
				GeomEdge e;
				e.a = (int)ln[i]["a"];
				e.b = (int)ln[i]["b"];
				lines.Add(e);
			}
			const Value& fc = v.json->Get()["faces"];
			for (int i = 0; i < fc.GetCount(); i++) {
				GeomFace f;
				f.a = (int)fc[i]["a"];
				f.b = (int)fc[i]["b"];
				f.c = (int)fc[i]["c"];
				faces.Add(f);
			}
		}
		else {
			Vector<Value> pts;
			for (const vec3& p : points)
				pts.Add(Vec3ToJsonValue(p));
			Vector<Value> ln;
			for (const GeomEdge& e : lines) {
				ValueMap item;
				item.Add("a", e.a);
				item.Add("b", e.b);
				ln.Add(item);
			}
			Vector<Value> fc;
			for (const GeomFace& f : faces) {
				ValueMap item;
				item.Add("a", f.a);
				item.Add("b", f.b);
				item.Add("c", f.c);
				fc.Add(item);
			}
			v.json->Set("points", ValueArray(pick(pts)));
			v.json->Set("lines", ValueArray(pick(ln)));
			v.json->Set("faces", ValueArray(pick(fc)));
		}
	}
	else {
		int point_count = points.GetCount();
		int line_count = lines.GetCount();
		int face_count = faces.GetCount();
		v VIS_(point_count)
		  VIS_(line_count)
		  VIS_(face_count);
		if (v.IsLoading()) {
			points.SetCount(point_count);
			lines.SetCount(line_count);
			faces.SetCount(face_count);
			for (int i = 0; i < point_count; i++)
				v VISN(points[i]);
			for (int i = 0; i < line_count; i++)
				lines[i].Visit(v);
			for (int i = 0; i < face_count; i++)
				faces[i].Visit(v);
		}
		else {
			for (int i = 0; i < point_count; i++)
				v VISN(points[i]);
			for (int i = 0; i < line_count; i++)
				lines[i].Visit(v);
			for (int i = 0; i < face_count; i++)
				faces[i].Visit(v);
		}
	}
}

void Geom2DShape::Visit(Vis& v) {
	int type_i = (int)type;
	if (v.mode == Vis::MODE_JSON) {
		if (v.IsLoading()) {
			points.Clear();
			type_i = (int)v.json->Get()["type"];
			radius = (float)v.json->Get()["radius"];
			width = (float)v.json->Get()["width"];
			stroke_cap = (int)v.json->Get()["stroke_cap"];
			stroke_join = (int)v.json->Get()["stroke_join"];
			closed = (bool)v.json->Get()["closed"];
			int r = (int)v.json->Get()["stroke_r"];
			int g = (int)v.json->Get()["stroke_g"];
			int b = (int)v.json->Get()["stroke_b"];
			stroke = Color(r, g, b);
			const Value& pts = v.json->Get()["points"];
			for (int i = 0; i < pts.GetCount(); i++)
				points.Add(JsonValueToVec2(pts[i]));
		}
		else {
			Vector<Value> pts;
			for (const vec2& p : points)
				pts.Add(Vec2ToJsonValue(p));
			v.json->Set("type", (int)type);
			v.json->Set("radius", radius);
			v.json->Set("width", width);
			v.json->Set("stroke_cap", stroke_cap);
			v.json->Set("stroke_join", stroke_join);
			v.json->Set("closed", closed);
			v.json->Set("stroke_r", (int)stroke.GetR());
			v.json->Set("stroke_g", (int)stroke.GetG());
			v.json->Set("stroke_b", (int)stroke.GetB());
			v.json->Set("points", ValueArray(pick(pts)));
		}
	}
	else {
		v VIS_(type_i)
		  VIS_(radius)
		  VIS_(width)
		  VIS_(stroke_cap)
		  VIS_(stroke_join)
		  VIS_(closed)
		  VIS_(tex_wrap)
		  VIS_(stroke_uv_mode)
		  VIS_(tex_repeat_x)
		  VIS_(tex_repeat_y);
		int r = stroke.GetR();
		int g = stroke.GetG();
		int b = stroke.GetB();
		v VIS_(r)
		  VIS_(g)
		  VIS_(b);
		if (v.IsLoading())
			stroke = Color(r, g, b);
		int count = points.GetCount();
		v VIS_(count);
		if (v.IsLoading()) {
			points.SetCount(count);
			for (int i = 0; i < count; i++)
				v VISN(points[i]);
		}
		else {
			for (int i = 0; i < count; i++)
				v VISN(points[i]);
		}
	}
	type = (Geom2DShape::Type)type_i;
}

void Geom2DLayer::Visit(Vis& v) {
	v VIS_(visible)
	  VIS_(stroke)
	  VIS_(fill)
	  VIS_(width)
	  VIS_(opacity)
	  VIS_(use_layer_style)
	  VIS_(stroke_cap)
	  VIS_(stroke_join)
	  VIS_(texture_ref)
	  VIS_(blend_mode)
	  VIS_(tex_offset_x)
	  VIS_(tex_offset_y)
	  VIS_(tex_repeat_x)
	  VIS_(tex_repeat_y)
	  VIS_(tex_rotate)
	  VIS_(tex_wrap)
	  VIS_(stroke_uv_mode);
	if (v.mode == Vis::MODE_JSON) {
		if (v.IsLoading()) {
			shapes.Clear();
			const Value& arr = v.json->Get()["shapes"];
			for (int i = 0; i < arr.GetCount(); i++) {
				Geom2DShape shape;
				JsonIO jio(arr[i]);
				Vis vis(jio);
				shape.Visit(vis);
				shapes.Add(pick(shape));
			}
		}
		else {
			Vector<Value> arr;
			for (Geom2DShape& shape : shapes)
				arr.Add(v.VisitAsJsonValue(shape));
			v.json->Set("shapes", ValueArray(pick(arr)));
		}
	}
	else {
		int count = shapes.GetCount();
		v VIS_(count);
		if (v.IsLoading()) {
			shapes.SetCount(count);
			for (int i = 0; i < count; i++)
				shapes[i].Visit(v);
		}
		else {
			for (int i = 0; i < count; i++)
				shapes[i].Visit(v);
		}
	}
}

void Geom2DKeyframe::Visit(Vis& v) {
	v VIS_(frame_id);
	if (v.mode == Vis::MODE_JSON) {
		if (v.IsLoading()) {
			shapes.Clear();
			const Value& arr = v.json->Get()["shapes"];
			for (int i = 0; i < arr.GetCount(); i++) {
				Geom2DShape shape;
				JsonIO jio(arr[i]);
				Vis vis(jio);
				shape.Visit(vis);
				shapes.Add(pick(shape));
			}
		}
		else {
			Vector<Value> arr;
			for (Geom2DShape& shape : shapes)
				arr.Add(v.VisitAsJsonValue(shape));
			v.json->Set("shapes", ValueArray(pick(arr)));
		}
	}
	else {
		int count = shapes.GetCount();
		v VIS_(count);
		if (v.IsLoading()) {
			shapes.SetCount(count);
			for (int i = 0; i < count; i++)
				shapes[i].Visit(v);
		}
		else {
			for (int i = 0; i < count; i++)
				shapes[i].Visit(v);
		}
	}
}

Geom2DKeyframe& Geom2DAnimation::GetAddKeyframe(int frame) {
	int i = keyframes.Find(frame);
	if (i >= 0)
		return keyframes[i];
	Geom2DKeyframe& kf = keyframes.Add(frame);
	kf.frame_id = frame;
	return kf;
}

int Geom2DAnimation::FindPre(int frame) const {
	for (int i = keyframes.GetCount() - 1; i >= 0; i--) {
		int j = keyframes.GetKey(i);
		if (j <= frame)
			return i;
	}
	return -1;
}

int Geom2DAnimation::FindPost(int frame) const {
	for (int i = 0; i < keyframes.GetCount(); i++) {
		int j = keyframes.GetKey(i);
		if (j >= frame)
			return i;
	}
	return -1;
}

void Geom2DAnimation::Visit(Vis& v) {
	v VISM(keyframes);
}

void GeomTextureEdit::Visit(Vis& v) {
	v VIS_(path)
	  VIS_(width)
	  VIS_(height);
}

void GeomMeshKeyframe::Visit(Vis& v) {
	v VIS_(frame_id);
	if (v.mode == Vis::MODE_JSON) {
		if (v.IsLoading()) {
			points.Clear();
			const Value& pts = v.json->Get()["points"];
			for (int i = 0; i < pts.GetCount(); i++)
				points.Add(JsonValueToVec3(pts[i]));
		}
		else {
			Vector<Value> pts;
			for (const vec3& p : points)
				pts.Add(Vec3ToJsonValue(p));
			v.json->Set("points", ValueArray(pick(pts)));
		}
	}
	else {
		int point_count = points.GetCount();
		v VIS_(point_count);
		if (v.IsLoading()) {
			points.SetCount(point_count);
			for (int i = 0; i < point_count; i++)
				v VISN(points[i]);
		}
		else {
			for (int i = 0; i < point_count; i++)
				v VISN(points[i]);
		}
	}
}

GeomMeshKeyframe& GeomMeshAnimation::GetAddKeyframe(int frame) {
	int i = keyframes.Find(frame);
	if (i >= 0)
		return keyframes[i];
	GeomMeshKeyframe& kf = keyframes.Add(frame);
	kf.frame_id = frame;
	return kf;
}

int GeomMeshAnimation::FindPre(int frame) const {
	for (int i = keyframes.GetCount() - 1; i >= 0; i--) {
		int j = keyframes.GetKey(i);
		if (j <= frame)
			return i;
	}
	return -1;
}

int GeomMeshAnimation::FindPost(int frame) const {
	for (int i = 0; i < keyframes.GetCount(); i++) {
		int j = keyframes.GetKey(i);
		if (j >= frame)
			return i;
	}
	return -1;
}

void GeomMeshAnimation::Visit(Vis& v) {
	v VISM(keyframes);
}

void GeomBone::Visit(Vis& v) {
	v VIS_(name)
	  VISN(position)
	  VISN(orientation)
	  VIS_(length);
	if (v.mode == Vis::MODE_JSON) {
		if (v.IsLoading()) {
			const Value& kids = v.json->Get()["children"];
			val.sub.Clear();
			for (int i = 0; i < kids.GetCount(); i++) {
				VfsValue& n = val.Add(String(), AsTypeHash<GeomBone>());
				GeomBone& b = n.GetExt<GeomBone>();
				JsonIO jio(kids[i]);
				Vis vis(jio);
				b.Visit(vis);
				if (!b.name.IsEmpty())
					n.id = b.name;
			}
		}
		else {
			Vector<Value> children;
			for (auto& s : val.sub) {
				if (!IsVfsType(s, AsTypeHash<GeomBone>()))
					continue;
				GeomBone& b = s.GetExt<GeomBone>();
				children.Add(v.VisitAsJsonValue(b));
			}
			v.json->Set("children", ValueArray(pick(children)));
		}
	}
	else {
		int child_count = 0;
		if (!v.IsLoading()) {
			for (auto& s : val.sub)
				if (IsVfsType(s, AsTypeHash<GeomBone>()))
					child_count++;
		}
		v VIS_(child_count);
		if (v.IsLoading()) {
			val.sub.Clear();
			for (int i = 0; i < child_count; i++) {
				VfsValue& n = val.Add(String(), AsTypeHash<GeomBone>());
				GeomBone& b = n.GetExt<GeomBone>();
				b.Visit(v);
				if (!b.name.IsEmpty())
					n.id = b.name;
			}
		}
		else {
			for (auto& s : val.sub) {
				if (!IsVfsType(s, AsTypeHash<GeomBone>()))
					continue;
				GeomBone& b = s.GetExt<GeomBone>();
				b.Visit(v);
			}
		}
	}
	if (v.IsLoading() && !name.IsEmpty())
		val.id = name;
}

void GeomSkeleton::Visit(Vis& v) {
	v VIS_(name);
	if (v.mode == Vis::MODE_JSON) {
		if (v.IsLoading()) {
			const Value& bones = v.json->Get()["bones"];
			val.sub.Clear();
			for (int i = 0; i < bones.GetCount(); i++) {
				VfsValue& n = val.Add(String(), AsTypeHash<GeomBone>());
				GeomBone& b = n.GetExt<GeomBone>();
				JsonIO jio(bones[i]);
				Vis vis(jio);
				b.Visit(vis);
				if (!b.name.IsEmpty())
					n.id = b.name;
			}
		}
		else {
			Vector<Value> bones;
			for (auto& s : val.sub) {
				if (!IsVfsType(s, AsTypeHash<GeomBone>()))
					continue;
				GeomBone& b = s.GetExt<GeomBone>();
				bones.Add(v.VisitAsJsonValue(b));
			}
			v.json->Set("bones", ValueArray(pick(bones)));
		}
	}
	else {
		int bone_count = 0;
		if (!v.IsLoading()) {
			for (auto& s : val.sub)
				if (IsVfsType(s, AsTypeHash<GeomBone>()))
					bone_count++;
		}
		v VIS_(bone_count);
		if (v.IsLoading()) {
			val.sub.Clear();
			for (int i = 0; i < bone_count; i++) {
				VfsValue& n = val.Add(String(), AsTypeHash<GeomBone>());
				GeomBone& b = n.GetExt<GeomBone>();
				b.Visit(v);
				if (!b.name.IsEmpty())
					n.id = b.name;
			}
		}
		else {
			for (auto& s : val.sub) {
				if (!IsVfsType(s, AsTypeHash<GeomBone>()))
					continue;
				GeomBone& b = s.GetExt<GeomBone>();
				b.Visit(v);
			}
		}
	}
	if (v.IsLoading() && !name.IsEmpty())
		val.id = name;
}

void GeomSkinWeights::Visit(Vis& v) {
	if (v.mode == Vis::MODE_JSON) {
		if (v.IsLoading()) {
			weights.Clear();
			const Value& arr = v.json->Get()["weights"];
			for (int i = 0; i < arr.GetCount(); i++) {
				String bone;
				LoadFromJsonValue(bone, arr[i]["bone"]);
				Vector<float> w;
				const Value& vals = arr[i]["values"];
				w.SetCount(vals.GetCount());
				for (int j = 0; j < vals.GetCount(); j++)
					w[j] = (float)vals[j];
				weights.Add(bone, pick(w));
			}
		}
		else {
			Vector<Value> arr;
			for (int i = 0; i < weights.GetCount(); i++) {
				ValueMap item;
				item.Add("bone", StoreAsJsonValue(weights.GetKey(i)));
				ValueArray vals;
				const Vector<float>& w = weights[i];
				for (int j = 0; j < w.GetCount(); j++)
					vals.Add(w[j]);
				item.Add("values", vals);
				arr.Add(item);
			}
			v.json->Set("weights", ValueArray(pick(arr)));
		}
	}
	else {
		int bone_count = weights.GetCount();
		v VIS_(bone_count);
		if (v.IsLoading()) {
			weights.Clear();
			for (int i = 0; i < bone_count; i++) {
				String bone;
				int cnt = 0;
				v VIS_(bone)
				  VIS_(cnt);
				Vector<float> w;
				w.SetCount(cnt);
				for (int j = 0; j < cnt; j++)
					v VIS_(w[j]);
				weights.Add(bone, pick(w));
			}
		}
		else {
			for (int i = 0; i < bone_count; i++) {
				String bone = weights.GetKey(i);
				const Vector<float>& w = weights[i];
				int cnt = w.GetCount();
				v VIS_(bone)
				  VIS_(cnt);
				for (int j = 0; j < cnt; j++)
					v VIS_(const_cast<float&>(w[j]));
			}
		}
	}
}

void GeomPointcloudEffectTransform::Visit(Vis& v) {
	v VIS_(name)
	  VIS_(enabled)
	  VIS_(locked)
	  VISN(position)
	  VISN(orientation);
	if (v.IsLoading() && !name.IsEmpty())
		val.id = name;
}

String GeomPointcloudDataset::GetId() const {
	if (!name.IsEmpty())
		return name;
	return val.id;
}

void GeomPointcloudDataset::Visit(Vis& v) {
	v VIS_(name)
	  VIS_(source_ref);
	if (v.IsLoading() && !name.IsEmpty())
		val.id = name;
}








GeomKeypoint& GeomTimeline::GetAddKeypoint(int kp_i) {
	int i = keypoints.Find(kp_i);
	if (i >= 0)
		return keypoints[i];
	int pos = keypoints.GetCount();
	for(int i = 0; i < keypoints.GetCount(); i++) {
		int j = keypoints.GetKey(i);
		if (j > kp_i) {
			pos = i;
			break;
		}
	}
	GeomKeypoint& kp = keypoints.Insert(pos, kp_i);
	kp.frame_id = kp_i;
	kp.position = Identity<vec3>();
	kp.orientation = Identity<quat>();
	kp.has_position = true;
	kp.has_orientation = true;
	return kp;
}

int GeomTimeline::FindPre(int kp_i) const {
	for(int i = 0; i < keypoints.GetCount(); i++) {
		int j = keypoints.GetKey(i);
		if (j > kp_i)
			return i-1;
		if (j == kp_i)
			return i;
	}
	return -1;
}

int GeomTimeline::FindPost(int kp_i) const {
	for(int i = keypoints.GetCount()-1; i >= 0; i--) {
		int j = keypoints.GetKey(i);
		if (j <= kp_i)
			return i+1 < keypoints.GetCount() ? i+1 : -1;
	}
	return -1;
}

int GeomTimeline::FindPrePosition(int kp_i) const {
	int best = -1;
	for (int i = 0; i < keypoints.GetCount(); i++) {
		int j = keypoints.GetKey(i);
		if (j > kp_i)
			break;
		if (keypoints[i].has_position)
			best = i;
	}
	return best;
}

int GeomTimeline::FindPostPosition(int kp_i) const {
	for (int i = 0; i < keypoints.GetCount(); i++) {
		int j = keypoints.GetKey(i);
		if (j > kp_i && keypoints[i].has_position)
			return i;
	}
	return -1;
}

int GeomTimeline::FindPreOrientation(int kp_i) const {
	int best = -1;
	for (int i = 0; i < keypoints.GetCount(); i++) {
		int j = keypoints.GetKey(i);
		if (j > kp_i)
			break;
		if (keypoints[i].has_orientation)
			best = i;
	}
	return best;
}

int GeomTimeline::FindPostOrientation(int kp_i) const {
	for (int i = 0; i < keypoints.GetCount(); i++) {
		int j = keypoints.GetKey(i);
		if (j > kp_i && keypoints[i].has_orientation)
			return i;
	}
	return -1;
}






	
GeomScene& GeomProject::AddScene() {
	VfsValue& n = val.Add(String(), AsTypeHash<GeomScene>());
	GeomScene& s = n.GetExt<GeomScene>();
	s.name = String();
	return s;
}

int GeomProject::GetSceneCount() const {
	int count = 0;
	for (const auto& s : val.sub)
		if (IsVfsType(s, AsTypeHash<GeomScene>()))
			count++;
	return count;
}

GeomScene& GeomProject::GetScene(int i) {
	int count = 0;
	for (auto& s : val.sub) {
		if (IsVfsType(s, AsTypeHash<GeomScene>())) {
			if (count == i)
				return s.GetExt<GeomScene>();
			count++;
		}
	}
	Panic("GeomProject::GetScene: index out of range");
	NEVER();
	return val.sub[0].GetExt<GeomScene>();
}

/*GeomModel& GeomProject::AddModel() {
	GeomModel& m = models.Add();
	m.owner = this;
	return m;
}*/


GeomTimeline& GeomObject::GetTimeline() {
	return val.GetAdd<GeomTimeline>("timeline");
}

GeomTimeline* GeomObject::FindTimeline() const {
	for (auto& sub : val.sub) {
		if (IsVfsType(sub, AsTypeHash<GeomTimeline>()) && sub.id == "timeline")
			return &sub.GetExt<GeomTimeline>();
	}
	return 0;
}

GeomSceneTimeline& GeomScene::GetTimeline() {
	static bool init = (TypedStringHasher<GeomSceneTimeline>("GeomSceneTimeline"), true);
	GeomSceneTimeline& tl = val.GetAdd<GeomSceneTimeline>("timeline");
	if (tl.length <= 0)
		tl.length = length;
	return tl;
}

GeomSceneTimeline* GeomScene::FindTimeline() const {
	static bool init = (TypedStringHasher<GeomSceneTimeline>("GeomSceneTimeline"), true);
	for (auto& sub : val.sub) {
		if (IsVfsType(sub, AsTypeHash<GeomSceneTimeline>()) && sub.id == "timeline")
			return &sub.GetExt<GeomSceneTimeline>();
	}
	return 0;
}

GeomTransform& GeomObject::GetTransform() {
	static bool init = (TypedStringHasher<GeomTransform>("GeomTransform"), true);
	return val.GetAdd<GeomTransform>("transform");
}

GeomTransform* GeomObject::FindTransform() const {
	static bool init = (TypedStringHasher<GeomTransform>("GeomTransform"), true);
	for (auto& sub : val.sub) {
		if (IsVfsType(sub, AsTypeHash<GeomTransform>()) && sub.id == "transform")
			return &sub.GetExt<GeomTransform>();
	}
	return 0;
}

GeomDynamicProperties& GeomObject::GetDynamicProperties() {
	static bool init = (TypedStringHasher<GeomDynamicProperties>("GeomDynamicProperties"), true);
	return val.GetAdd<GeomDynamicProperties>("props");
}

GeomDynamicProperties* GeomObject::FindDynamicProperties() const {
	static bool init = (TypedStringHasher<GeomDynamicProperties>("GeomDynamicProperties"), true);
	for (auto& sub : val.sub) {
		if (IsVfsType(sub, AsTypeHash<GeomDynamicProperties>()) && sub.id == "props")
			return &sub.GetExt<GeomDynamicProperties>();
	}
	return 0;
}

GeomEditableMesh& GeomObject::GetEditableMesh() {
	static bool init = (TypedStringHasher<GeomEditableMesh>("GeomEditableMesh"), true);
	return val.GetAdd<GeomEditableMesh>("editable");
}

GeomEditableMesh* GeomObject::FindEditableMesh() const {
	static bool init = (TypedStringHasher<GeomEditableMesh>("GeomEditableMesh"), true);
	for (auto& sub : val.sub) {
		if (IsVfsType(sub, AsTypeHash<GeomEditableMesh>()) && sub.id == "editable")
			return &sub.GetExt<GeomEditableMesh>();
	}
	return 0;
}

Geom2DLayer& GeomObject::Get2DLayer() {
	static bool init = (TypedStringHasher<Geom2DLayer>("Geom2DLayer"), true);
	return val.GetAdd<Geom2DLayer>("layer2d");
}

Geom2DLayer* GeomObject::Find2DLayer() const {
	static bool init = (TypedStringHasher<Geom2DLayer>("Geom2DLayer"), true);
	for (auto& sub : val.sub) {
		if (IsVfsType(sub, AsTypeHash<Geom2DLayer>()) && sub.id == "layer2d")
			return &sub.GetExt<Geom2DLayer>();
	}
	return 0;
}

Geom2DAnimation& GeomObject::Get2DAnimation() {
	static bool init = (TypedStringHasher<Geom2DAnimation>("Geom2DAnimation"), true);
	return val.GetAdd<Geom2DAnimation>("layer2d_anim");
}

Geom2DAnimation* GeomObject::Find2DAnimation() const {
	static bool init = (TypedStringHasher<Geom2DAnimation>("Geom2DAnimation"), true);
	for (auto& sub : val.sub) {
		if (IsVfsType(sub, AsTypeHash<Geom2DAnimation>()) && sub.id == "layer2d_anim")
			return &sub.GetExt<Geom2DAnimation>();
	}
	return 0;
}

GeomTextureEdit& GeomObject::GetTextureEdit() {
	static bool init = (TypedStringHasher<GeomTextureEdit>("GeomTextureEdit"), true);
	return val.GetAdd<GeomTextureEdit>("texture_edit");
}

GeomTextureEdit* GeomObject::FindTextureEdit() const {
	static bool init = (TypedStringHasher<GeomTextureEdit>("GeomTextureEdit"), true);
	for (auto& sub : val.sub) {
		if (IsVfsType(sub, AsTypeHash<GeomTextureEdit>()) && sub.id == "texture_edit")
			return &sub.GetExt<GeomTextureEdit>();
	}
	return 0;
}

GeomMeshAnimation& GeomObject::GetMeshAnimation() {
	static bool init = (TypedStringHasher<GeomMeshAnimation>("GeomMeshAnimation"), true);
	return val.GetAdd<GeomMeshAnimation>("mesh_anim");
}

GeomMeshAnimation* GeomObject::FindMeshAnimation() const {
	static bool init = (TypedStringHasher<GeomMeshAnimation>("GeomMeshAnimation"), true);
	for (auto& sub : val.sub) {
		if (IsVfsType(sub, AsTypeHash<GeomMeshAnimation>()) && sub.id == "mesh_anim")
			return &sub.GetExt<GeomMeshAnimation>();
	}
	return 0;
}

GeomSkeleton& GeomObject::GetSkeleton() {
	static bool init = (TypedStringHasher<GeomSkeleton>("GeomSkeleton"), true);
	return val.GetAdd<GeomSkeleton>("skeleton");
}

GeomSkeleton* GeomObject::FindSkeleton() const {
	static bool init = (TypedStringHasher<GeomSkeleton>("GeomSkeleton"), true);
	for (auto& sub : val.sub) {
		if (IsVfsType(sub, AsTypeHash<GeomSkeleton>()) && sub.id == "skeleton")
			return &sub.GetExt<GeomSkeleton>();
	}
	return 0;
}

GeomSkinWeights& GeomObject::GetSkinWeights() {
	static bool init = (TypedStringHasher<GeomSkinWeights>("GeomSkinWeights"), true);
	return val.GetAdd<GeomSkinWeights>("skinweights");
}

GeomSkinWeights* GeomObject::FindSkinWeights() const {
	static bool init = (TypedStringHasher<GeomSkinWeights>("GeomSkinWeights"), true);
	for (auto& sub : val.sub) {
		if (IsVfsType(sub, AsTypeHash<GeomSkinWeights>()) && sub.id == "skinweights")
			return &sub.GetExt<GeomSkinWeights>();
	}
	return 0;
}

GeomPointcloudEffectTransform& GeomObject::GetAddPointcloudEffect(String name) {
	for (auto& sub : val.sub) {
		if (!IsVfsType(sub, AsTypeHash<GeomPointcloudEffectTransform>()))
			continue;
		GeomPointcloudEffectTransform& fx = sub.GetExt<GeomPointcloudEffectTransform>();
		String fx_name = fx.name.IsEmpty() ? sub.id : fx.name;
		if (fx_name == name)
			return fx;
	}
	GeomPointcloudEffectTransform& fx = val.GetAdd<GeomPointcloudEffectTransform>(name);
	fx.name = name;
	fx.val.id = name;
	return fx;
}

void GeomObject::GetPointcloudEffects(Vector<GeomPointcloudEffectTransform*>& out) const {
	out.Clear();
	for (auto& sub : val.sub) {
		if (!IsVfsType(sub, AsTypeHash<GeomPointcloudEffectTransform>()))
			continue;
		out.Add(&sub.GetExt<GeomPointcloudEffectTransform>());
	}
}

String GeomObject::GetPath() const {
	String path = name;
	const VfsValue* dir = val.owner;
	while (dir) {
		if (dir->ext) {
			const GeomDirectory* d = CastConstPtr<GeomDirectory>(dir->ext.Get());
			if (d && !d->name.IsEmpty())
				path = d->name + "/" + path;
		}
		dir = dir->owner;
	}
	return path;
}

void GeomObject::Visit(Vis& v) {
	EnsureGeomScriptRegistered();
	const hash_t fx_hash = TypedStringHasher<GeomPointcloudEffectTransform>("GeomPointcloudEffectTransform");
	const hash_t sk_hash = TypedStringHasher<GeomSkeleton>("GeomSkeleton");
	const hash_t sw_hash = TypedStringHasher<GeomSkinWeights>("GeomSkinWeights");
	const hash_t sc_hash = TypedStringHasher<GeomScript>("GeomScript");
	int type_i = (int)type;
	v VIS_(name)
	  VIS_(type_i)
	  VIS_(ui_type)
	  VIS_(asset_ref)
	  VIS_(pointcloud_ref)
	  VIS_(is_visible)
	  VIS_(is_locked)
	  VIS_(read_enabled)
	  VIS_(write_enabled)
	  VIS_(timeline_muted)
	  VIS_(timeline_solo);
	GeomTimeline& tl = GetTimeline();
	v("timeline", tl, VISIT_NODE);
	GeomTransform& tr = GetTransform();
	v("transform", tr, VISIT_NODE);
	GeomDynamicProperties& props = GetDynamicProperties();
	v("props", props, VISIT_NODE);
	GeomEditableMesh& mesh = GetEditableMesh();
	v("editable", mesh, VISIT_NODE);
	Geom2DLayer& layer2d = Get2DLayer();
	v("layer2d", layer2d, VISIT_NODE);
	Geom2DAnimation& layer2d_anim = Get2DAnimation();
	v("layer2d_anim", layer2d_anim, VISIT_NODE);
	GeomTextureEdit& tex = GetTextureEdit();
	v("texture_edit", tex, VISIT_NODE);
	GeomMeshAnimation& mesh_anim = GetMeshAnimation();
	v("mesh_anim", mesh_anim, VISIT_NODE);
	GeomSkeleton* skel_ptr = FindSkeleton();
	GeomSkinWeights* weights_ptr = FindSkinWeights();
	if (v.mode == Vis::MODE_JSON) {
		if (v.IsLoading()) {
			const Value& effects_va = v.json->Get()["effects"];
			for (int i = 0; i < effects_va.GetCount(); i++) {
				VfsValue& n = val.Add(String(), fx_hash);
				GeomPointcloudEffectTransform& fx = n.GetExt<GeomPointcloudEffectTransform>();
				JsonIO jio(effects_va[i]);
				Vis vis(jio);
				fx.Visit(vis);
				if (!fx.name.IsEmpty())
					n.id = fx.name;
			}
			const Value& skv = v.json->Get()["skeleton"];
			if (!IsNull(skv)) {
				VfsValue& n = val.GetAdd("skeleton", sk_hash);
				GeomSkeleton& sk = n.GetExt<GeomSkeleton>();
				JsonIO jio(skv);
				Vis vis(jio);
				sk.Visit(vis);
			}
			const Value& wv = v.json->Get()["skinweights"];
			if (!IsNull(wv)) {
				VfsValue& n = val.GetAdd("skinweights", sw_hash);
				GeomSkinWeights& sw = n.GetExt<GeomSkinWeights>();
				JsonIO jio(wv);
				Vis vis(jio);
				sw.Visit(vis);
			}
			const Value& scripts_va = v.json->Get()["scripts"];
			for (int i = 0; i < scripts_va.GetCount(); i++) {
				VfsValue& n = val.Add(String(), sc_hash);
				GeomScript& sc = n.GetExt<GeomScript>();
				JsonIO jio(scripts_va[i]);
				Vis vis(jio);
				sc.Visit(vis);
				if (!sc.file.IsEmpty())
					n.id = GetFileTitle(sc.file);
			}
		}
		else {
			Vector<Value> effects_values;
			for (auto& s : val.sub) {
				if (!IsVfsType(s, fx_hash))
					continue;
				GeomPointcloudEffectTransform& fx = s.GetExt<GeomPointcloudEffectTransform>();
				effects_values.Add(v.VisitAsJsonValue(fx));
			}
			v.json->Set("effects", ValueArray(pick(effects_values)));
			if (skel_ptr)
				v.json->Set("skeleton", v.VisitAsJsonValue(*skel_ptr));
			if (weights_ptr)
				v.json->Set("skinweights", v.VisitAsJsonValue(*weights_ptr));
			Vector<Value> script_values;
			for (auto& s : val.sub) {
				if (!IsVfsType(s, sc_hash))
					continue;
				GeomScript& sc = s.GetExt<GeomScript>();
				script_values.Add(v.VisitAsJsonValue(sc));
			}
			v.json->Set("scripts", ValueArray(pick(script_values)));
		}
	}
	else {
		int effect_count = 0;
		int has_skeleton = skel_ptr != nullptr;
		int has_weights = weights_ptr != nullptr;
		if (!v.IsLoading()) {
			for (auto& s : val.sub)
				if (IsVfsType(s, fx_hash))
					effect_count++;
		}
		v VIS_(effect_count)
		  VIS_(has_skeleton)
		  VIS_(has_weights);
		if (v.IsLoading()) {
			for (int i = 0; i < effect_count; i++) {
				VfsValue& n = val.Add(String(), fx_hash);
				GeomPointcloudEffectTransform& fx = n.GetExt<GeomPointcloudEffectTransform>();
				fx.Visit(v);
				if (!fx.name.IsEmpty())
					n.id = fx.name;
			}
			if (has_skeleton) {
				VfsValue& n = val.GetAdd("skeleton", sk_hash);
				GeomSkeleton& sk = n.GetExt<GeomSkeleton>();
				sk.Visit(v);
			}
			if (has_weights) {
				VfsValue& n = val.GetAdd("skinweights", sw_hash);
				GeomSkinWeights& sw = n.GetExt<GeomSkinWeights>();
				sw.Visit(v);
			}
		}
		else {
			for (auto& s : val.sub) {
				if (!IsVfsType(s, fx_hash))
					continue;
				GeomPointcloudEffectTransform& fx = s.GetExt<GeomPointcloudEffectTransform>();
				fx.Visit(v);
			}
			if (skel_ptr)
				skel_ptr->Visit(v);
			if (weights_ptr)
				weights_ptr->Visit(v);
		}
	}
	if (v.IsLoading()) {
		type = (Type)type_i;
		if (!name.IsEmpty())
			val.id = name;
	}
}




GeomProject& GeomDirectory::GetProject() const {
	GeomProject* prj = val.FindOwner<GeomProject>();
	ASSERT(prj);
	return *prj;
}

int GeomDirectory::GetSubdirCount() const {
	int count = 0;
	for (const auto& s : val.sub)
		if (IsVfsType(s, AsTypeHash<GeomDirectory>()))
			count++;
	return count;
}

GeomDirectory& GeomDirectory::GetSubdir(int i) const {
	int count = 0;
	for (auto& s : val.sub) {
		if (IsVfsType(s, AsTypeHash<GeomDirectory>())) {
			if (count == i)
				return s.GetExt<GeomDirectory>();
			count++;
		}
	}
	Panic("GeomDirectory::GetSubdir: index out of range");
	NEVER();
	return val.sub[0].GetExt<GeomDirectory>();
}

int GeomDirectory::GetObjectCount() const {
	int count = 0;
	for (const auto& s : val.sub)
		if (IsVfsType(s, AsTypeHash<GeomObject>()))
			count++;
	return count;
}

GeomObject& GeomDirectory::GetObject(int i) const {
	int count = 0;
	for (auto& s : val.sub) {
		if (IsVfsType(s, AsTypeHash<GeomObject>())) {
			if (count == i)
				return s.GetExt<GeomObject>();
			count++;
		}
	}
	Panic("GeomDirectory::GetObject: index out of range");
	NEVER();
	return val.sub[0].GetExt<GeomObject>();
}

GeomDirectory& GeomDirectory::GetAddDirectory(String name) {
	GeomDirectory& dir = val.GetAdd<GeomDirectory>(name);
	dir.name = name;
	dir.val.id = name;
	return dir;
}

GeomTransform& GeomDirectory::GetTransform() {
	static bool init = (TypedStringHasher<GeomTransform>("GeomTransform"), true);
	return val.GetAdd<GeomTransform>("transform");
}

GeomTransform* GeomDirectory::FindTransform() const {
	static bool init = (TypedStringHasher<GeomTransform>("GeomTransform"), true);
	for (auto& sub : val.sub) {
		if (IsVfsType(sub, AsTypeHash<GeomTransform>()) && sub.id == "transform")
			return &sub.GetExt<GeomTransform>();
	}
	return 0;
}

GeomDynamicProperties& GeomDirectory::GetDynamicProperties() {
	static bool init = (TypedStringHasher<GeomDynamicProperties>("GeomDynamicProperties"), true);
	return val.GetAdd<GeomDynamicProperties>("props");
}

GeomDynamicProperties* GeomDirectory::FindDynamicProperties() const {
	static bool init = (TypedStringHasher<GeomDynamicProperties>("GeomDynamicProperties"), true);
	for (auto& sub : val.sub) {
		if (IsVfsType(sub, AsTypeHash<GeomDynamicProperties>()) && sub.id == "props")
			return &sub.GetExt<GeomDynamicProperties>();
	}
	return 0;
}

void GeomDirectory::Visit(Vis& v) {
	v VIS_(name);
	if (v.IsLoading() && !name.IsEmpty())
		val.id = name;
	EnsureGeomScriptRegistered();
	const hash_t sc_hash = TypedStringHasher<GeomScript>("GeomScript");
	GeomTransform& tr = GetTransform();
	GeomDynamicProperties& props = GetDynamicProperties();
	if (v.mode == Vis::MODE_JSON) {
		if (v.IsLoading()) {
			const Value& trv = v.json->Get()["transform"];
			if (!IsNull(trv)) {
				JsonIO jio(trv);
				Vis vis(jio);
				tr.Visit(vis);
			}
			const Value& propv = v.json->Get()["props"];
			if (!IsNull(propv)) {
				JsonIO jio(propv);
				Vis vis(jio);
				props.Visit(vis);
			}
			val.sub.Clear();
			const Value& sub_va = v.json->Get()["subdir"];
			for (int i = 0; i < sub_va.GetCount(); i++) {
				String key;
				LoadFromJsonValue(key, sub_va[i]["key"]);
				VfsValue& n = val.Add(key, AsTypeHash<GeomDirectory>());
				GeomDirectory& dir = n.GetExt<GeomDirectory>();
				dir.name = key;
				JsonIO jio(sub_va[i]["value"]);
				Vis vis(jio);
				dir.Visit(vis);
			}
			const Value& obj_va = v.json->Get()["objs"];
			for (int i = 0; i < obj_va.GetCount(); i++) {
				VfsValue& n = val.Add(String(), AsTypeHash<GeomObject>());
				GeomObject& o = n.GetExt<GeomObject>();
				JsonIO jio(obj_va[i]);
				Vis vis(jio);
				o.Visit(vis);
				if (!o.name.IsEmpty())
					n.id = o.name;
			}
			const Value& ds_va = v.json->Get()["datasets"];
			for (int i = 0; i < ds_va.GetCount(); i++) {
				VfsValue& n = val.Add(String(), AsTypeHash<GeomPointcloudDataset>());
				GeomPointcloudDataset& ds = n.GetExt<GeomPointcloudDataset>();
				JsonIO jio(ds_va[i]);
				Vis vis(jio);
				ds.Visit(vis);
				if (!ds.name.IsEmpty())
					n.id = ds.name;
			}
			const Value& scripts_va = v.json->Get()["scripts"];
			for (int i = 0; i < scripts_va.GetCount(); i++) {
				VfsValue& n = val.Add(String(), sc_hash);
				GeomScript& sc = n.GetExt<GeomScript>();
				JsonIO jio(scripts_va[i]);
				Vis vis(jio);
				sc.Visit(vis);
				if (!sc.file.IsEmpty())
					n.id = GetFileTitle(sc.file);
			}
		}
		else {
			Vector<Value> subdir_values;
			for (auto& s : val.sub) {
				if (!IsVfsType(s, AsTypeHash<GeomDirectory>()))
					continue;
				GeomDirectory& dir = s.GetExt<GeomDirectory>();
				ValueMap item;
				String key = dir.name;
				if (key.IsEmpty())
					key = s.id;
				item.Add("key", StoreAsJsonValue(key));
				item.Add("value", v.VisitAsJsonValue(dir));
				subdir_values.Add(item);
			}
			v.json->Set("transform", v.VisitAsJsonValue(tr));
			v.json->Set("props", v.VisitAsJsonValue(props));
			v.json->Set("subdir", ValueArray(pick(subdir_values)));
			Vector<Value> obj_values;
			for (auto& s : val.sub) {
				if (!IsVfsType(s, AsTypeHash<GeomObject>()))
					continue;
				GeomObject& o = s.GetExt<GeomObject>();
				obj_values.Add(v.VisitAsJsonValue(o));
			}
			v.json->Set("objs", ValueArray(pick(obj_values)));
			Vector<Value> ds_values;
			for (auto& s : val.sub) {
				if (!IsVfsType(s, AsTypeHash<GeomPointcloudDataset>()))
					continue;
				GeomPointcloudDataset& ds = s.GetExt<GeomPointcloudDataset>();
				ds_values.Add(v.VisitAsJsonValue(ds));
			}
			v.json->Set("datasets", ValueArray(pick(ds_values)));
			Vector<Value> script_values;
			for (auto& s : val.sub) {
				if (!IsVfsType(s, sc_hash))
					continue;
				GeomScript& sc = s.GetExt<GeomScript>();
				script_values.Add(v.VisitAsJsonValue(sc));
			}
			v.json->Set("scripts", ValueArray(pick(script_values)));
		}
	}
	else {
		int subdir_count = 0;
		int obj_count = 0;
		int ds_count = 0;
		if (!v.IsLoading()) {
			for (auto& s : val.sub) {
				if (IsVfsType(s, AsTypeHash<GeomDirectory>()))
					subdir_count++;
				else if (IsVfsType(s, AsTypeHash<GeomObject>()))
					obj_count++;
				else if (IsVfsType(s, AsTypeHash<GeomPointcloudDataset>()))
					ds_count++;
			}
		}
		v("transform", tr, VISIT_NODE);
		v("props", props, VISIT_NODE);
		v VIS_(subdir_count)
		  VIS_(obj_count)
		  VIS_(ds_count);
		if (v.IsLoading()) {
			val.sub.Clear();
			for (int i = 0; i < subdir_count; i++) {
				String key;
				v VIS_(key);
				VfsValue& n = val.Add(key, AsTypeHash<GeomDirectory>());
				GeomDirectory& dir = n.GetExt<GeomDirectory>();
				dir.name = key;
				dir.Visit(v);
			}
			for (int i = 0; i < obj_count; i++) {
				VfsValue& n = val.Add(String(), AsTypeHash<GeomObject>());
				GeomObject& o = n.GetExt<GeomObject>();
				o.Visit(v);
				if (!o.name.IsEmpty())
					n.id = o.name;
			}
			for (int i = 0; i < ds_count; i++) {
				VfsValue& n = val.Add(String(), AsTypeHash<GeomPointcloudDataset>());
				GeomPointcloudDataset& ds = n.GetExt<GeomPointcloudDataset>();
				ds.Visit(v);
				if (!ds.name.IsEmpty())
					n.id = ds.name;
			}
		}
		else {
			for (auto& s : val.sub) {
				if (!IsVfsType(s, AsTypeHash<GeomDirectory>()))
					continue;
				GeomDirectory& dir = s.GetExt<GeomDirectory>();
				String key = dir.name;
				if (key.IsEmpty())
					key = s.id;
				v VIS_(key);
				dir.Visit(v);
			}
			for (auto& s : val.sub) {
				if (!IsVfsType(s, AsTypeHash<GeomObject>()))
					continue;
				GeomObject& o = s.GetExt<GeomObject>();
				o.Visit(v);
			}
			for (auto& s : val.sub) {
				if (!IsVfsType(s, AsTypeHash<GeomPointcloudDataset>()))
					continue;
				GeomPointcloudDataset& ds = s.GetExt<GeomPointcloudDataset>();
				ds.Visit(v);
			}
		}
	}
}

GeomObject* GeomDirectory::FindObject(String name) {
	for (auto& s : val.sub) {
		if (!IsVfsType(s, AsTypeHash<GeomObject>()))
			continue;
		GeomObject& o = s.GetExt<GeomObject>();
		if (o.name == name)
			return &o;
	}
	return 0;
}

GeomObject* GeomDirectory::FindObject(String name, GeomObject::Type type) {
	for (auto& s : val.sub) {
		if (!IsVfsType(s, AsTypeHash<GeomObject>()))
			continue;
		GeomObject& o = s.GetExt<GeomObject>();
		if (o.name == name && o.type == type)
			return &o;
	}
	return 0;
}

GeomObject* GeomDirectory::FindCamera(String name) {
	return FindObject(name, GeomObject::O_CAMERA);
}

GeomPointcloudDataset& GeomDirectory::GetAddPointcloudDataset(String id) {
	for (auto& s : val.sub) {
		if (!IsVfsType(s, AsTypeHash<GeomPointcloudDataset>()))
			continue;
		GeomPointcloudDataset& ds = s.GetExt<GeomPointcloudDataset>();
		String ds_id = ds.GetId();
		if (ds_id == id)
			return ds;
	}
	GeomPointcloudDataset& ds = val.GetAdd<GeomPointcloudDataset>(id);
	ds.name = id;
	ds.val.id = id;
	return ds;
}

GeomPointcloudDataset* GeomDirectory::FindPointcloudDataset(String id) {
	for (auto& s : val.sub) {
		if (!IsVfsType(s, AsTypeHash<GeomPointcloudDataset>()))
			continue;
		GeomPointcloudDataset& ds = s.GetExt<GeomPointcloudDataset>();
		String ds_id = ds.GetId();
		if (ds_id == id)
			return &ds;
	}
	return 0;
}

GeomObject& GeomDirectory::GetAddModel(String name) {
	GeomObject* o = FindObject(name);
	if (o)
		return *o;
	GeomObject& obj = val.GetAdd<GeomObject>(name);
	obj.key = GetProject().NewKey();
	obj.name = name;
	obj.type = GeomObject::O_MODEL;
	obj.val.id = name;
	return obj;
}

void GeomScene::Visit(Vis& v) {
	v.VisitT<GeomDirectory>("GeomDirectory", *this);
	v VIS_(length);
	GeomSceneTimeline& tl = GetTimeline();
	v("timeline", tl, VISIT_NODE);
	if (v.IsLoading() && tl.length <= 0)
		tl.length = length;
}

void GeomProject::Visit(Vis& v) {
	if (v.mode == Vis::MODE_JSON) {
		if (v.IsLoading()) {
			val.sub.Clear();
			const Value& va = v.json->Get()["scenes"];
			for (int i = 0; i < va.GetCount(); i++) {
				VfsValue& n = val.Add(String(), AsTypeHash<GeomScene>());
				GeomScene& scene = n.GetExt<GeomScene>();
				JsonIO jio(va[i]);
				Vis vis(jio);
				scene.Visit(vis);
			}
		}
		else {
			Vector<Value> va;
			for (auto& s : val.sub) {
				if (!IsVfsType(s, AsTypeHash<GeomScene>()))
					continue;
				GeomScene& scene = s.GetExt<GeomScene>();
				va.Add(v.VisitAsJsonValue(scene));
			}
			v.json->Set("scenes", ValueArray(pick(va)));
		}
		v VIS_(kps)
		  VIS_(fps)
		  VIS_(key_counter);
	}
	else {
		int scene_count = 0;
		if (!v.IsLoading()) {
			for (auto& s : val.sub)
				if (IsVfsType(s, AsTypeHash<GeomScene>()))
					scene_count++;
		}
		v VIS_(scene_count)
		  VIS_(kps)
		  VIS_(fps)
		  VIS_(key_counter);
		if (v.IsLoading()) {
			val.sub.Clear();
			for (int i = 0; i < scene_count; i++) {
				VfsValue& n = val.Add(String(), AsTypeHash<GeomScene>());
				GeomScene& scene = n.GetExt<GeomScene>();
				scene.Visit(v);
			}
		}
		else {
			for (auto& s : val.sub) {
				if (!IsVfsType(s, AsTypeHash<GeomScene>()))
					continue;
				GeomScene& scene = s.GetExt<GeomScene>();
				scene.Visit(v);
			}
		}
	}
}

GeomObject& GeomDirectory::GetAddCamera(String name) {
	GeomObject* o = FindObject(name);
	if (o)
		return *o;
	GeomObject& obj = val.GetAdd<GeomObject>(name);
	obj.key = GetProject().NewKey();
	obj.name = name;
	obj.type = GeomObject::O_CAMERA;
	obj.val.id = name;
	return obj;
}

GeomObject& GeomDirectory::GetAddOctree(String name) {
	GeomObject* o = FindObject(name);
	if (o)
		return *o;
	GeomObject& obj = val.GetAdd<GeomObject>(name);
	obj.key = GetProject().NewKey();
	obj.name = name;
	obj.type = GeomObject::O_OCTREE;
	obj.val.id = name;
	return obj;
}













GeomCamera& GeomWorldState::GetFocus() {
	GeomCamera& cam = val.GetAdd<GeomCamera>("focus");
	return cam;
}

GeomCamera& GeomWorldState::GetProgram() {
	GeomCamera& cam = val.GetAdd<GeomCamera>("program");
	return cam;
}

void GeomWorldState::Visit(Vis& v) {
	v VIS_(active_scene)
	  VIS_(active_camera_obj_i)
	  VIS_(focus_mode)
	  VIS_(focus_object_key)
	  VIS_(program_visible)
	  VIS_(focus_visible);
	GeomCamera& focus = GetFocus();
	GeomCamera& program = GetProgram();
	v("focus", focus, VISIT_NODE);
	v("program", program, VISIT_NODE);
}

void GeomWorldState::UpdateObjects() {
	GeomScene& scene = GetActiveScene();
	GeomObjectCollection collection(scene);
	this->objs.SetCount(0);
	int i = 0;
	active_camera_obj_i = -1;
	auto apply_local = [](vec3& pos, quat& ori, vec3& scale, const vec3& lpos, const quat& lori, const vec3& lscale) {
		pos = pos + VectorTransform(lpos * scale, ori);
		ori = ori * lori;
		scale = scale * lscale;
	};
	for (GeomObject& o : collection) {
		GeomObjectState& s = objs.Add();
		s.obj = &o;
		vec3 pos = vec3(0);
		quat ori = Identity<quat>();
		vec3 scale = vec3(1);
		for (VfsValue* n = o.val.owner; n; n = n->owner) {
			if (IsVfsType(*n, AsTypeHash<GeomDirectory>())) {
				GeomDirectory& dir = n->GetExt<GeomDirectory>();
				if (GeomTransform* tr = dir.FindTransform())
					apply_local(pos, ori, scale, tr->position, tr->orientation, tr->scale);
			}
		}
		GeomTimeline* tl = o.FindTimeline();
		if (o.read_enabled && tl && !tl->keypoints.IsEmpty()) {
			GeomKeypoint& kp = tl->keypoints[0];
			if (GeomTransform* tr = o.FindTransform()) {
				tr->position = kp.position;
				tr->orientation = kp.orientation;
			}
			apply_local(pos, ori, scale, kp.position, kp.orientation, vec3(1));
		}
		else if (GeomTransform* tr = o.FindTransform()) {
			apply_local(pos, ori, scale, tr->position, tr->orientation, tr->scale);
		}
		s.position = pos;
		s.orientation = ori;
		s.scale = scale;
		if (o.IsOctree()) {
			if (!o.pointcloud_ref.IsEmpty()) {
				GeomPointcloudDataset* ds = scene.FindPointcloudDataset(o.pointcloud_ref);
				if (ds)
					o.octree_ptr = ds->octree_ptr ? ds->octree_ptr : &ds->octree.octree;
			}
		}
		if (active_camera_obj_i < 0 && o.IsCamera())
			active_camera_obj_i = i;
		i++;
	}
}

GeomObject* GeomWorldState::FindObjectByKey(hash_t key) const {
	if (!prj || key == 0 || active_scene < 0 || active_scene >= prj->GetSceneCount())
		return 0;
	GeomScene& scene = prj->GetScene(active_scene);
	GeomObjectCollection iter(scene);
	for (GeomObject& go : iter)
		if (go.key == key)
			return &go;
	return 0;
}

const GeomObjectState* GeomWorldState::FindObjectStateByKey(hash_t key) const {
	for (const GeomObjectState& os : objs) {
		if (os.obj && os.obj->key == key)
			return &os;
	}
	return 0;
}

GeomScene& GeomWorldState::GetActiveScene() {
	ASSERT(prj);
	ASSERT(active_scene >= 0 && active_scene < prj->GetSceneCount());
	return prj->GetScene(active_scene);
}







void GeomCamera::LoadCamera(ViewMode m, Camera& cam, Size sz, float far) const {
	float ratio = (float)sz.cy / (float)sz.cx;
	float aspect = (float)sz.cx / (float)sz.cy;
	
	vec3 position = this->position;
	quat orientation = this->orientation;
	float scale = this->scale;
	float len = 2;
	bool move_camera = true;
	switch (m) {
		
	case VIEWMODE_YZ:
		orientation = MatQuat(YRotation(M_PI/2));
		cam.SetOrthographic(len * aspect, len, 0.1, far);
		break;
		
	case VIEWMODE_XZ:
		orientation = MatQuat(XRotation(-M_PI/2));
		cam.SetOrthographic(len * aspect, len, 0.1, far);
		break;
		
	case VIEWMODE_XY:
		orientation = MatQuat(YRotation(0));
		cam.SetOrthographic(len * aspect, len, 0.1, far);
		break;
		
	case VIEWMODE_PERSPECTIVE:
		cam.SetPerspective(fov, aspect, 0.1, far);
		move_camera = false;
		break;
		
	default:
		return;
		
	}
	
	if (move_camera)
		position = position - VecMul(QuatMat(orientation), VEC_FWD) * scale * 0.01;
	
	cam.SetWorld(position, orientation, scale);
}

mat4 GeomCamera::GetViewMatrix(ViewMode m, Size sz) const {
	Camera cam;
	LoadCamera(m, cam, sz);
	return cam.GetViewMatrix();
}

Frustum GeomCamera::GetFrustum(ViewMode m, Size sz) const {
	Camera cam;
	LoadCamera(m, cam, sz);
	return cam.GetFrustum();
}

static void ApplyMeshAnimation(GeomObject& obj, int position, double time, int kps) {
	GeomEditableMesh* mesh = obj.FindEditableMesh();
	GeomMeshAnimation* anim = obj.FindMeshAnimation();
	if (!mesh || !anim || anim->keyframes.IsEmpty())
		return;
	int pre_i = anim->FindPre(position);
	int post_i = anim->FindPost(position);
	if (pre_i < 0 && post_i < 0)
		return;
	if (pre_i < 0)
		pre_i = post_i;
	if (post_i < 0)
		post_i = pre_i;
	GeomMeshKeyframe& pre = anim->keyframes[pre_i];
	GeomMeshKeyframe& post = anim->keyframes[post_i];
	if (pre.points.IsEmpty())
		return;
	if (pre_i == post_i || post.points.IsEmpty()) {
		mesh->points.SetCount(pre.points.GetCount());
		for (int i = 0; i < pre.points.GetCount(); i++)
			mesh->points[i] = pre.points[i];
		return;
	}
	float pre_time = pre.frame_id / (float)kps;
	float post_time = post.frame_id / (float)kps;
	float f = (time - pre_time) / (post_time - pre_time);
	f = minmax(f, 0.0f, 1.0f);
	int count = min(pre.points.GetCount(), post.points.GetCount());
	mesh->points.SetCount(count);
	for (int i = 0; i < count; i++)
		mesh->points[i] = Lerp(pre.points[i], post.points[i], f);
}

static void Copy2DShapes(Vector<Geom2DShape>& dst, const Vector<Geom2DShape>& src) {
	dst.SetCount(src.GetCount());
	for (int i = 0; i < src.GetCount(); i++) {
		const Geom2DShape& s = src[i];
		Geom2DShape& d = dst[i];
		d.type = s.type;
		d.radius = s.radius;
		d.stroke = s.stroke;
		d.width = s.width;
		d.stroke_cap = s.stroke_cap;
		d.stroke_join = s.stroke_join;
		d.closed = s.closed;
		d.tex_wrap = s.tex_wrap;
		d.stroke_uv_mode = s.stroke_uv_mode;
		d.tex_repeat_x = s.tex_repeat_x;
		d.tex_repeat_y = s.tex_repeat_y;
		d.points.SetCount(s.points.GetCount());
		for (int k = 0; k < s.points.GetCount(); k++)
			d.points[k] = s.points[k];
	}
}

static void Apply2DAnimation(GeomObject& obj, int position, double time, int kps) {
	Geom2DLayer* layer = obj.Find2DLayer();
	Geom2DAnimation* anim = obj.Find2DAnimation();
	if (!layer || !anim || anim->keyframes.IsEmpty())
		return;
	int pre_i = anim->FindPre(position);
	int post_i = anim->FindPost(position);
	if (pre_i < 0 && post_i < 0)
		return;
	if (pre_i < 0)
		pre_i = post_i;
	if (post_i < 0)
		post_i = pre_i;
	Geom2DKeyframe& pre = anim->keyframes[pre_i];
	Geom2DKeyframe& post = anim->keyframes[post_i];
	if (pre.shapes.IsEmpty())
		return;
	if (pre_i == post_i || post.shapes.IsEmpty() || pre.shapes.GetCount() != post.shapes.GetCount()) {
		Copy2DShapes(layer->shapes, pre.shapes);
		return;
	}
	float pre_time = pre.frame_id / (float)kps;
	float post_time = post.frame_id / (float)kps;
	float f = (time - pre_time) / (post_time - pre_time);
	f = minmax(f, 0.0f, 1.0f);
	layer->shapes.SetCount(pre.shapes.GetCount());
	for (int i = 0; i < pre.shapes.GetCount(); i++) {
		const Geom2DShape& a = pre.shapes[i];
		const Geom2DShape& b = post.shapes[i];
		Geom2DShape& d = layer->shapes[i];
		d.type = a.type;
		d.closed = a.closed;
		d.stroke = a.stroke;
		d.width = (float)((1.0f - f) * a.width + f * b.width);
		d.radius = (float)((1.0f - f) * a.radius + f * b.radius);
		if (a.type != b.type || a.points.GetCount() != b.points.GetCount()) {
			d.points.Clear();
			d.points.SetCount(a.points.GetCount());
			for (int k = 0; k < a.points.GetCount(); k++)
				d.points[k] = a.points[k];
			continue;
		}
		d.points.SetCount(a.points.GetCount());
		for (int k = 0; k < a.points.GetCount(); k++) {
			d.points[k][0] = (float)((1.0f - f) * a.points[k][0] + f * b.points[k][0]);
			d.points[k][1] = (float)((1.0f - f) * a.points[k][1] + f * b.points[k][1]);
		}
	}
}






void GeomAnim::ApplyAtPosition(int pos, double t) {
	if (!state || !state->prj)
		return;
	GeomProject& prj = *state->prj;
	position = pos;
	time = t;
	bool has_solo = false;
	for (int i = 0; i < state->objs.GetCount(); i++) {
		if (state->objs[i].obj && state->objs[i].obj->timeline_solo)
			has_solo = true;
	}
	auto apply_ease = [](int mode, float f) {
		if (mode == 1) // step
			return 0.0f;
		if (mode == 2) { // smooth
			return f * f * (3.0f - 2.0f * f);
		}
		return f;
	};
	for (GeomObjectState& os : state->objs) {
		GeomObject& o = *os.obj;
		if (!o.read_enabled)
			continue;
		if (o.timeline_muted)
			continue;
		if (has_solo && !o.timeline_solo)
			continue;
		GeomTimeline* tl = o.FindTimeline();
		if (tl && !tl->keypoints.IsEmpty()) {
			int pre_i = tl->FindPrePosition(position);
			int post_i = tl->FindPostPosition(position);
			if (pre_i >= 0 && post_i >= 0) {
				ASSERT(pre_i < post_i);
				GeomKeypoint& pre = tl->keypoints[pre_i];
				GeomKeypoint& post = tl->keypoints[post_i];
				float pre_time = pre.frame_id / (float)prj.kps;
				float post_time = post.frame_id / (float)prj.kps;
				float f = (post_time > pre_time) ? (time - pre_time) / (post_time - pre_time) : 0.0f;
				f = min(max(f, 0.0f), 1.0f);
				f = apply_ease(pre.position_ease, f);
				os.position = Lerp(pre.position, post.position, f);
			}
			else if (pre_i >= 0) {
				GeomKeypoint& pre = tl->keypoints[pre_i];
				os.position = pre.position;
			}
			else if (post_i >= 0) {
				GeomKeypoint& post = tl->keypoints[post_i];
				os.position = post.position;
			}

			pre_i = tl->FindPreOrientation(position);
			post_i = tl->FindPostOrientation(position);
			if (pre_i >= 0 && post_i >= 0) {
				ASSERT(pre_i < post_i);
				GeomKeypoint& pre = tl->keypoints[pre_i];
				GeomKeypoint& post = tl->keypoints[post_i];
				float pre_time = pre.frame_id / (float)prj.kps;
				float post_time = post.frame_id / (float)prj.kps;
				float f = (post_time > pre_time) ? (time - pre_time) / (post_time - pre_time) : 0.0f;
				f = min(max(f, 0.0f), 1.0f);
				f = apply_ease(pre.orientation_ease, f);
				os.orientation = Slerp(pre.orientation, post.orientation, f);
			}
			else if (pre_i >= 0) {
				GeomKeypoint& pre = tl->keypoints[pre_i];
				os.orientation = pre.orientation;
			}
			else if (post_i >= 0) {
				GeomKeypoint& post = tl->keypoints[post_i];
				os.orientation = post.orientation;
			}
		}
		Apply2DAnimation(o, position, time, prj.kps);
		ApplyMeshAnimation(o, position, time, prj.kps);
	}
	
	
	if (state->active_camera_obj_i >= 0) {
		GeomObjectState& os = state->objs[state->active_camera_obj_i];
		ASSERT(os.obj->IsCamera());
		if (os.obj->read_enabled) {
			GeomCamera& cam = state->GetProgram();
			cam.position = os.position;
			cam.orientation = os.orientation;
		}
	}
	
}

void GeomAnim::Update(double dt) {
	if (!is_playing) {
		return;
	}
	
	time += dt;
	
	GeomProject& prj = *state->prj;
	GeomScene& scene = state->GetActiveScene();
	
	double frame_time = 1.0 / state->prj->kps;
	position = time / frame_time;
	
	if (position < 0 || position >= scene.length) {
		is_playing = false;
		WhenSceneEnd();
		return;
	}
	
	ApplyAtPosition(position, time);
}

void GeomAnim::Reset() {
	is_playing = false;
	position = 0;
	time = 0;
}

void GeomAnim::Pause() {
	is_playing = false;
}

void GeomAnim::Play() {
	GeomScene& scene = state->GetActiveScene();
	if (position < 0 || position >= scene.length)
		Reset();
	is_playing = true;
	state->UpdateObjects();
}

void GeomAnim::Visit(Vis& v) {
	v VIS_(time)
	  VIS_(position)
	  VIS_(is_playing);
}

void GeomCamera::Visit(Vis& v) {
	v VISN(position)
	  VISN(orientation)
	  VIS_(distance)
	  VIS_(fov)
	  VIS_(scale);
}

INITIALIZER(GeomTransformType) {
	TYPED_STRING_HASHER(GeomTransform);
}
INITIALIZER_VFSEXT(GeomTimeline, "scene3d.timeline", "Scene3D|Core")
INITIALIZER_VFSEXT(GeomSceneTimeline, "scene3d.scene.timeline", "Scene3D|Core")
INITIALIZER_VFSEXT(GeomTransform, "scene3d.transform", "Scene3D|Core")
INITIALIZER_VFSEXT(GeomScript, "scene3d.script", "Scene3D|Core")
INITIALIZER_VFSEXT(GeomDynamicProperties, "scene3d.props", "Scene3D|Core")
INITIALIZER_VFSEXT(GeomBone, "scene3d.bone", "Scene3D|Core")
INITIALIZER_VFSEXT(GeomSkeleton, "scene3d.skeleton", "Scene3D|Core")
INITIALIZER_VFSEXT(GeomSkinWeights, "scene3d.skinweights", "Scene3D|Core")
INITIALIZER_VFSEXT(GeomEditableMesh, "scene3d.editable.mesh", "Scene3D|Core")
INITIALIZER_VFSEXT(Geom2DLayer, "scene3d.layer2d", "Scene3D|Core")
INITIALIZER_VFSEXT(Geom2DAnimation, "scene3d.layer2d.anim", "Scene3D|Core")
INITIALIZER_VFSEXT(GeomTextureEdit, "scene3d.texture.edit", "Scene3D|Core")
INITIALIZER_VFSEXT(GeomMeshAnimation, "scene3d.mesh.anim", "Scene3D|Core")
INITIALIZER_VFSEXT(GeomPointcloudEffectTransform, "scene3d.pointcloud.effect.transform", "Scene3D|Core")
INITIALIZER_VFSEXT(GeomPointcloudDataset, "scene3d.pointcloud.dataset", "Scene3D|Core")
INITIALIZER_VFSEXT(GeomObject, "scene3d.object", "Scene3D|Core")
INITIALIZER_VFSEXT(GeomDirectory, "scene3d.directory", "Scene3D|Core")
INITIALIZER_VFSEXT(GeomScene, "scene3d.scene", "Scene3D|Core")
INITIALIZER_VFSEXT(GeomProject, "scene3d.project", "Scene3D|Core")
INITIALIZER_VFSEXT(GeomCamera, "scene3d.camera", "Scene3D|Core")
INITIALIZER_VFSEXT(GeomWorldState, "scene3d.world", "Scene3D|Core")
INITIALIZER_VFSEXT(GeomAnim, "scene3d.anim", "Scene3D|Core")


END_UPP_NAMESPACE

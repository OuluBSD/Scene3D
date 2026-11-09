#include "ModelerApp.h"
#include <ByteVM/PyBindings.h>
#include <Scene3D/Exec/Exec.h>

#define IMAGECLASS ImagesImg
#define IMAGEFILE <ModelerApp/Images.iml>
#include <Draw/iml_source.h>

namespace Upp { class PyVM; }
void RegisterGeometry(Upp::PyVM& vm);

NAMESPACE_UPP

void ToolPanel::Init(Edit3D* e) {
	owner = e;
	layout_items.Clear();
	auto setup_opt = [&](Option& o, const char* label) {
		o.SetLabel(label);
		o.NoWantFocus();
		Add(o);
		layout_items.Add(&o);
	};
	auto setup_btn = [&](Button& b, const char* label) {
		b.SetLabel(label);
		b.NoWantFocus();
		Add(b);
		layout_items.Add(&b);
	};
	setup_opt(view, "View");
	setup_opt(obj_select, "Select");
	setup_opt(obj_move, "Move");
	setup_opt(obj_rotate, "Rotate");
	setup_opt(mesh_select, "Mesh");
	setup_opt(mesh_vertex, "Vtx");
	setup_opt(mesh_edge, "Edge");
	setup_opt(mesh_face, "Face");
	setup_opt(draw_point, "Point");
	setup_opt(draw_line, "Line");
	setup_opt(draw_face, "Poly");
	setup_btn(extrude, "Extrude");
	setup_btn(inset, "Inset");
	setup_btn(spin, "Spin");
	setup_btn(screw, "Screw");
	cam_view.NoWantFocus();
	cam_view.Add(0, "Active View");
	cam_view.Add(1, "View 1");
	cam_view.Add(2, "View 2");
	cam_view.Add(3, "View 3");
	cam_view.Add(4, "View 4");
	cam_view.SetData(0);
	Add(cam_view);
	layout_items.Add(&cam_view);
	setup_opt(cam_focus, "Cam Focus");
	setup_opt(cam_program, "Cam Program");
	setup_opt(cam_selected, "Cam Selected");

	view.WhenAction = [=] { if (owner) owner->SetEditTool(Edit3D::TOOL_VIEW); };
	obj_select.WhenAction = [=] { if (owner) owner->SetEditTool(Edit3D::TOOL_OBJ_SELECT); };
	obj_move.WhenAction = [=] { if (owner) owner->SetEditTool(Edit3D::TOOL_OBJ_MOVE); };
	obj_rotate.WhenAction = [=] { if (owner) owner->SetEditTool(Edit3D::TOOL_OBJ_ROTATE); };
	mesh_select.WhenAction = [=] { if (owner) owner->SetEditTool(Edit3D::TOOL_MESH_SELECT); };
	mesh_vertex.WhenAction = [=] {
		if (owner) {
			owner->mesh_select_mode = Edit3D::MESHSEL_VERTEX;
			owner->SetEditTool(Edit3D::TOOL_MESH_SELECT);
		}
	};
	mesh_edge.WhenAction = [=] {
		if (owner) {
			owner->mesh_select_mode = Edit3D::MESHSEL_EDGE;
			owner->SetEditTool(Edit3D::TOOL_MESH_SELECT);
		}
	};
	mesh_face.WhenAction = [=] {
		if (owner) {
			owner->mesh_select_mode = Edit3D::MESHSEL_FACE;
			owner->SetEditTool(Edit3D::TOOL_MESH_SELECT);
		}
	};
	draw_point.WhenAction = [=] { if (owner) owner->SetEditTool(Edit3D::TOOL_POINT); };
	draw_line.WhenAction = [=] { if (owner) owner->SetEditTool(Edit3D::TOOL_LINE); };
	draw_face.WhenAction = [=] { if (owner) owner->SetEditTool(Edit3D::TOOL_FACE); };
	extrude.WhenAction = [=] { if (owner) owner->ExtrudeMeshSelection(0.1); };
	inset.WhenAction = [=] { if (owner) owner->InsetMeshSelection(0.1); };
	spin.WhenAction = [=] { if (owner) owner->SpinMeshSelection(); };
	screw.WhenAction = [=] { if (owner) owner->ScrewMeshSelection(); };
	cam_focus.WhenAction = [=] { if (owner) owner->ApplyToolPanelCameraSource(CAMSRC_FOCUS); };
	cam_program.WhenAction = [=] { if (owner) owner->ApplyToolPanelCameraSource(CAMSRC_PROGRAM); };
	cam_selected.WhenAction = [=] { if (owner) owner->ApplyToolPanelCameraSource(CAMSRC_OBJECT); };
	Sync();
}

void ToolPanel::Sync() {
	if (!owner)
		return;
	view.Set((int)owner->edit_tool == (int)Edit3D::TOOL_VIEW);
	obj_select.Set((int)owner->edit_tool == (int)Edit3D::TOOL_OBJ_SELECT);
	obj_move.Set((int)owner->edit_tool == (int)Edit3D::TOOL_OBJ_MOVE);
	obj_rotate.Set((int)owner->edit_tool == (int)Edit3D::TOOL_OBJ_ROTATE);
	mesh_select.Set((int)owner->edit_tool == (int)Edit3D::TOOL_MESH_SELECT);
	mesh_vertex.Set(owner->mesh_select_mode == Edit3D::MESHSEL_VERTEX);
	mesh_edge.Set(owner->mesh_select_mode == Edit3D::MESHSEL_EDGE);
	mesh_face.Set(owner->mesh_select_mode == Edit3D::MESHSEL_FACE);
	draw_point.Set((int)owner->edit_tool == (int)Edit3D::TOOL_POINT);
	draw_line.Set((int)owner->edit_tool == (int)Edit3D::TOOL_LINE);
	draw_face.Set((int)owner->edit_tool == (int)Edit3D::TOOL_FACE);
	int view_idx = 0;
	Value v = cam_view.GetData();
	if (!IsNull(v))
		view_idx = (int)v;
	int rend_i = view_idx <= 0 ? owner->active_view : view_idx - 1;
	if (rend_i < 0 || rend_i >= 4)
		rend_i = owner->active_view;
	EditRendererBase* rend = owner->v0.rends[rend_i];
	if (rend) {
		cam_focus.Set(rend->cam_src == CAMSRC_FOCUS);
		cam_program.Set(rend->cam_src == CAMSRC_PROGRAM);
		cam_selected.Set(rend->cam_src == CAMSRC_OBJECT);
	}
}

void ToolPanel::Layout() {
	int pad = 4;
	int row_h = 22;
	int col_w = max(80, (GetSize().cx - pad * 2) / 4);
	int x = pad;
	int y = pad;
	auto place = [&](Ctrl& c) {
		c.SetRect(x, y, col_w - 4, row_h);
		x += col_w;
		if (x + col_w > GetSize().cx + 1) {
			x = pad;
			y += row_h + 2;
		}
	};
	for (Ctrl* c : layout_items) {
		if (c)
			place(*c);
	}
}

namespace {

static String ModelerAppConfigPath() {
	return ConfigFile("ModelerApp.view");
}

static void PruneEmptyObjects(GeomDirectory& dir) {
	for (int i = 0; i < dir.val.sub.GetCount(); ) {
		VfsValue& sub = dir.val.sub[i];
		if (IsVfsType(sub, AsTypeHash<GeomDirectory>())) {
			PruneEmptyObjects(sub.GetExt<GeomDirectory>());
			i++;
			continue;
		}
		if (IsVfsType(sub, AsTypeHash<GeomObject>())) {
			GeomObject& obj = sub.GetExt<GeomObject>();
			bool empty = obj.type == GeomObject::O_NULL &&
			             obj.name.IsEmpty() &&
			             obj.asset_ref.IsEmpty() &&
			             obj.pointcloud_ref.IsEmpty();
			GeomTimeline* tl = obj.FindTimeline();
			if (tl && !tl->keypoints.IsEmpty())
				empty = false;
			if (empty) {
				dir.val.sub.Remove(i);
				continue;
			}
		}
		i++;
	}
}

static void PruneEmptyObjects(GeomProject& prj) {
	for (GeomScene& scene : prj.val.Sub<GeomScene>())
		PruneEmptyObjects(scene);
}

void FillOctree(Octree& octree, const Vector<vec3>& points, int scale_level) {
	octree.Initialize(-3, 8);
	for (const vec3& p : points) {
		OctreeNode* node = octree.GetAddNode(p, scale_level);
		if (!node)
			continue;
		Pointcloud::Point& pt = node->Add<Pointcloud::Point>();
		pt.SetPosition(p);
	}
}

Vector<vec3> TransformPointsToWorld(const PointcloudPose& pose, const Vector<vec3>& points) {
	Vector<vec3> out;
	out.SetCount(points.GetCount());
	for (int i = 0; i < points.GetCount(); i++)
		out[i] = VectorTransform(points[i], pose.orientation) + pose.position;
	return out;
}

void UpdateCameraObject(GeomObject& cam, const PointcloudPose& pose) {
	GeomTimeline& tl = cam.GetTimeline();
	tl.keypoints.Clear();
	GeomKeypoint& kp = tl.keypoints.Add(0);
	kp.position = pose.position;
	kp.orientation = pose.orientation;
	kp.has_position = true;
	kp.has_orientation = true;
	if (GeomTransform* tr = cam.FindTransform()) {
		tr->position = pose.position;
		tr->orientation = pose.orientation;
	}
}

void UpdateCameraObjectRender(GeomObject& cam, const PointcloudPose& pose, bool flip_z) {
	PointcloudPose p = pose;
	if (flip_z) {
		p.orientation = p.orientation * MatQuat(YRotation(M_PI));
	}
	UpdateCameraObject(cam, p);
}

bool CompilePySource(const String& code, const String& filename, Vector<PyIR>& out_ir, String& err) {
	Tokenizer tk;
	tk.SkipPythonComments();
	if (!tk.Process(code, filename.IsEmpty() ? "<script>" : filename)) {
		err = "Tokenize failed";
		return false;
	}
	tk.NewlineToEndStatement();
	tk.CombineTokens();
	PyCompiler compiler(tk.GetTokens());
	try {
		compiler.Compile(out_ir);
	} catch (Exc& e) {
		err = e;
		return false;
	}
	return true;
}

bool RunPyIR(PyVM& vm, const Vector<PyIR>& ir, String& err) {
	Vector<PyIR> run;
	run.Append(ir);
	vm.SetIR(run);
	try {
		vm.Run();
	} catch (Exc& e) {
		err = e;
		return false;
	}
	return true;
}

Vector<String> SplitPathParts(const String& path) {
	Vector<String> parts;
	Vector<String> raw = Split(path, '/');
	for (const String& part : raw) {
		if (!part.IsEmpty())
			parts.Add(part);
	}
	return parts;
}

GeomDirectory* FindDirectoryByName(GeomDirectory& dir, const String& name) {
	for (auto& s : dir.val.sub) {
		if (!IsVfsType(s, AsTypeHash<GeomDirectory>()))
			continue;
		GeomDirectory& sub = s.GetExt<GeomDirectory>();
		if (sub.name == name || sub.val.id == name)
			return &sub;
	}
	return 0;
}

GeomDirectory* ResolveDirectoryPath(Edit3D& e, const String& path, bool create) {
	GeomScene& scene = e.GetActiveScene();
	Vector<String> parts = SplitPathParts(path);
	if (parts.IsEmpty())
		return &scene;
	if (parts[0] == scene.name || parts[0] == scene.val.id)
		parts.Remove(0);
	GeomDirectory* dir = &scene;
	for (int i = 0; i < parts.GetCount(); i++) {
		GeomDirectory* next = FindDirectoryByName(*dir, parts[i]);
		if (!next && create)
			next = &dir->GetAddDirectory(parts[i]);
		if (!next)
			return 0;
		dir = next;
	}
	return dir;
}

GeomObject* ResolveObjectPath(Edit3D& e, const String& path) {
	Vector<String> parts = SplitPathParts(path);
	if (parts.IsEmpty())
		return 0;
	GeomScene& scene = e.GetActiveScene();
	if (parts[0] == scene.name || parts[0] == scene.val.id)
		parts.Remove(0);
	if (parts.IsEmpty())
		return 0;
	String obj_name = parts.Pop();
	GeomDirectory* dir = ResolveDirectoryPath(e, Join(parts, "/"), false);
	if (!dir)
		return 0;
	return dir->FindObject(obj_name);
}

PyValue ModelerLog(const Vector<PyValue>& args, void* user_data) {
	if (args.GetCount() > 0)
		LOG(args[0].ToString());
	return PyValue::None();
}

PyValue ModelerTrace(const Vector<PyValue>& args, void* user_data) {
	if (args.GetCount() > 0)
		LOG(args[0].ToString());
	return PyValue::None();
}

PyValue ModelerGetTimer(const Vector<PyValue>& args, void* user_data) {
	Edit3D* e = (Edit3D*)user_data;
	if (!e)
		return PyValue(0);
	return PyValue((int64)e->script_timer.Elapsed());
}

PyValue ModelerExecExit(const Vector<PyValue>& args, void* user_data) {
	Edit3D* e = (Edit3D*)user_data;
	if (e)
		e->RequestExecutionExit();
	return PyValue::None();
}

PyValue ModelerRandom(const Vector<PyValue>& args, void* user_data) {
	if (args.GetCount() == 0)
		return PyValue(Randomf());
	if (args.GetCount() == 1)
		return PyValue(Randomf() * args[0].AsDouble());
	double minv = args[0].AsDouble();
	double maxv = args[1].AsDouble();
	return PyValue(minv + (maxv - minv) * Randomf());
}

PyValue ModelerDebugGeneratePointcloud(const Vector<PyValue>& args, void* user_data) {
	Edit3D* e = (Edit3D*)user_data;
	if (e)
		e->DebugGeneratePointcloud();
	return PyValue::None();
}

PyValue ModelerDebugSimulateObservation(const Vector<PyValue>& args, void* user_data) {
	Edit3D* e = (Edit3D*)user_data;
	if (e)
		e->DebugSimulateObservation();
	return PyValue::None();
}

PyValue ModelerDebugRunLocalization(const Vector<PyValue>& args, void* user_data) {
	Edit3D* e = (Edit3D*)user_data;
	if (e)
		e->DebugRunLocalization();
	return PyValue::None();
}

PyValue ModelerDebugSimulateControllerObservations(const Vector<PyValue>& args, void* user_data) {
	Edit3D* e = (Edit3D*)user_data;
	if (e)
		e->DebugSimulateControllerObservations();
	return PyValue::None();
}

PyValue ModelerDebugRunControllerLocalization(const Vector<PyValue>& args, void* user_data) {
	Edit3D* e = (Edit3D*)user_data;
	if (e)
		e->DebugRunControllerLocalization();
	return PyValue::None();
}

PyValue ModelerDebugClearSynthetic(const Vector<PyValue>& args, void* user_data) {
	Edit3D* e = (Edit3D*)user_data;
	if (e)
		e->DebugClearSynthetic();
	return PyValue::None();
}

PyValue ModelerDebugRunFullSynthetic(const Vector<PyValue>& args, void* user_data) {
	Edit3D* e = (Edit3D*)user_data;
	if (e)
		e->RunSyntheticPointcloudSimDialog();
	return PyValue::None();
}

PyValue ModelerSetPosition(const Vector<PyValue>& args, void* user_data) {
	Edit3D* e = (Edit3D*)user_data;
	if (!e || args.GetCount() < 4)
		return PyValue::False();
	String path = args[0].ToString();
	GeomObject* obj = ResolveObjectPath(*e, path);
	if (!obj)
		return PyValue::False();
	GeomTransform& tr = obj->GetTransform();
	tr.position = vec3((float)args[1].AsDouble(),
	                   (float)args[2].AsDouble(),
	                   (float)args[3].AsDouble());
	e->state->UpdateObjects();
	e->v0.RefreshAll();
	return PyValue::True();
}

PyValue ModelerSetOrientation(const Vector<PyValue>& args, void* user_data) {
	Edit3D* e = (Edit3D*)user_data;
	if (!e || args.GetCount() < 5)
		return PyValue::False();
	String path = args[0].ToString();
	GeomObject* obj = ResolveObjectPath(*e, path);
	if (!obj)
		return PyValue::False();
	quat q((float)args[1].AsDouble(),
	       (float)args[2].AsDouble(),
	       (float)args[3].AsDouble(),
	       (float)args[4].AsDouble());
	q.Normalize();
	GeomTransform& tr = obj->GetTransform();
	tr.orientation = q;
	e->state->UpdateObjects();
	e->v0.RefreshAll();
	return PyValue::True();
}

PyValue ModelerGetPosition(const Vector<PyValue>& args, void* user_data) {
	Edit3D* e = (Edit3D*)user_data;
	if (!e || args.GetCount() < 1)
		return PyValue::None();
	String path = args[0].ToString();
	GeomObject* obj = ResolveObjectPath(*e, path);
	if (!obj)
		return PyValue::None();
	vec3 pos = vec3(0);
	if (GeomTransform* tr = obj->FindTransform()) {
		pos = tr->position;
	}
	else if (const GeomObjectState* os = e->state->FindObjectStateByKey(obj->key)) {
		pos = os->position;
	}
	Vector<PyValue> out;
	out.Add(pos[0]);
	out.Add(pos[1]);
	out.Add(pos[2]);
	return PyValue::FromVector(out, true);
}

PyValue ModelerAddTransformKeyframe(const Vector<PyValue>& args, void* user_data) {
	Edit3D* e = (Edit3D*)user_data;
	if (!e || args.GetCount() < 2)
		return PyValue::False();
	String path = args[0].ToString();
	int frame = (int)args[1].AsInt();
	bool pos = args.GetCount() > 2 ? args[2].IsTrue() : true;
	bool ori = args.GetCount() > 3 ? args[3].IsTrue() : true;
	GeomObject* obj = ResolveObjectPath(*e, path);
	if (!obj)
		return PyValue::False();
	GeomTimeline& tl = obj->GetTimeline();
	GeomKeypoint& kp = tl.GetAddKeypoint(frame);
	kp.frame_id = frame;
	if (const GeomObjectState* os = e->state->FindObjectStateByKey(obj->key)) {
		if (pos) kp.position = os->position;
		if (ori) kp.orientation = os->orientation;
	}
	else if (GeomTransform* tr = obj->FindTransform()) {
		if (pos) kp.position = tr->position;
		if (ori) kp.orientation = tr->orientation;
	}
	if (pos) kp.has_position = true;
	if (ori) kp.has_orientation = true;
	e->RefrehRenderers();
	return PyValue::True();
}

PyValue ModelerSetTimelineExpanded(const Vector<PyValue>& args, void* user_data) {
	Edit3D* e = (Edit3D*)user_data;
	if (!e || !e->state || args.GetCount() < 2)
		return PyValue::False();
	String path = args[0].ToString();
	bool expand = args[1].IsTrue();
	GeomObject* obj = ResolveObjectPath(*e, path);
	if (!obj)
		return PyValue::False();
	int idx = e->v0.timeline_expanded.Find(obj->key);
	if (expand) {
		if (idx < 0)
			e->v0.timeline_expanded.Add(obj->key);
	}
	else {
		if (idx >= 0)
			e->v0.timeline_expanded.Remove(idx);
	}
	e->v0.TimelineData();
	return PyValue::True();
}

PyValue ModelerGetGizmoPixels(const Vector<PyValue>& args, void* user_data) {
	Edit3D* e = (Edit3D*)user_data;
	if (!e)
		return PyValue::None();
	return PyValue(e->render_ctx.gizmo_pixels);
}

PyValue ModelerSetRendererVersion(const Vector<PyValue>& args, void* user_data) {
	Edit3D* e = (Edit3D*)user_data;
	if (!e || args.GetCount() < 2)
		return PyValue::False();
	int view_i = (int)args[0].AsInt();
	int version = (int)args[1].AsInt();
	if (view_i < 0 || view_i >= 4)
		return PyValue::False();
	if (version < 1 || version > 3)
		return PyValue::False();
	e->v0.SetRendererVersion(view_i, version);
	return PyValue::True();
}

PyValue ModelerGetRendererVersion(const Vector<PyValue>& args, void* user_data) {
	Edit3D* e = (Edit3D*)user_data;
	if (!e || args.GetCount() < 1)
		return PyValue::None();
	int view_i = (int)args[0].AsInt();
	if (view_i < 0 || view_i >= 4)
		return PyValue::None();
	return PyValue(e->v0.rend_version[view_i]);
}

PyValue ModelerSetRendererWireframe(const Vector<PyValue>& args, void* user_data) {
	Edit3D* e = (Edit3D*)user_data;
	if (!e || args.GetCount() < 2)
		return PyValue::False();
	int view_i = (int)args[0].AsInt();
	bool enable = args[1].IsTrue();
	if (view_i < 0 || view_i >= 4)
		return PyValue::False();
	EditRendererBase* rend = e->v0.rends[view_i];
	if (!rend)
		return PyValue::False();
	rend->SetWireframeOnly(enable);
	e->RefrehRenderers();
	return PyValue::True();
}

PyValue ModelerGetRendererWireframe(const Vector<PyValue>& args, void* user_data) {
	Edit3D* e = (Edit3D*)user_data;
	if (!e || args.GetCount() < 1)
		return PyValue::None();
	int view_i = (int)args[0].AsInt();
	if (view_i < 0 || view_i >= 4)
		return PyValue::None();
	EditRendererBase* rend = e->v0.rends[view_i];
	if (!rend)
		return PyValue::None();
	return PyValue(rend->IsWireframeOnly());
}

PyValue ModelerSetShowGrid(const Vector<PyValue>& args, void* user_data) {
	Edit3D* e = (Edit3D*)user_data;
	if (!e || args.GetCount() < 1)
		return PyValue::False();
	e->conf.show_grid = args[0].IsTrue();
	e->RefrehRenderers();
	return PyValue::True();
}

PyValue ModelerGetShowGrid(const Vector<PyValue>& args, void* user_data) {
	Edit3D* e = (Edit3D*)user_data;
	if (!e)
		return PyValue::None();
	return PyValue(e->conf.show_grid);
}

PyValue ModelerSetHudVisible(const Vector<PyValue>& args, void* user_data) {
	Edit3D* e = (Edit3D*)user_data;
	if (!e || args.GetCount() < 1)
		return PyValue::False();
	e->show_hud = args[0].IsTrue();
	e->RefrehRenderers();
	return PyValue::True();
}

PyValue ModelerGetHudVisible(const Vector<PyValue>& args, void* user_data) {
	Edit3D* e = (Edit3D*)user_data;
	if (!e)
		return PyValue::None();
	return PyValue(e->show_hud);
}

PyValue ModelerCreateObject(const Vector<PyValue>& args, void* user_data) {
	Edit3D* e = (Edit3D*)user_data;
	if (!e || args.GetCount() < 2)
		return PyValue::False();
	String path = args[0].ToString();
	String type = ToLower(args[1].ToString());
	Vector<String> parts = SplitPathParts(path);
	if (parts.IsEmpty())
		return PyValue::False();
	GeomScene& scene = e->GetActiveScene();
	if (parts[0] == scene.name || parts[0] == scene.val.id)
		parts.Remove(0);
	if (parts.IsEmpty())
		return PyValue::False();
	String obj_name = parts.Pop();
	GeomDirectory* dir = ResolveDirectoryPath(*e, Join(parts, "/"), true);
	if (!dir)
		return PyValue::False();
	GeomObject* obj = 0;
	if (type == "camera")
		obj = &dir->GetAddCamera(obj_name);
	else if (type == "model")
		obj = &dir->GetAddModel(obj_name);
	else if (type == "pointcloud" || type == "octree")
		obj = &dir->GetAddOctree(obj_name);
	if (!obj)
		return PyValue::False();
	e->state->UpdateObjects();
	e->RefreshData();
	return PyValue::True();
}

PyValue ModelerCreateDirectory(const Vector<PyValue>& args, void* user_data) {
	Edit3D* e = (Edit3D*)user_data;
	if (!e || args.GetCount() < 1)
		return PyValue::False();
	String path = args[0].ToString();
	GeomDirectory* dir = ResolveDirectoryPath(*e, path, true);
	if (!dir)
		return PyValue::False();
	e->state->UpdateObjects();
	e->RefreshData();
	return PyValue::True();
}

PyValue ModelerGetProjectDir(const Vector<PyValue>& args, void* user_data) {
	Edit3D* e = (Edit3D*)user_data;
	if (!e)
		return PyValue::None();
	return PyValue(e->GetProjectDir());
}

static hash_t HashGeomDirectory() { static hash_t h = TypedStringHasher<GeomDirectory>("GeomDirectory"); return h; }
static hash_t HashGeomScene() { static hash_t h = TypedStringHasher<GeomScene>("GeomScene"); return h; }
static hash_t HashGeomObject() { static hash_t h = TypedStringHasher<GeomObject>("GeomObject"); return h; }
static hash_t HashGeomDataset() { static hash_t h = TypedStringHasher<GeomPointcloudDataset>("GeomPointcloudDataset"); return h; }

static String GetNodeName(VfsValue& n) {
	if (IsVfsType(n, HashGeomDirectory())) {
		const GeomDirectory& dir = n.GetExt<GeomDirectory>();
		return dir.name.IsEmpty() ? n.id : dir.name;
	}
	if (IsVfsType(n, HashGeomScene())) {
		const GeomScene& scene = n.GetExt<GeomScene>();
		return scene.name.IsEmpty() ? n.id : scene.name;
	}
	if (IsVfsType(n, HashGeomObject())) {
		const GeomObject& obj = n.GetExt<GeomObject>();
		return obj.name.IsEmpty() ? n.id : obj.name;
	}
	if (IsVfsType(n, HashGeomDataset())) {
		const GeomPointcloudDataset& ds = n.GetExt<GeomPointcloudDataset>();
		return ds.name.IsEmpty() ? n.id : ds.name;
	}
	return n.id;
}

static GeomTransform* GetNodeTransform(VfsValue& n) {
	if (IsVfsType(n, HashGeomObject()))
		return &n.GetExt<GeomObject>().GetTransform();
	if (IsVfsType(n, HashGeomScene()))
		return &n.GetExt<GeomScene>().GetTransform();
	if (IsVfsType(n, HashGeomDirectory()))
		return &n.GetExt<GeomDirectory>().GetTransform();
	return nullptr;
}

static GeomObject* GetNodeObject(VfsValue& n) {
	return IsVfsType(n, HashGeomObject()) ? &n.GetExt<GeomObject>() : nullptr;
}

static GeomDirectory* GetNodeDirectory(VfsValue& n) {
	if (IsVfsType(n, HashGeomScene()))
		return &n.GetExt<GeomScene>();
	if (IsVfsType(n, HashGeomDirectory()))
		return &n.GetExt<GeomDirectory>();
	return nullptr;
}

static bool PyValueToVec3(const PyValue& v, vec3& out) {
	if (v.IsNumber()) {
		double d = v.AsDouble();
		out = vec3(d, d, d);
		return true;
	}
	int n = v.GetCount();
	if (n >= 3) {
		out[0] = v.GetItem(0).AsDouble();
		out[1] = v.GetItem(1).AsDouble();
		out[2] = v.GetItem(2).AsDouble();
		return true;
	}
	return false;
}

static PyValue MakeVec3Value(const vec3& v) {
	PyValue t = PyValue::Tuple();
	t.Add(PyValue(v[0]));
	t.Add(PyValue(v[1]));
	t.Add(PyValue(v[2]));
	return t;
}

static GeomDynamicProperties* FindDynamicProps(VfsValue& node) {
	for (auto& sub : node.sub) {
		if (IsVfsType(sub, AsTypeHash<GeomDynamicProperties>()) && sub.id == "props")
			return &sub.GetExt<GeomDynamicProperties>();
	}
	return nullptr;
}

static GeomDynamicProperties& GetDynamicProps(VfsValue& node) {
	return node.GetAdd<GeomDynamicProperties>("props");
}

static PyValue ValueToPyValue(const Value& v) {
	return PyValue::FromValue(v);
}

static Value PyValueToValue(const PyValue& v) {
	return v.ToValue();
}

struct DisplayObjectProxy : PyUserData {
	Edit3D* app = nullptr;
	VfsValue* node = nullptr;
	PyVM* vm = nullptr;

	DisplayObjectProxy(Edit3D* a, VfsValue* n, PyVM* v) : app(a), node(n), vm(v) {}
	String GetTypeName() const override { return "DisplayObject"; }

	PyValue GetAttr(const String& name) override;
	bool SetAttr(const String& name, const PyValue& v) override;
};

static PyValue MakeDisplayObject(Edit3D* app, VfsValue* node, PyVM* vm) {
	if (!app || !node || !vm)
		return PyValue::None();
	return PyValue(new DisplayObjectProxy(app, node, vm));
}

static VfsValue* FindChildByName(VfsValue& node, const String& name) {
	for (auto& sub : node.sub) {
		if (!IsVfsType(sub, HashGeomDirectory()) && !IsVfsType(sub, HashGeomObject()) && !IsVfsType(sub, HashGeomScene()))
			continue;
		if (GetNodeName(sub) == name || sub.id == name)
			return &sub;
	}
	return nullptr;
}

static VfsValue* ResolvePath(VfsValue& root, const String& path) {
	String p = path;
	if (p.StartsWith("/"))
		p = p.Mid(1);
	if (p.IsEmpty())
		return &root;
	Vector<String> parts = Split(p, '/');
	VfsValue* cur = &root;
	for (const String& part : parts) {
		if (part.IsEmpty())
			continue;
		VfsValue* next = FindChildByName(*cur, part);
		if (!next)
			return nullptr;
		cur = next;
	}
	return cur;
}

static bool NodeTypeMatches(VfsValue& n, const String& type) {
	String t = ToLower(type);
	if (t.IsEmpty())
		return false;
	if (t == "directory" || t == "dir")
		return IsVfsType(n, HashGeomDirectory()) || IsVfsType(n, HashGeomScene());
	if (t == "model") {
		if (!IsVfsType(n, HashGeomObject()))
			return false;
		return n.GetExt<GeomObject>().type == GeomObject::O_MODEL;
	}
	if (t == "camera") {
		if (!IsVfsType(n, HashGeomObject()))
			return false;
		return n.GetExt<GeomObject>().type == GeomObject::O_CAMERA;
	}
	if (t == "pointcloud" || t == "octree") {
		if (!IsVfsType(n, HashGeomObject()))
			return false;
		return n.GetExt<GeomObject>().type == GeomObject::O_OCTREE;
	}
	if (t == "dataset" || t == "pointcloud_dataset")
		return IsVfsType(n, HashGeomDataset());
	if (t == "object")
		return IsVfsType(n, HashGeomObject());
	return false;
}

static int FindChildIndex(VfsValue& parent, VfsValue* child) {
	if (!child)
		return -1;
	for (int i = 0; i < parent.sub.GetCount(); i++)
		if (&parent.sub[i] == child)
			return i;
	return -1;
}

static bool ReorderChild(VfsValue& parent, VfsValue* child, int new_index) {
	int old_index = FindChildIndex(parent, child);
	if (old_index < 0)
		return false;
	if (new_index < 0)
		new_index = 0;
	if (new_index >= parent.sub.GetCount())
		new_index = parent.sub.GetCount() - 1;
	if (new_index == old_index)
		return true;
	parent.sub.Move(old_index, new_index);
	for (auto& sub : parent.sub)
		sub.owner = &parent;
	return true;
}

static PyValue DisplayObject_AddChild(const Vector<PyValue>& args, void* user_data) {
	if (args.GetCount() < 2 || !args[0].IsUserData())
		return PyValue::None();
	DisplayObjectProxy& self = (DisplayObjectProxy&)args[0].GetUserData();
	String type = args[1].ToString();
	String name = args.GetCount() > 2 ? args[2].ToString() : String();
	if (!self.app || !self.node)
		return PyValue::None();
	GeomDirectory* dir = GetNodeDirectory(*self.node);
	if (!dir)
		return PyValue::None();
	if (name.IsEmpty())
		name = type.IsEmpty() ? "node" : type;
	VfsValue* out = nullptr;
	String t = ToLower(type);
	if (t == "directory" || t == "dir")
		out = &dir->GetAddDirectory(name).val;
	else if (t == "model")
		out = &dir->GetAddModel(name).val;
	else if (t == "camera")
		out = &dir->GetAddCamera(name).val;
	else if (t == "pointcloud" || t == "octree")
		out = &dir->GetAddOctree(name).val;
	else if (t == "dataset" || t == "pointcloud_dataset") {
		GeomPointcloudDataset& ds = dir->GetAddPointcloudDataset(name);
		out = &ds.val;
	}
	if (!out)
		return PyValue::None();
	self.app->state->UpdateObjects();
	self.app->RefreshData();
	return MakeDisplayObject(self.app, out, self.vm);
}

static PyValue DisplayObject_GetChildByName(const Vector<PyValue>& args, void* user_data) {
	if (args.GetCount() < 2 || !args[0].IsUserData())
		return PyValue::None();
	DisplayObjectProxy& self = (DisplayObjectProxy&)args[0].GetUserData();
	if (!self.app || !self.node)
		return PyValue::None();
	String name = args[1].ToString();
	VfsValue* found = FindChildByName(*self.node, name);
	return MakeDisplayObject(self.app, found, self.vm);
}

static PyValue DisplayObject_GetChildrenByType(const Vector<PyValue>& args, void* user_data) {
	if (args.GetCount() < 2 || !args[0].IsUserData())
		return PyValue::None();
	DisplayObjectProxy& self = (DisplayObjectProxy&)args[0].GetUserData();
	if (!self.app || !self.node)
		return PyValue::None();
	String type = args[1].ToString();
	PyValue list = PyValue::List();
	for (auto& sub : self.node->sub) {
		if (NodeTypeMatches(sub, type))
			list.Add(MakeDisplayObject(self.app, &sub, self.vm));
	}
	return list;
}

static PyValue DisplayObject_GetChildIndex(const Vector<PyValue>& args, void* user_data) {
	if (args.GetCount() < 2 || !args[0].IsUserData())
		return PyValue::None();
	DisplayObjectProxy& self = (DisplayObjectProxy&)args[0].GetUserData();
	if (!self.node)
		return PyValue::None();
	if (!args[1].IsUserData())
		return PyValue::None();
	DisplayObjectProxy& child = (DisplayObjectProxy&)args[1].GetUserData();
	if (!child.node)
		return PyValue::None();
	return PyValue(FindChildIndex(*self.node, child.node));
}

static PyValue DisplayObject_SetChildIndex(const Vector<PyValue>& args, void* user_data) {
	if (args.GetCount() < 3 || !args[0].IsUserData())
		return PyValue::None();
	DisplayObjectProxy& self = (DisplayObjectProxy&)args[0].GetUserData();
	if (!self.app || !self.node)
		return PyValue::None();
	if (!args[1].IsUserData())
		return PyValue::None();
	DisplayObjectProxy& child = (DisplayObjectProxy&)args[1].GetUserData();
	if (!child.node)
		return PyValue::None();
	int idx = (int)args[2].AsInt();
	if (!ReorderChild(*self.node, child.node, idx))
		return PyValue::False();
	self.app->RefreshData();
	return PyValue::True();
}

static PyValue DisplayObject_On(const Vector<PyValue>& args, void* user_data) {
	if (args.GetCount() < 3 || !args[0].IsUserData())
		return PyValue::None();
	DisplayObjectProxy& self = (DisplayObjectProxy&)args[0].GetUserData();
	if (!self.app || !self.node)
		return PyValue::None();
	String ev = args[1].ToString();
	PyValue fn = args[2];
	self.app->AddScriptEventHandler(ev, self.vm, self.node, fn);
	return PyValue::True();
}

static PyValue DisplayObject_OnFrame(const Vector<PyValue>& args, void* user_data) {
	if (args.GetCount() < 2 || !args[0].IsUserData())
		return PyValue::None();
	DisplayObjectProxy& self = (DisplayObjectProxy&)args[0].GetUserData();
	if (!self.app || !self.node)
		return PyValue::None();
	PyValue fn = args[1];
	self.app->AddScriptEventHandler("enterFrame", self.vm, self.node, fn);
	return PyValue::True();
}

static bool SetTimelinePosition(Edit3D* app, int frame, bool play) {
	if (!app || !app->anim || !app->prj)
		return false;
	app->anim->position = frame;
	double frame_time = 1.0 / max(1, app->prj->kps);
	app->anim->time = frame * frame_time;
	app->anim->is_playing = play;
	app->RefrehRenderers();
	return true;
}

static PyValue DisplayObject_GotoAndPlay(const Vector<PyValue>& args, void* user_data) {
	if (args.GetCount() < 2 || !args[0].IsUserData())
		return PyValue::None();
	DisplayObjectProxy& self = (DisplayObjectProxy&)args[0].GetUserData();
	int frame = (int)args[1].AsInt();
	return PyValue(SetTimelinePosition(self.app, frame, true));
}

static PyValue DisplayObject_GotoAndStop(const Vector<PyValue>& args, void* user_data) {
	if (args.GetCount() < 2 || !args[0].IsUserData())
		return PyValue::None();
	DisplayObjectProxy& self = (DisplayObjectProxy&)args[0].GetUserData();
	int frame = (int)args[1].AsInt();
	return PyValue(SetTimelinePosition(self.app, frame, false));
}

static PyValue DisplayObject_Remove(const Vector<PyValue>& args, void* user_data) {
	if (args.GetCount() < 1 || !args[0].IsUserData())
		return PyValue::None();
	DisplayObjectProxy& self = (DisplayObjectProxy&)args[0].GetUserData();
	if (!self.app || !self.node)
		return PyValue::None();
	if (!self.node->owner)
		return PyValue::None();
	self.node->owner->Remove(self.node);
	self.app->state->UpdateObjects();
	self.app->RefreshData();
	return PyValue::True();
}

static PyValue DisplayObject_Find(const Vector<PyValue>& args, void* user_data) {
	if (args.GetCount() < 2 || !args[0].IsUserData())
		return PyValue::None();
	DisplayObjectProxy& self = (DisplayObjectProxy&)args[0].GetUserData();
	if (!self.app || !self.node)
		return PyValue::None();
	String path = args[1].ToString();
	VfsValue* found = ResolvePath(*self.node, path);
	return MakeDisplayObject(self.app, found, self.vm);
}

static PyValue DisplayObject_Children(const Vector<PyValue>& args, void* user_data) {
	if (args.GetCount() < 1 || !args[0].IsUserData())
		return PyValue::None();
	DisplayObjectProxy& self = (DisplayObjectProxy&)args[0].GetUserData();
	if (!self.app || !self.node)
		return PyValue::None();
	PyValue list = PyValue::List();
	for (auto& sub : self.node->sub) {
		if (!IsVfsType(sub, HashGeomDirectory()) && !IsVfsType(sub, HashGeomObject()) && !IsVfsType(sub, HashGeomScene()))
			continue;
		list.Add(MakeDisplayObject(self.app, &sub, self.vm));
	}
	return list;
}

PyValue DisplayObjectProxy::GetAttr(const String& name) {
	if (!node)
		return PyValue::None();
	GeomTransform* tr = GetNodeTransform(*node);
	if (name == "name")
		return PyValue(GetNodeName(*node));
	if (name == "x" && tr)
		return PyValue(tr->position[0]);
	if (name == "y" && tr)
		return PyValue(tr->position[1]);
	if (name == "z" && tr)
		return PyValue(tr->position[2]);
	if (name == "rotation" && tr)
		return MakeVec3Value(GetQuatAxes(tr->orientation));
	if (name == "scale" && tr)
		return MakeVec3Value(tr->scale);
	if (name == "visible") {
		if (GeomObject* obj = GetNodeObject(*node))
			return PyValue(obj->is_visible);
	}
	if (name == "addChild")
		return PyValue::BoundMethod(PyValue::Function("addChild", DisplayObject_AddChild, nullptr), PyValue(this));
	if (name == "remove")
		return PyValue::BoundMethod(PyValue::Function("remove", DisplayObject_Remove, nullptr), PyValue(this));
	if (name == "find")
		return PyValue::BoundMethod(PyValue::Function("find", DisplayObject_Find, nullptr), PyValue(this));
	if (name == "children")
		return PyValue::BoundMethod(PyValue::Function("children", DisplayObject_Children, nullptr), PyValue(this));
	if (name == "getChildByName")
		return PyValue::BoundMethod(PyValue::Function("getChildByName", DisplayObject_GetChildByName, nullptr), PyValue(this));
	if (name == "getChildrenByType")
		return PyValue::BoundMethod(PyValue::Function("getChildrenByType", DisplayObject_GetChildrenByType, nullptr), PyValue(this));
	if (name == "getChildIndex")
		return PyValue::BoundMethod(PyValue::Function("getChildIndex", DisplayObject_GetChildIndex, nullptr), PyValue(this));
	if (name == "setChildIndex")
		return PyValue::BoundMethod(PyValue::Function("setChildIndex", DisplayObject_SetChildIndex, nullptr), PyValue(this));
	if (name == "on")
		return PyValue::BoundMethod(PyValue::Function("on", DisplayObject_On, nullptr), PyValue(this));
	if (name == "onFrame" || name == "onEnterFrame")
		return PyValue::BoundMethod(PyValue::Function("onFrame", DisplayObject_OnFrame, nullptr), PyValue(this));
	if (name == "gotoAndPlay")
		return PyValue::BoundMethod(PyValue::Function("gotoAndPlay", DisplayObject_GotoAndPlay, nullptr), PyValue(this));
	if (name == "gotoAndStop")
		return PyValue::BoundMethod(PyValue::Function("gotoAndStop", DisplayObject_GotoAndStop, nullptr), PyValue(this));
	if (GeomDynamicProperties* props = FindDynamicProps(*node)) {
		int idx = props->props.Find(name);
		if (idx >= 0)
			return ValueToPyValue(props->props[idx]);
	}
	return PyValue::None();
}

bool DisplayObjectProxy::SetAttr(const String& name, const PyValue& v) {
	if (!node)
		return false;
	GeomTransform* tr = GetNodeTransform(*node);
	if (tr) {
		if (name == "x") { tr->position[0] = v.AsDouble(); if (app) { app->state->UpdateObjects(); app->RefrehRenderers(); } return true; }
		else if (name == "y") { tr->position[1] = v.AsDouble(); if (app) { app->state->UpdateObjects(); app->RefrehRenderers(); } return true; }
		else if (name == "z") { tr->position[2] = v.AsDouble(); if (app) { app->state->UpdateObjects(); app->RefrehRenderers(); } return true; }
		else if (name == "rotation") {
			vec3 axes;
			if (PyValueToVec3(v, axes)) {
				tr->orientation = AxesQuat(axes);
				if (app) { app->state->UpdateObjects(); app->RefrehRenderers(); }
				return true;
			}
		}
		else if (name == "scale") {
			vec3 scale;
			if (PyValueToVec3(v, scale)) {
				tr->scale = scale;
				if (app) { app->state->UpdateObjects(); app->RefrehRenderers(); }
				return true;
			}
		}
	}
	if (name == "onFrame" || name == "onEnterFrame") {
		if (app)
			app->AddScriptEventHandler("enterFrame", vm, node, v);
		return true;
	}
	if (name == "visible") {
		if (GeomObject* obj = GetNodeObject(*node))
			obj->is_visible = v.IsTrue();
		if (GeomDirectory* dir = GetNodeDirectory(*node)) {
			bool vis = v.IsTrue();
			for (auto& sub : dir->val.sub) {
				if (IsVfsType(sub, HashGeomObject()))
					sub.GetExt<GeomObject>().is_visible = vis;
			}
		}
	}
	else if (GeomDynamicProperties* props = node ? &GetDynamicProps(*node) : nullptr) {
		props->props.GetAdd(name) = PyValueToValue(v);
	}
	if (app) {
		app->state->UpdateObjects();
		app->RefrehRenderers();
	}
	return true;
}

struct StageProxy : PyUserData {
	Edit3D* app = nullptr;
	PyVM* vm = nullptr;
	StageProxy(Edit3D* a, PyVM* v) : app(a), vm(v) {}
	String GetTypeName() const override { return "Stage"; }
	PyValue GetAttr(const String& name) override;
	bool SetAttr(const String& name, const PyValue& v) override;
};

static PyValue Stage_Find(const Vector<PyValue>& args, void* user_data) {
	if (args.GetCount() < 2 || !args[0].IsUserData())
		return PyValue::None();
	StageProxy& self = (StageProxy&)args[0].GetUserData();
	if (!self.app)
		return PyValue::None();
	GeomScene& scene = self.app->state->GetActiveScene();
	String path = args[1].ToString();
	VfsValue* found = ResolvePath(scene.val, path);
	return MakeDisplayObject(self.app, found, self.vm);
}

static PyValue Stage_Children(const Vector<PyValue>& args, void* user_data) {
	if (args.GetCount() < 1 || !args[0].IsUserData())
		return PyValue::None();
	StageProxy& self = (StageProxy&)args[0].GetUserData();
	if (!self.app)
		return PyValue::None();
	VfsValue* root = &self.app->state->GetActiveScene().val;
	if (args.GetCount() > 1 && args[1].IsUserData())
		root = ((DisplayObjectProxy&)args[1].GetUserData()).node;
	PyValue list = PyValue::List();
	if (root) {
		for (auto& sub : root->sub) {
			if (!IsVfsType(sub, HashGeomDirectory()) && !IsVfsType(sub, HashGeomObject()) && !IsVfsType(sub, HashGeomScene()))
				continue;
			list.Add(MakeDisplayObject(self.app, &sub, self.vm));
		}
	}
	return list;
}

static PyValue Stage_Create(const Vector<PyValue>& args, void* user_data) {
	if (args.GetCount() < 2 || !args[0].IsUserData())
		return PyValue::None();
	StageProxy& self = (StageProxy&)args[0].GetUserData();
	if (!self.app)
		return PyValue::None();
	String type = args[1].ToString();
	String name = args.GetCount() > 2 ? args[2].ToString() : String();
	VfsValue* parent = &self.app->state->GetActiveScene().val;
	if (args.GetCount() > 3 && args[3].IsUserData())
		parent = ((DisplayObjectProxy&)args[3].GetUserData()).node;
	if (!parent)
		return PyValue::None();
	GeomDirectory* dir = GetNodeDirectory(*parent);
	if (!dir)
		return PyValue::None();
	if (name.IsEmpty())
		name = type.IsEmpty() ? "node" : type;
	VfsValue* out = nullptr;
	String t = ToLower(type);
	if (t == "directory" || t == "dir")
		out = &dir->GetAddDirectory(name).val;
	else if (t == "model")
		out = &dir->GetAddModel(name).val;
	else if (t == "camera")
		out = &dir->GetAddCamera(name).val;
	else if (t == "pointcloud" || t == "octree")
		out = &dir->GetAddOctree(name).val;
	else if (t == "dataset" || t == "pointcloud_dataset") {
		GeomPointcloudDataset& ds = dir->GetAddPointcloudDataset(name);
		out = &ds.val;
	}
	if (!out)
		return PyValue::None();
	self.app->state->UpdateObjects();
	self.app->RefreshData();
	return MakeDisplayObject(self.app, out, self.vm);
}

static PyValue Stage_Goto(const Vector<PyValue>& args, void* user_data) {
	if (args.GetCount() < 2 || !args[0].IsUserData())
		return PyValue::None();
	StageProxy& self = (StageProxy&)args[0].GetUserData();
	int frame = (int)args[1].AsInt();
	return PyValue(SetTimelinePosition(self.app, frame, self.app && self.app->anim ? self.app->anim->is_playing : false));
}

static PyValue Stage_GotoAndPlay(const Vector<PyValue>& args, void* user_data) {
	if (args.GetCount() < 2 || !args[0].IsUserData())
		return PyValue::None();
	StageProxy& self = (StageProxy&)args[0].GetUserData();
	int frame = (int)args[1].AsInt();
	return PyValue(SetTimelinePosition(self.app, frame, true));
}

static PyValue Stage_GotoAndStop(const Vector<PyValue>& args, void* user_data) {
	if (args.GetCount() < 2 || !args[0].IsUserData())
		return PyValue::None();
	StageProxy& self = (StageProxy&)args[0].GetUserData();
	int frame = (int)args[1].AsInt();
	return PyValue(SetTimelinePosition(self.app, frame, false));
}

static PyValue Stage_On(const Vector<PyValue>& args, void* user_data) {
	if (args.GetCount() < 3 || !args[0].IsUserData())
		return PyValue::None();
	StageProxy& self = (StageProxy&)args[0].GetUserData();
	if (!self.app)
		return PyValue::None();
	String ev = args[1].ToString();
	PyValue fn = args[2];
	self.app->AddScriptEventHandler(ev, self.vm, nullptr, fn);
	return PyValue::True();
}

static PyValue Stage_OnFrame(const Vector<PyValue>& args, void* user_data) {
	if (args.GetCount() < 2 || !args[0].IsUserData())
		return PyValue::None();
	StageProxy& self = (StageProxy&)args[0].GetUserData();
	if (!self.app)
		return PyValue::None();
	PyValue fn = args[1];
	self.app->AddScriptEventHandler("enterFrame", self.vm, nullptr, fn);
	return PyValue::True();
}

static PyValue Stage_Exit(const Vector<PyValue>& args, void* user_data) {
	if (args.GetCount() < 1 || !args[0].IsUserData())
		return PyValue::None();
	StageProxy& self = (StageProxy&)args[0].GetUserData();
	if (self.app)
		self.app->RequestExecutionExit();
	return PyValue::None();
}

PyValue StageProxy::GetAttr(const String& name) {
	if (!app)
		return PyValue::None();
	if (name == "root")
		return MakeDisplayObject(app, &app->state->GetActiveScene().val, vm);
	if (name == "time")
		return PyValue(app->anim ? app->anim->time : 0.0);
	if (name == "frame")
		return PyValue(app->anim ? app->anim->position : 0);
	if (name == "fps")
		return PyValue(app->prj ? app->prj->fps : 0);
	if (name == "find")
		return PyValue::BoundMethod(PyValue::Function("find", Stage_Find, nullptr), PyValue(this));
	if (name == "children")
		return PyValue::BoundMethod(PyValue::Function("children", Stage_Children, nullptr), PyValue(this));
	if (name == "create")
		return PyValue::BoundMethod(PyValue::Function("create", Stage_Create, nullptr), PyValue(this));
	if (name == "goto")
		return PyValue::BoundMethod(PyValue::Function("goto", Stage_Goto, nullptr), PyValue(this));
	if (name == "gotoAndPlay")
		return PyValue::BoundMethod(PyValue::Function("gotoAndPlay", Stage_GotoAndPlay, nullptr), PyValue(this));
	if (name == "gotoAndStop")
		return PyValue::BoundMethod(PyValue::Function("gotoAndStop", Stage_GotoAndStop, nullptr), PyValue(this));
	if (name == "on")
		return PyValue::BoundMethod(PyValue::Function("on", Stage_On, nullptr), PyValue(this));
	if (name == "onFrame" || name == "onEnterFrame")
		return PyValue::BoundMethod(PyValue::Function("onFrame", Stage_OnFrame, nullptr), PyValue(this));
	if (name == "exit")
		return PyValue::BoundMethod(PyValue::Function("exit", Stage_Exit, nullptr), PyValue(this));
	return PyValue::None();
}

bool StageProxy::SetAttr(const String& name, const PyValue& v) {
	if (!app || !app->anim || !app->prj)
		return false;
	if (name == "frame") {
		int frame = (int)v.AsInt();
		app->anim->position = frame;
		double frame_time = 1.0 / max(1, app->prj->kps);
		app->anim->time = frame * frame_time;
		app->RefrehRenderers();
		return true;
	}
	if (name == "onFrame" || name == "onEnterFrame") {
		app->AddScriptEventHandler("enterFrame", vm, nullptr, v);
		return true;
	}
	return false;
}

struct InputProxy : PyUserData {
	ExecInputState* input = nullptr;
	InputProxy(ExecInputState* st) : input(st) {}
	String GetTypeName() const override { return "Input"; }
	PyValue GetAttr(const String& name) override;
	bool SetAttr(const String& name, const PyValue& v) override { return false; }
};

static PyValue Input_IsKeyDown(const Vector<PyValue>& args, void* user_data) {
	InputProxy* self = (InputProxy*)user_data;
	if (!self || !self->input || args.GetCount() < 1)
		return PyValue::False();
	return PyValue(self->input->IsKeyDown(args[0].AsInt()));
}

static PyValue Input_WasKeyPressed(const Vector<PyValue>& args, void* user_data) {
	InputProxy* self = (InputProxy*)user_data;
	if (!self || !self->input || args.GetCount() < 1)
		return PyValue::False();
	return PyValue(self->input->WasKeyPressed(args[0].AsInt()));
}

static PyValue Input_WasKeyReleased(const Vector<PyValue>& args, void* user_data) {
	InputProxy* self = (InputProxy*)user_data;
	if (!self || !self->input || args.GetCount() < 1)
		return PyValue::False();
	return PyValue(self->input->WasKeyReleased(args[0].AsInt()));
}

static PyValue Input_IsMouseDown(const Vector<PyValue>& args, void* user_data) {
	InputProxy* self = (InputProxy*)user_data;
	if (!self || !self->input)
		return PyValue::False();
	return PyValue(self->input->mouse_down);
}

PyValue InputProxy::GetAttr(const String& name) {
	if (!input)
		return PyValue::None();
	if (name == "mouseX")
		return PyValue(input->mouse_pos.x);
	if (name == "mouseY")
		return PyValue(input->mouse_pos.y);
	if (name == "mouseDX")
		return PyValue(input->mouse_delta.x);
	if (name == "mouseDY")
		return PyValue(input->mouse_delta.y);
	if (name == "mouseDown")
		return PyValue(input->mouse_down);
	if (name == "wheel")
		return PyValue(input->wheel_delta);
	if (name == "isKeyDown")
		return PyValue::Function("isKeyDown", Input_IsKeyDown, this);
	if (name == "wasKeyPressed")
		return PyValue::Function("wasKeyPressed", Input_WasKeyPressed, this);
	if (name == "wasKeyReleased")
		return PyValue::Function("wasKeyReleased", Input_WasKeyReleased, this);
	if (name == "isMouseDown")
		return PyValue::Function("isMouseDown", Input_IsMouseDown, this);
	return PyValue::None();
}

vec3 ApplyInversePoseSimple(const PointcloudPose& pose, const vec3& p) {
	quat inv = pose.orientation.GetInverse();
	return VectorTransform(p - pose.position, inv);
}

bool VisibleInFrustumSimple(const vec3& cam, const SyntheticPointcloudConfig& cfg) {
	float z = cam[2] * SCALAR_FWD_Zf;
	if (z < cfg.hmd_min_dist || z > cfg.max_range)
		return false;
	float half = (cfg.hmd_fov_deg * 0.5f) * (float)M_PI / 180.0f;
	float limit = tan(half) * z;
	if (fabs(cam[0]) > limit || fabs(cam[1]) > limit)
		return false;
	return true;
}

float RandRange(float a, float b) {
	return a + (b - a) * (float)Randomf();
}

PointcloudPose RandomHmdPose(const SyntheticPointcloudConfig& cfg) {
	PointcloudPose pose;
	pose.position[0] = RandRange(cfg.bounds_min[0], cfg.bounds_max[0]);
	pose.position[1] = RandRange(cfg.bounds_min[1], cfg.bounds_max[1]);
	pose.position[2] = RandRange(cfg.bounds_min[2], cfg.bounds_max[2]);
	vec3 to_origin = -pose.position;
	if (to_origin.GetLength() < 1e-3f)
		to_origin = VEC_FWD;
	vec3 axes = GetDirAxes(to_origin);
	float yaw = axes[0] + RandRange(-0.2f, 0.2f);
	float pitch = axes[1] + RandRange(-0.15f, 0.15f);
	float roll = RandRange(-M_PIf * 0.05f, M_PIf * 0.05f);
	pose.orientation = AxesQuat(yaw, pitch, roll);
	return pose;
}

PointcloudPose RandomControllerPoseInFrustum(const PointcloudPose& hmd_pose,
                                             const SyntheticPointcloudConfig& cfg,
                                             float min_dist,
                                             float max_dist) {
	PointcloudPose pose;
	float dist = RandRange(min_dist, max_dist);
	float half = (cfg.hmd_fov_deg * 0.5f) * (float)M_PI / 180.0f;
	float limit = tan(half) * dist;
	float horiz = RandRange(-limit * 0.8f, limit * 0.8f);
	float vert = RandRange(-limit * 0.6f, limit * 0.6f);
	vec3 fwd = VectorTransform(VEC_FWD, hmd_pose.orientation);
	vec3 right = VectorTransform(VEC_RIGHT, hmd_pose.orientation);
	vec3 up = VectorTransform(VEC_UP, hmd_pose.orientation);
	pose.position = hmd_pose.position + fwd * dist + right * horiz + up * vert;
	float yaw = RandRange(-M_PIf, M_PIf);
	float pitch = RandRange(-M_PIf * 0.25f, M_PIf * 0.25f);
	float roll = RandRange(-M_PIf * 0.1f, M_PIf * 0.1f);
	pose.orientation = AxesQuat(yaw, pitch, roll);
	return pose;
}

}

FilePoolCtrl::FilePoolCtrl(Edit3D* e) {
	owner = e;
	Add(files.SizePos());
	files.AddColumn("Path");
	files.AddColumn("Type");
	files.AddColumn("Info");
	files.AddColumn("Usage");
	files.AddColumn("Size");
	files.AddColumn("Modified");
	files.EvenRowColor();
	files.SetLineCy(EditField::GetStdHeight());
}

void FilePoolCtrl::Data() {
	files.Clear();
	if (!owner || !owner->prj)
		return;
	
	VectorMap<String, int> usage;
	for (GeomScene& scene : owner->prj->val.Sub<GeomScene>()) {
		GeomObjectCollection objs(scene);
		for (GeomObject& o : objs) {
			if (!o.asset_ref.IsEmpty())
				usage.GetAdd(o.asset_ref, 0)++;
			if (!o.pointcloud_ref.IsEmpty())
				usage.GetAdd(o.pointcloud_ref, 0)++;
		}
	}
	
	const Array<Scene3DExternalFile>& files_list = owner->scene3d_external_files;
	files.SetCount(files_list.GetCount());
	for (int i = 0; i < files_list.GetCount(); i++) {
		const Scene3DExternalFile& f = files_list[i];
		String info = f.note;
		if (info.IsEmpty())
			info = f.id;
		String size = f.size >= 0 ? FormatInt64(f.size) : String();
		String usage_str;
		int usage_count = usage.Find(f.path) >= 0 ? usage.Get(f.path) : 0;
		if (usage_count > 0)
			usage_str = IntStr(usage_count);
		files.Set(i, 0, f.path);
		files.Set(i, 1, f.type);
		files.Set(i, 2, info);
		files.Set(i, 3, usage_str);
		files.Set(i, 4, size);
		files.Set(i, 5, f.modified_utc);
	}
}

AssetBrowserCtrl::AssetBrowserCtrl(Edit3D* e) {
	owner = e;
	Add(split.SizePos());
	split.Horz(tree, files);
	split.SetPos(2600);
	tree.WhenCursor << THISBACK(OnTreeCursor);
	files.AddIndex("path");
	files.AddColumn(t_("Preview")).FixedWidth(56);
	files.AddColumn(t_("Name"));
	files.AddColumn(t_("Type"), 70);
	files.AddColumn(t_("Size"), 80);
	files.EvenRowColor();
	files.SetLineCy(52);
	files.SetColumnDisplay(0, CenteredImageDisplay());
	files.WhenLeftDouble << THISBACK(OnFileDouble);
	files.WhenDrag << THISBACK(StartDrag);
}

void AssetBrowserCtrl::SetRoot(const String& dir) {
	root_dir = dir;
	BuildTree();
}

void AssetBrowserCtrl::SetRecent(const Vector<String>& list) {
	recent_assets.Clear();
	recent_assets.Append(list);
	UpdateFiles();
}

void AssetBrowserCtrl::Data() {
	BuildTree();
	UpdateFiles();
}

void AssetBrowserCtrl::BuildTree() {
	tree.Clear();
	tree.SetRoot(CtrlImg::Dir(), Value(), t_("Assets"));
	int recent = tree.Add(0, CtrlImg::Dir(), String("recent:"), t_("Recent"));
	if (!root_dir.IsEmpty()) {
		int root = tree.Add(0, CtrlImg::Dir(), root_dir, t_("Project Assets"));
		Vector<String> stack;
		stack.Add(root_dir);
		Vector<int> stack_id;
		stack_id.Add(root);
		while (!stack.IsEmpty()) {
			String dir = stack.Top();
			int parent = stack_id.Top();
			stack.Drop();
			stack_id.Drop();
			FindFile ff(AppendFileName(dir, "*"));
			Vector<String> dirs;
			while (ff) {
				if (ff.IsFolder()) {
					String name = ff.GetName();
					if (name != "." && name != "..")
						dirs.Add(AppendFileName(dir, name));
				}
				ff.Next();
			}
			Sort(dirs, StdLess<String>());
			for (const String& sub : dirs) {
				int id = tree.Add(parent, CtrlImg::Dir(), sub, GetFileName(sub));
				stack.Add(sub);
				stack_id.Add(id);
			}
		}
		tree.Open(root);
	}
	tree.Open(recent);
	if (!tree.IsCursor())
		tree.SetCursor(recent);
}

String AssetBrowserCtrl::GetCursorPath() const {
	if (!tree.IsCursor())
		return String();
	Value v = tree.Get(tree.GetCursor());
	if (v.Is<String>())
		return v.To<String>();
	return String();
}

Image AssetBrowserCtrl::MakePreview(const String& path, int size) const {
	String ext = ToLower(GetFileExt(path));
	if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp" ||
	    ext == ".tga" || ext == ".gif") {
		Image img = StreamRaster::LoadFileAny(path);
		if (!img.IsEmpty()) {
			Size sz = img.GetSize();
			if (sz.cx > size || sz.cy > size)
				return Rescale(img, size, size);
			return img;
		}
	}
	return CtrlImg::File();
}

void AssetBrowserCtrl::UpdateFiles() {
	files.Clear();
	String sel = GetCursorPath();
	Vector<String> entries;
	if (sel.StartsWith("recent:")) {
		for (const String& rel : recent_assets) {
			String abs = rel;
			if (owner && !owner->GetProjectDir().IsEmpty() && !IsFullPath(rel))
				abs = AppendFileName(owner->GetProjectDir(), rel);
			if (FileExists(abs))
				entries.Add(abs);
		}
	}
	else if (!sel.IsEmpty() && DirectoryExists(sel)) {
		FindFile ff(AppendFileName(sel, "*"));
		while (ff) {
			if (!ff.IsFolder()) {
				String abs = AppendFileName(sel, ff.GetName());
				entries.Add(abs);
			}
			ff.Next();
		}
	}
	Sort(entries, StdLess<String>());
	files.SetCount(entries.GetCount());
	for (int i = 0; i < entries.GetCount(); i++) {
		String abs = entries[i];
		String name = GetFileName(abs);
		String ext = ToLower(GetFileExt(abs));
		int64 size = (int64)GetFileLength(abs);
		String size_str = size >= 0 ? FormatInt64(size) : String();
		Image preview = MakePreview(abs, 48);
		files.Set(i, "path", abs);
		files.Set(i, 0, preview);
		files.Set(i, 1, name);
		files.Set(i, 2, ext);
		files.Set(i, 3, size_str);
	}
}

void AssetBrowserCtrl::OnTreeCursor() {
	UpdateFiles();
}

void AssetBrowserCtrl::OnFileDouble() {
	if (!owner)
		return;
	String file = files.IsCursor() ? String(files.Get("path")) : String();
	if (!file.IsEmpty())
		owner->AddAssetFromPath(file);
}

void AssetBrowserCtrl::StartDrag() {
	if (!files.IsCursor())
		return;
	String path = files.Get("path");
	if (path.IsEmpty())
		return;
	VectorMap<String, ClipData> data;
	Append(data, path);
	Image sample = MakePreview(path, 48);
	DoDragAndDrop(data, sample);
}

ScriptEditorCtrl::ScriptEditorCtrl(Edit3D* e) : owner(e) {
	AddFrame(tool);
	Add(split.SizePos());
	editor.Highlight("python");
	editor.EnableBreakpointing();
	editor.WhenAction << THISBACK(OnChange);
	editor.WhenBreakpoint = [=](int line) { ToggleBreakpoint(line); };
	errors.AddColumn(t_("Line"), 50);
	errors.AddColumn(t_("Message"), 400);
	errors.WhenLeftDouble = [=] {
		int row = errors.GetCursor();
		if (row < 0)
			return;
		int line = errors.Get(row, 0);
		if (line > 0) {
			editor.GotoBarLine(line - 1);
			editor.SetFocus();
		}
	};
	split.Vert(editor, errors);
	split.SetPos(8000);
	tool.Set([=](Bar& bar) {
		bar.Add(t_("Save"), THISBACK(Save));
		bar.Add(t_("Reload"), [=] { if (!path.IsEmpty()) OpenFile(path); });
		bar.Separator();
		bar.Add(t_("Run File"), THISBACK(RunFile));
		bar.Add(t_("Run Selection"), THISBACK(RunSelection));
		bar.Add(t_("Stop"), THISBACK(StopScript));
		bar.Separator();
		bar.Add(t_("Clear Errors"), THISBACK(ClearErrors));
	});
}

TextureEditCtrl::TextureEditCtrl() {
	WantFocus();
}

void TextureEditCtrl::SetImage(const Image& image) {
	Image im = image;
	img = im;
	has_img = !img.IsEmpty();
	UpdateCached();
	Refresh();
}

void TextureEditCtrl::Clear() {
	img.Clear();
	cached = Image();
	has_img = false;
	Refresh();
}

Image TextureEditCtrl::GetImage() const {
	return cached;
}

void TextureEditCtrl::UpdateCached() {
	if (img.IsEmpty()) {
		cached = Image();
		return;
	}
	cached = Image(img);
}

void TextureEditCtrl::Paint(Draw& d) {
	Size sz = GetSize();
	d.DrawRect(sz, SColorPaper());
	if (!has_img || img.IsEmpty())
		return;
	Size isz = img.GetSize();
	if (isz.cx <= 0 || isz.cy <= 0)
		return;
	double sx = (double)sz.cx / (double)isz.cx;
	double sy = (double)sz.cy / (double)isz.cy;
	double s = min(sx, sy);
	int w = (int)floor(isz.cx * s);
	int h = (int)floor(isz.cy * s);
	int x = (sz.cx - w) / 2;
	int y = (sz.cy - h) / 2;
	img_rect = RectC(x, y, w, h);
	d.DrawRect(img_rect, Color(30, 30, 30));
	d.DrawImage(x, y, w, h, cached);
}

void TextureEditCtrl::PaintAt(Point p) {
	if (!has_img || img.IsEmpty())
		return;
	if (!img_rect.Contains(p))
		return;
	Size isz = img.GetSize();
	int ix = (int)((double)(p.x - img_rect.left) / (double)img_rect.Width() * isz.cx);
	int iy = (int)((double)(p.y - img_rect.top) / (double)img_rect.Height() * isz.cy);
	ix = Clamp(ix, 0, isz.cx - 1);
	iy = Clamp(iy, 0, isz.cy - 1);
	int r = max(1, brush);
	int r2 = r * r;
	for (int y = -r; y <= r; y++) {
		for (int x = -r; x <= r; x++) {
			if (x * x + y * y > r2)
				continue;
			int px = ix + x;
			int py = iy + y;
			if (px < 0 || py < 0 || px >= isz.cx || py >= isz.cy)
				continue;
			img[py][px] = color;
		}
	}
	UpdateCached();
	Refresh();
	if (WhenPaint)
		WhenPaint();
}

void TextureEditCtrl::LeftDown(Point p, dword keyflags) {
	painting = true;
	SetFocus();
	PaintAt(p);
}

void TextureEditCtrl::LeftDrag(Point p, dword keyflags) {
	if (!painting)
		return;
	PaintAt(p);
}

void TextureEditCtrl::LeftUp(Point p, dword keyflags) {
	painting = false;
}

void TextureEditCtrl::MouseWheel(Point p, int zdelta, dword keyflags) {
	if (keyflags & K_CTRL) {
		if (zdelta > 0)
			brush = min(128, brush + 2);
		else
			brush = max(1, brush - 2);
	}
}

void ScriptEditorCtrl::OpenFile(const String& p) {
	path = p;
	String data;
	if (FileExists(path))
		data = LoadFile(path);
	editor.Set(data);
	editor.SetFocus();
	dirty = false;
	ApplyBreakpoints();
}

void ScriptEditorCtrl::Save() {
	if (path.IsEmpty())
		return;
	SaveFile(path, editor.Get());
	dirty = false;
}

void ScriptEditorCtrl::SaveAs(const String& p) {
	path = p;
	Save();
}

void ScriptEditorCtrl::OnChange() {
	dirty = true;
}

void ScriptEditorCtrl::OpenScript(GeomScript& s) {
	script = &s;
	if (owner)
		owner->EnsureScriptFile(s, "script");
	if (owner)
		OpenFile(owner->GetScriptAbsPath(s.file));
}

void ScriptEditorCtrl::ClearErrors() {
	errors.Clear();
}

void ScriptEditorCtrl::AddError(int line, const String& msg) {
	errors.Add(line > 0 ? line : Null, msg);
	errors.SetCursor(errors.GetCount() - 1);
}

void ScriptEditorCtrl::ApplyBreakpoints() {
	editor.ClearBreakpoints();
	int idx = breakpoints.Find(path);
	if (idx < 0)
		return;
	const Vector<int>& lines = breakpoints[idx];
	for (int ln : lines) {
		if (ln >= 0)
			editor.SetBreakpoint(ln, "1");
	}
}

void ScriptEditorCtrl::ToggleBreakpoint(int line) {
	if (line < 0)
		return;
	String cur = editor.GetBreakpoint(line);
	bool on = cur.IsEmpty();
	editor.SetBreakpoint(line, on ? "1" : "");
	Vector<int>& lines = breakpoints.GetAdd(path);
	int idx = FindIndex(lines, line);
	if (on) {
		if (idx < 0)
			lines.Add(line);
	}
	else {
		if (idx >= 0)
			lines.Remove(idx);
	}
}

static int ExtractErrorLine(const String& msg) {
	int pos = msg.Find("line");
	if (pos < 0)
		pos = msg.Find("Line");
	if (pos >= 0) {
		pos += 4;
		while (pos < msg.GetCount() && !IsDigit(msg[pos]))
			pos++;
		if (pos < msg.GetCount()) {
			int end = pos;
			while (end < msg.GetCount() && IsDigit(msg[end]))
				end++;
			return ScanInt(msg.Mid(pos, end - pos));
		}
	}
	return 0;
}

void ScriptEditorCtrl::RunFile() {
	if (!owner)
		return;
	if (dirty)
		Save();
	ClearErrors();
	PyVM vm;
	owner->RegisterScriptVM(vm);
	if (owner->state) {
		PyValue root_obj = MakeDisplayObject(owner, &owner->state->GetActiveScene().val, &vm);
		vm.GetGlobalsRW().GetDictRW().GetAdd(PyValue("root")) = root_obj;
		PyValue this_obj = root_obj;
		if (script && script->val.owner) {
			PyValue owner_obj = MakeDisplayObject(owner, script->val.owner, &vm);
			if (!owner_obj.IsNone())
				this_obj = owner_obj;
		}
		vm.GetGlobalsRW().GetDictRW().GetAdd(PyValue("this")) = this_obj;
	}
	vm.GetGlobalsRW().GetDictRW().GetAdd(PyValue("__project_dir__")) = PyValue(owner->project_dir);
	vm.GetGlobalsRW().GetDictRW().GetAdd(PyValue("__script_path__")) = PyValue(path);
	String code = editor.Get();
	String err;
	Vector<PyIR> ir;
	if (!CompilePySource(code, path, ir, err)) {
		int line = ExtractErrorLine(err);
		AddError(line, err);
		return;
	}
	if (!RunPyIR(vm, ir, err)) {
		int line = ExtractErrorLine(err);
		AddError(line, err);
	}
	owner->RemoveScriptEventHandlers(&vm);
}

void ScriptEditorCtrl::RunSelection() {
	if (!owner)
		return;
	int64 l = 0, h = 0;
	if (!editor.GetSelection(l, h)) {
		RunFile();
		return;
	}
	String code = editor.Get(l, h);
	if (code.IsEmpty()) {
		RunFile();
		return;
	}
	ClearErrors();
	int base_line = editor.GetLine(l) + 1;
	PyVM vm;
	owner->RegisterScriptVM(vm);
	if (owner->state) {
		PyValue root_obj = MakeDisplayObject(owner, &owner->state->GetActiveScene().val, &vm);
		vm.GetGlobalsRW().GetDictRW().GetAdd(PyValue("root")) = root_obj;
		PyValue this_obj = root_obj;
		if (script && script->val.owner) {
			PyValue owner_obj = MakeDisplayObject(owner, script->val.owner, &vm);
			if (!owner_obj.IsNone())
				this_obj = owner_obj;
		}
		vm.GetGlobalsRW().GetDictRW().GetAdd(PyValue("this")) = this_obj;
	}
	vm.GetGlobalsRW().GetDictRW().GetAdd(PyValue("__project_dir__")) = PyValue(owner->project_dir);
	vm.GetGlobalsRW().GetDictRW().GetAdd(PyValue("__script_path__")) = PyValue(path);
	String err;
	Vector<PyIR> ir;
	if (!CompilePySource(code, path, ir, err)) {
		int line = ExtractErrorLine(err);
		if (line > 0)
			line += base_line - 1;
		AddError(line, err);
		return;
	}
	if (!RunPyIR(vm, ir, err)) {
		int line = ExtractErrorLine(err);
		if (line > 0)
			line += base_line - 1;
		AddError(line, err);
	}
	owner->RemoveScriptEventHandlers(&vm);
}

void ScriptEditorCtrl::StopScript() {
	if (!owner || !script)
		return;
	for (int i = 0; i < owner->script_instances.GetCount(); i++) {
		Edit3D::ScriptInstance& inst = owner->script_instances[i];
		if (inst.script == script) {
			owner->RemoveScriptEventHandlers(&inst.vm);
			script->run_every_frame = false;
			break;
		}
	}
}


#if defined(flagPOSIX) && defined(flagSOFTHMD)
bool HmdCapture::Start() {
	if (running)
		return true;
	if (!sys.Initialise())
		return false;
	source = CreateStereoSource("hmd");
	if (source.IsEmpty() || !source->Start()) {
		source.Clear();
		sys.Uninitialise();
		return false;
	}
	ResetTracking();
	running = true;
	return true;
}

void HmdCapture::Stop() {
	if (!running)
		return;
	recording = false;
	if (source)
		source->Stop();
	source.Clear();
	sys.Uninitialise();
	running = false;
	bright = Image();
	dark = Image();
	bright_serial = -1;
	dark_serial = -1;
}

void HmdCapture::ResetTracking() {
	fusion.Reset();
	fusion.GetBrightTracker().SetWmrDefaults(sys.vendor_id, sys.product_id);
	fusion.GetDarkTracker().SetWmrDefaults(sys.vendor_id, sys.product_id);
}

const Octree* HmdCapture::GetPointcloud(bool use_bright) const {
	if (use_bright)
		return const_cast<HMD::SoftHmdFusion&>(fusion).GetBrightTracker().GetPointcloud();
	return const_cast<HMD::SoftHmdFusion&>(fusion).GetDarkTracker().GetPointcloud();
}

void HmdCapture::Poll() {
	if (!running)
		return;
	sys.UpdateData();
	if(sys.hmd) {
		ImuSample imu;
		imu.timestamp_us = usecs();
		bool has_imu = false;
		if(HMD::GetDeviceFloat(sys.hmd, HMD::HMD_ACCELEROMETER_VECTOR, imu.accel.data) == HMD::HMD_S_OK)
			has_imu = true;
		if(HMD::GetDeviceFloat(sys.hmd, HMD::HMD_GYROSCOPE_VECTOR, imu.gyro.data) == HMD::HMD_S_OK)
			has_imu = true;
		if(has_imu)
			fusion.PutImu(imu);
	}
	if (!source || !source->IsRunning())
		return;
	CameraFrame lf, rf;
	if (!source->ReadFrame(lf, rf, false))
		return;
	Image combined;
	if (!JoinStereoImage(lf.img, rf.img, combined))
		return;
	if (lf.is_bright) {
		bright = combined;
		bright_serial = lf.serial;
	} else {
		dark = combined;
		dark_serial = lf.serial;
	}
	if (!recording)
		return;
	VisualFrame vf;
	vf.timestamp_us = usecs();
	vf.format = GEOM_EVENT_CAM_RGBA8;
	vf.width = combined.GetWidth();
	vf.height = combined.GetHeight();
	vf.stride = vf.width * (int)sizeof(RGBA);
	vf.img = combined;
	vf.data = 0;
	vf.data_bytes = combined.GetLength() * (int)sizeof(RGBA);
	vf.flags = lf.is_bright ? VIS_FRAME_BRIGHT : VIS_FRAME_DARK;
	fusion.PutVisual(vf);
}
#endif



Edit3D::Edit3D() :
	v0(this),
	v1(this),
	file_pool(this),
	asset_browser(this),
	script_editor(this)
{
	prj = &prj_val.CreateExt<GeomProject>();
	state = &state_val.CreateExt<GeomWorldState>();
	anim = &anim_val.CreateExt<GeomAnim>();
	state->prj = prj;
	anim->state = state;
	video.anim = anim;
	render_ctx.conf = &conf;
	render_ctx.state = state;
	render_ctx.anim = anim;
	render_ctx.video = &video;
	script_timer.Reset();
	
	anim->WhenSceneEnd << THISBACK(OnSceneEnd);
	
	Sizeable().MaximizeBox();
	Title("Edit3D");
	LayoutId("mainWindow");
	scene3d_data_dir = "data";
	SetProjectDir(GetCurrentDirectory());
	view = VIEW_GEOMPROJECT;
	Add(v0.grid.SizePos());
	
	AddFrame(ribbon);
	ribbon.LayoutId("ribbonBar");
	tool_panel.Init(this);
	ribbon.Init(this);
	ribbon.SetAppButton(t_("File"), CtrlImg::File())
	      .SetAppMenu([this](Bar& bar) {
		bar.Sub(t_("File"), [this](Bar& bar) {
			bar.Add(t_("New"), THISBACK(LoadEmptyProject)).Key(K_CTRL|K_N);
			bar.Add(t_("Open..."), THISBACK(OpenScene3D)).Key(K_CTRL|K_O);
			bar.Add(t_("Save"), THISBACK(SaveScene3DInteractive)).Key(K_CTRL|K_S);
			bar.Add(t_("Save As..."), THISBACK(SaveScene3DAs)).Key(K_CTRL|K_SHIFT|K_S);
			bar.Add(t_("Save as JSON..."), THISBACK(SaveScene3DAsJson));
			bar.Add(t_("Save as Binary..."), THISBACK(SaveScene3DAsBinary));
			bar.Separator();
			bar.Sub(t_("Export"), [this](Bar& bar) {
				bar.Add(t_("Export (Full Assets)"), [this] { ExportExecutionProject(true); });
				bar.Add(t_("Export (Lightweight)"), [this] { ExportExecutionProject(false); });
			});
			bar.Separator();
			bar.Add(t_("Execute"), THISBACK(OpenExecutionWindow)).Key(K_F9);
			bar.Separator();
			bar.Sub(t_("Format"), [this](Bar& bar) {
				bar.Add(t_("JSON (default)"), THISBACK1(SetScene3DFormat, true))
					.Check(scene3d_use_json);
				bar.Add(t_("Binary (default)"), THISBACK1(SetScene3DFormat, false))
					.Check(!scene3d_use_json);
			});
			bar.Separator();
			bar.Add(t_("Exit"), THISBACK(Exit));
		});
		bar.Sub(t_("View"), [this](Bar& bar) {
			bar.Add(t_("Geometry"), THISBACK1(SetView, VIEW_GEOMPROJECT)).Key(K_ALT|K_1);
			bar.Add(t_("Video import"), THISBACK1(SetView, VIEW_VIDEOIMPORT)).Key(K_ALT|K_2);
			bar.Separator();
			bar.Add(t_("File Pool"), THISBACK(OpenFilePool));
			bar.Add(t_("Asset Browser"), THISBACK(OpenAssetBrowser));
			bar.Add(t_("Texture Editor"), THISBACK(OpenTextureEditor));
			bar.Add(t_("Script Editor"), [this] { if (dock_script) ActivateDockableChild(*dock_script); });
			bar.Add(t_("Tools Panel"), [this] { if (dock_tools) ActivateDockableChild(*dock_tools); });
			bar.Separator();
			bar.Sub(t_("Tree"), [this](Bar& bar) { v0.TreeMenu(bar); });
			bar.Separator();
			bar.Sub(t_("HUD"), [this](Bar& bar) {
				bar.Add(t_("Show HUD"), [this] { show_hud = !show_hud; }).Check(show_hud);
				bar.Add(t_("Show Help"), [this] { show_hud_help = !show_hud_help; }).Check(show_hud_help);
				bar.Add(t_("Show Status"), [this] { show_hud_status = !show_hud_status; }).Check(show_hud_status);
				bar.Add(t_("Show Debug"), [this] { show_hud_debug = !show_hud_debug; }).Check(show_hud_debug);
			});
			bar.Separator();
			bar.Add(t_("Reset Layout"), THISBACK(ResetLayout));
			bar.Add(t_("Reset Props Cursor"), THISBACK(ResetPropsCursor));
		});
		bar.Sub(t_("Edit"), [this](Bar& bar) {
			bar.Add(t_("Undo"), THISBACK(Undo)).Key(K_CTRL|K_Z);
			bar.Add(t_("Redo"), THISBACK(Redo)).Key(K_CTRL|K_Y);
			bar.Separator();
			bar.Add(t_("View (Camera)"), THISBACK1(SetEditTool, TOOL_VIEW)).Key(K_SPACE)
				.Check(edit_tool == TOOL_VIEW);
			bar.Separator();
			bar.Add(t_("Create Editable Mesh"), THISBACK(CreateEditableMeshObject));
			bar.Add(t_("Create 2D Layer"), THISBACK(Create2DLayerObject));
			bar.Separator();
			bar.Sub(t_("Object Tools"), [this](Bar& bar) {
				bar.Add(t_("Select"), THISBACK1(SetEditTool, TOOL_OBJ_SELECT)).Key(K_Q)
					.Check(edit_tool == TOOL_OBJ_SELECT);
				bar.Add(t_("Move"), THISBACK1(SetEditTool, TOOL_OBJ_MOVE)).Key(K_W)
					.Check(edit_tool == TOOL_OBJ_MOVE);
				bar.Add(t_("Rotate"), THISBACK1(SetEditTool, TOOL_OBJ_ROTATE)).Key(K_E)
					.Check(edit_tool == TOOL_OBJ_ROTATE);
			});
			bar.Sub(t_("Mesh Tools"), [this](Bar& bar) {
				bar.Add(t_("Mesh Select"), THISBACK1(SetEditTool, TOOL_MESH_SELECT))
					.Check(edit_tool == TOOL_MESH_SELECT).Key(K_A);
				bar.Add(t_("Point Tool"), THISBACK1(SetEditTool, TOOL_POINT))
					.Check(edit_tool == TOOL_POINT).Key(K_S);
				bar.Add(t_("Line Tool"), THISBACK1(SetEditTool, TOOL_LINE))
					.Check(edit_tool == TOOL_LINE).Key(K_D);
				bar.Add(t_("Face Tool"), THISBACK1(SetEditTool, TOOL_FACE))
					.Check(edit_tool == TOOL_FACE).Key(K_F);
				bar.Add(t_("Erase Tool"), THISBACK1(SetEditTool, TOOL_ERASE))
					.Check(edit_tool == TOOL_ERASE).Key(K_X);
				bar.Add(t_("Join Tool"), THISBACK1(SetEditTool, TOOL_JOIN))
					.Check(edit_tool == TOOL_JOIN).Key(K_C);
				bar.Add(t_("Split Tool"), THISBACK1(SetEditTool, TOOL_SPLIT))
					.Check(edit_tool == TOOL_SPLIT).Key(K_V);
			});
			bar.Separator();
			bar.Sub(t_("2D Tools"), [this](Bar& bar) {
				bar.Add(t_("2D Select"), THISBACK1(SetEditTool, TOOL_2D_SELECT))
					.Check(edit_tool == TOOL_2D_SELECT);
				bar.Add(t_("2D Line"), THISBACK1(SetEditTool, TOOL_2D_LINE))
					.Check(edit_tool == TOOL_2D_LINE);
				bar.Add(t_("2D Rectangle"), THISBACK1(SetEditTool, TOOL_2D_RECT))
					.Check(edit_tool == TOOL_2D_RECT);
				bar.Add(t_("2D Circle"), THISBACK1(SetEditTool, TOOL_2D_CIRCLE))
					.Check(edit_tool == TOOL_2D_CIRCLE);
				bar.Add(t_("2D Polygon"), THISBACK1(SetEditTool, TOOL_2D_POLY))
					.Check(edit_tool == TOOL_2D_POLY);
				bar.Add(t_("2D Erase"), THISBACK1(SetEditTool, TOOL_2D_ERASE))
					.Check(edit_tool == TOOL_2D_ERASE);
				bar.Separator();
				bar.Sub(t_("2D Ops"), [this](Bar& bar) {
					bar.Add(t_("Union"), THISBACK(Union2DSelection));
					bar.Add(t_("Intersect"), THISBACK(Intersect2DSelection));
					bar.Add(t_("Subtract"), THISBACK(Subtract2DSelection));
					bar.Separator();
					bar.Add(t_("Align Left"), [this] { Align2DSelection(0); });
					bar.Add(t_("Align H Center"), [this] { Align2DSelection(1); });
					bar.Add(t_("Align Right"), [this] { Align2DSelection(2); });
					bar.Add(t_("Align Top"), [this] { Align2DSelection(3); });
					bar.Add(t_("Align V Center"), [this] { Align2DSelection(4); });
					bar.Add(t_("Align Bottom"), [this] { Align2DSelection(5); });
					bar.Separator();
					bar.Add(t_("Distribute Horiz"), [this] { Distribute2DSelection(true); });
					bar.Add(t_("Distribute Vert"), [this] { Distribute2DSelection(false); });
				});
			});
			bar.Sub(t_("Mesh Selection"), [this](Bar& bar) {
				bar.Add(t_("Vertex"), [this] { mesh_select_mode = MESHSEL_VERTEX; tool_panel.Sync(); })
					.Check(mesh_select_mode == MESHSEL_VERTEX);
				bar.Add(t_("Edge"), [this] { mesh_select_mode = MESHSEL_EDGE; tool_panel.Sync(); })
					.Check(mesh_select_mode == MESHSEL_EDGE);
				bar.Add(t_("Face"), [this] { mesh_select_mode = MESHSEL_FACE; tool_panel.Sync(); })
					.Check(mesh_select_mode == MESHSEL_FACE);
				bar.Separator();
				bar.Add(t_("Loop Select"), THISBACK(SelectMeshLoop));
				bar.Add(t_("Ring Select"), THISBACK(SelectMeshRing));
				bar.Add(t_("Expand Selection"), THISBACK(ExpandMeshSelection));
				bar.Add(t_("Contract Selection"), THISBACK(ContractMeshSelection));
				bar.Separator();
				bar.Add(t_("Extrude"), [this] { ExtrudeMeshSelection(0.1); });
				bar.Add(t_("Inset"), [this] { InsetMeshSelection(0.1); });
				bar.Add(t_("Spin"), THISBACK(SpinMeshSelection));
				bar.Add(t_("Screw"), THISBACK(ScrewMeshSelection));
			});
			bar.Separator();
			bar.Sub(t_("Plane"), [this](Bar& bar) {
				bar.Add(t_("View Plane"), [this] { edit_plane = PLANE_VIEW; })
					.Check(edit_plane == PLANE_VIEW);
				bar.Add(t_("XY"), [this] { edit_plane = PLANE_XY; })
					.Check(edit_plane == PLANE_XY);
				bar.Add(t_("XZ"), [this] { edit_plane = PLANE_XZ; })
					.Check(edit_plane == PLANE_XZ);
				bar.Add(t_("YZ"), [this] { edit_plane = PLANE_YZ; })
					.Check(edit_plane == PLANE_YZ);
				bar.Add(t_("Local"), [this] { edit_plane = PLANE_LOCAL; })
					.Check(edit_plane == PLANE_LOCAL);
			});
			bar.Sub(t_("Snap"), [this](Bar& bar) {
				bar.Add(t_("Enable"), [this] { edit_snap_enable = !edit_snap_enable; })
					.Check(edit_snap_enable);
				bar.Add(t_("Local Axes"), [this] { edit_snap_local = !edit_snap_local; })
					.Check(edit_snap_local);
				bar.Separator();
				bar.Add(t_("Step 0.1"), [this] { edit_snap_step = 0.1; })
					.Check(fabs(edit_snap_step - 0.1) < 1e-6);
				bar.Add(t_("Step 0.5"), [this] { edit_snap_step = 0.5; })
					.Check(fabs(edit_snap_step - 0.5) < 1e-6);
				bar.Add(t_("Step 1.0"), [this] { edit_snap_step = 1.0; })
					.Check(fabs(edit_snap_step - 1.0) < 1e-6);
			});
			bar.Sub(t_("Transform"), [this](Bar& bar) {
				bar.Add(t_("Use Local Axes"), [this] { transform_use_local = !transform_use_local; })
					.Check(transform_use_local);
				bar.Separator();
				bar.Add(t_("Position Snap"), [this] { transform_snap_enable = !transform_snap_enable; })
					.Check(transform_snap_enable);
				bar.Sub(t_("Position Snap Step"), [this](Bar& bar) {
					bar.Add(t_("0.01"), [this] { transform_snap_step = 0.01; })
						.Check(fabs(transform_snap_step - 0.01) < 1e-6);
					bar.Add(t_("0.1"), [this] { transform_snap_step = 0.1; })
						.Check(fabs(transform_snap_step - 0.1) < 1e-6);
					bar.Add(t_("0.5"), [this] { transform_snap_step = 0.5; })
						.Check(fabs(transform_snap_step - 0.5) < 1e-6);
					bar.Add(t_("1.0"), [this] { transform_snap_step = 1.0; })
						.Check(fabs(transform_snap_step - 1.0) < 1e-6);
				});
				bar.Separator();
				bar.Add(t_("Angle Snap"), [this] { transform_angle_snap = !transform_angle_snap; })
					.Check(transform_angle_snap);
				bar.Sub(t_("Angle Snap Step"), [this](Bar& bar) {
					bar.Add(t_("1 deg"), [this] { transform_angle_step = M_PIf / 180.0; })
						.Check(fabs(transform_angle_step - (M_PIf / 180.0)) < 1e-6);
					bar.Add(t_("5 deg"), [this] { transform_angle_step = M_PIf / 36.0; })
						.Check(fabs(transform_angle_step - (M_PIf / 36.0)) < 1e-6);
					bar.Add(t_("15 deg"), [this] { transform_angle_step = M_PIf / 12.0; })
						.Check(fabs(transform_angle_step - (M_PIf / 12.0)) < 1e-6);
					bar.Add(t_("45 deg"), [this] { transform_angle_step = M_PIf / 4.0; })
						.Check(fabs(transform_angle_step - (M_PIf / 4.0)) < 1e-6);
				});
				bar.Separator();
				bar.Sub(t_("Nudge Step"), [this](Bar& bar) {
					bar.Add(t_("Small 0.01"), [this] { transform_nudge_small = 0.01; })
						.Check(fabs(transform_nudge_small - 0.01) < 1e-6);
					bar.Add(t_("Small 0.1"), [this] { transform_nudge_small = 0.1; })
						.Check(fabs(transform_nudge_small - 0.1) < 1e-6);
					bar.Add(t_("Small 0.5"), [this] { transform_nudge_small = 0.5; })
						.Check(fabs(transform_nudge_small - 0.5) < 1e-6);
					bar.Separator();
					bar.Add(t_("Large 0.1"), [this] { transform_nudge_large = 0.1; })
						.Check(fabs(transform_nudge_large - 0.1) < 1e-6);
					bar.Add(t_("Large 1.0"), [this] { transform_nudge_large = 1.0; })
						.Check(fabs(transform_nudge_large - 1.0) < 1e-6);
					bar.Add(t_("Large 5.0"), [this] { transform_nudge_large = 5.0; })
						.Check(fabs(transform_nudge_large - 5.0) < 1e-6);
				});
				bar.Sub(t_("Rotate Step"), [this](Bar& bar) {
					bar.Add(t_("Small 1 deg"), [this] { transform_rot_small = M_PIf / 180.0; })
						.Check(fabs(transform_rot_small - (M_PIf / 180.0)) < 1e-6);
					bar.Add(t_("Small 5 deg"), [this] { transform_rot_small = M_PIf / 36.0; })
						.Check(fabs(transform_rot_small - (M_PIf / 36.0)) < 1e-6);
					bar.Add(t_("Small 15 deg"), [this] { transform_rot_small = M_PIf / 12.0; })
						.Check(fabs(transform_rot_small - (M_PIf / 12.0)) < 1e-6);
					bar.Separator();
					bar.Add(t_("Large 15 deg"), [this] { transform_rot_large = M_PIf / 12.0; })
						.Check(fabs(transform_rot_large - (M_PIf / 12.0)) < 1e-6);
					bar.Add(t_("Large 45 deg"), [this] { transform_rot_large = M_PIf / 4.0; })
						.Check(fabs(transform_rot_large - (M_PIf / 4.0)) < 1e-6);
					bar.Add(t_("Large 90 deg"), [this] { transform_rot_large = M_PIf / 2.0; })
						.Check(fabs(transform_rot_large - (M_PIf / 2.0)) < 1e-6);
				});
			});
			bar.Sub(t_("Sculpt"), [this](Bar& bar) {
				bar.Add(t_("Enable"), [this] { sculpt_mode = !sculpt_mode; })
					.Check(sculpt_mode);
				bar.Separator();
				bar.Add(t_("Add"), [this] { sculpt_add = true; })
					.Check(sculpt_add);
				bar.Add(t_("Subtract"), [this] { sculpt_add = false; })
					.Check(!sculpt_add);
				bar.Separator();
				bar.Add(t_("Radius 0.25"), [this] { sculpt_radius = 0.25; })
					.Check(fabs(sculpt_radius - 0.25) < 1e-6);
				bar.Add(t_("Radius 0.5"), [this] { sculpt_radius = 0.5; })
					.Check(fabs(sculpt_radius - 0.5) < 1e-6);
				bar.Add(t_("Radius 1.0"), [this] { sculpt_radius = 1.0; })
					.Check(fabs(sculpt_radius - 1.0) < 1e-6);
				bar.Separator();
				bar.Add(t_("Strength 0.05"), [this] { sculpt_strength = 0.05; })
					.Check(fabs(sculpt_strength - 0.05) < 1e-6);
				bar.Add(t_("Strength 0.2"), [this] { sculpt_strength = 0.2; })
					.Check(fabs(sculpt_strength - 0.2) < 1e-6);
				bar.Add(t_("Strength 0.5"), [this] { sculpt_strength = 0.5; })
					.Check(fabs(sculpt_strength - 0.5) < 1e-6);
			});
			bar.Sub(t_("Mesh Animation"), [this](Bar& bar) {
				bar.Add(t_("Add Keyframe"), THISBACK(AddMeshKeyframeAtCursor));
				bar.Add(t_("Clear Keyframes"), THISBACK(ClearMeshKeyframes));
			});
			bar.Sub(t_("2D Animation"), [this](Bar& bar) {
				bar.Add(t_("Add Keyframe"), THISBACK(Add2DKeyframeAtCursor));
				bar.Add(t_("Clear Keyframes"), THISBACK(Clear2DKeyframes));
			});
			bar.Sub(t_("Skeleton"), [this](Bar& bar) {
				bar.Add(t_("Create Skeleton"), THISBACK(CreateSkeletonForSelected));
				bar.Add(t_("Add Bone"), THISBACK(AddBoneToSelectedSkeleton));
				bar.Add(t_("Remove Bone"), THISBACK(RemoveSelectedBone));
			});
			bar.Sub(t_("Weights"), [this](Bar& bar) {
				bar.Add(t_("Enable Paint"), [this] { SetWeightPaintMode(!weight_paint_mode); })
					.Check(weight_paint_mode);
				bar.Separator();
				bar.Add(t_("Add"), [this] { weight_add = true; })
					.Check(weight_add);
				bar.Add(t_("Subtract"), [this] { weight_add = false; })
					.Check(!weight_add);
				bar.Separator();
				bar.Add(t_("Radius 0.25"), [this] { weight_radius = 0.25; })
					.Check(fabs(weight_radius - 0.25) < 1e-6);
				bar.Add(t_("Radius 0.5"), [this] { weight_radius = 0.5; })
					.Check(fabs(weight_radius - 0.5) < 1e-6);
				bar.Add(t_("Radius 1.0"), [this] { weight_radius = 1.0; })
					.Check(fabs(weight_radius - 1.0) < 1e-6);
				bar.Separator();
				bar.Add(t_("Strength 0.05"), [this] { weight_strength = 0.05; })
					.Check(fabs(weight_strength - 0.05) < 1e-6);
				bar.Add(t_("Strength 0.2"), [this] { weight_strength = 0.2; })
					.Check(fabs(weight_strength - 0.2) < 1e-6);
				bar.Add(t_("Strength 0.5"), [this] { weight_strength = 0.5; })
					.Check(fabs(weight_strength - 0.5) < 1e-6);
			});
		});
		bar.Sub(t_("Windows"), [this](Bar& bar) { DockWindowMenu(bar); });
		bar.Sub(t_("Pointcloud"), [this](Bar& bar) {
			bar.Add(t_("Record"), THISBACK(TogglePointcloudRecording))
				.Check(record_pointcloud);
			bar.Add(t_("Run Synthetic Sim"), THISBACK(RunSyntheticPointcloudSimDialog));
		});
		bar.Sub(t_("Debug"), [this](Bar& bar) {
			bar.Sub(t_("Pointcloud"), [this](Bar& bar) {
				bar.Add(t_("Generate Source Pointcloud"), THISBACK(DebugGeneratePointcloud));
				bar.Add(t_("Simulate HMD Observation"), THISBACK(DebugSimulateObservation));
				bar.Add(t_("Run Localization"), THISBACK(DebugRunLocalization));
				bar.Add(t_("Simulate Controller Observations"), THISBACK(DebugSimulateControllerObservations));
				bar.Add(t_("Run Controller Localization"), THISBACK(DebugRunControllerLocalization));
				bar.Add(t_("Run Full Synthetic Sim"), THISBACK(RunSyntheticPointcloudSimDialog));
				bar.Separator();
				bar.Add(t_("Clear Synthetic Data"), THISBACK(DebugClearSynthetic));
			});
		});
		
	});
	
	LoadEmptyProject();
	UpdateWindowTitle();
	
	tc.Set(-1000/60, THISBACK(Update));
	
}

static String Scene3DExportTimeStamp() {
	Time now = GetUtcTime();
	return Format("%04d%02d%02d_%02d%02d%02d", now.year, now.month, now.day, now.hour, now.minute, now.second);
}

static String Scene3DExportRootDir() {
	return ShareDirFile(AppendFileName("scene3d", "exports"));
}

static String Scene3DExportRelPath(const String& rel) {
	if (IsFullPath(rel))
		return AppendFileName(AppendFileName("data", "external"), GetFileName(rel));
	return rel;
}

bool Edit3D::ExportExecutionProject(bool full_assets) {
	if (!state || !prj) {
		PromptOK("No project loaded.");
		return false;
	}
	SyncPointcloudDatasetsExternalFiles();
	SyncAssetExternalFiles();
	SyncTextureExternalFiles();
	
	String root = Scene3DExportRootDir();
	RealizeDirectory(root);
	String base = scene3d_path.IsEmpty() ? String("untitled") : GetFileTitle(scene3d_path);
	if (base.IsEmpty())
		base = "project";
	String export_dir = AppendFileName(root, base + "_" + Scene3DExportTimeStamp());
	RealizeDirectory(export_dir);
	
	auto ExecIsoTime = [] {
		Time now = GetUtcTime();
		String date = Format("%04d-%02d-%02d", now.year, now.month, now.day);
		String clock = Format("%02d:%02d:%02d", now.hour, now.minute, now.second);
		return date + "T" + clock + "Z";
	};
	Scene3DDocument doc;
	doc.version = SCENE3D_VERSION;
	doc.name = "ModelerApp";
	doc.project = prj;
	doc.active_scene = state->active_scene;
	doc.focus = &state->GetFocus();
	doc.program = &state->GetProgram();
	if (scene3d_created.IsEmpty())
		scene3d_created = ExecIsoTime();
	scene3d_modified = ExecIsoTime();
	if (scene3d_data_dir.IsEmpty())
		scene3d_data_dir = "data";
	doc.created_utc = scene3d_created;
	doc.modified_utc = scene3d_modified;
	doc.data_dir = scene3d_data_dir;
	doc.external_files.Clear();
	for (const Scene3DExternalFile& file : scene3d_external_files)
		doc.external_files.Add(file);
	doc.meta.Clear();
	for (const Scene3DMetaEntry& entry : scene3d_meta)
		doc.meta.Add(entry);
	
	String scene_name = base + ".scene3d";
	String scene_path = AppendFileName(export_dir, scene_name);
	if (!SaveScene3DJson(scene_path, doc, true)) {
		PromptOK("Export failed: could not write scene3d.");
		return false;
	}
	
	ExecutionManifest manifest;
	manifest.version = "1";
	manifest.mode = full_assets ? "full" : "lightweight";
	manifest.exported_utc = ExecIsoTime();
	manifest.project_name = base;
	manifest.scene3d = scene_name;
	manifest.project_dir = project_dir;
	manifest.data_dir = scene3d_data_dir;
	
	Vector<GeomScript*> scripts;
	if (state && state->HasActiveScene()) {
		GeomScene& scene = GetActiveScene();
		GetScriptsFromNode(scene.val, scripts);
		for (auto& sub : scene.val.sub) {
			hash_t dir_hash = TypedStringHasher<GeomDirectory>("GeomDirectory");
			if (IsVfsType(sub, dir_hash))
				GetScriptsFromNode(sub, scripts);
		}
		for (GeomObject& obj : GeomObjectCollection(scene))
			GetScriptsFromNode(obj.val, scripts);
	}
	auto add_mapping = [&](const String& type, const String& src, const String& rel) {
		ExecutionFileMapping map;
		map.type = type;
		map.source = src;
		map.exported = rel;
		manifest.files.Add(map);
	};
	for (const Scene3DExternalFile& file : scene3d_external_files) {
		if (file.path.IsEmpty())
			continue;
		String src = IsFullPath(file.path) ? file.path : AppendFileName(project_dir, file.path);
		String rel = Scene3DExportRelPath(file.path);
		String dst = AppendFileName(export_dir, rel);
		if (full_assets) {
			RealizeDirectory(GetFileFolder(dst));
			if (FileExists(src))
				FileCopy(src, dst);
		}
		add_mapping(file.type, src, rel);
	}
	for (GeomScript* script : scripts) {
		if (!script || script->file.IsEmpty())
			continue;
		String src = IsFullPath(script->file) ? script->file : AppendFileName(project_dir, script->file);
		String rel = Scene3DExportRelPath(script->file);
		String dst = AppendFileName(export_dir, rel);
		if (full_assets) {
			RealizeDirectory(GetFileFolder(dst));
			if (FileExists(src))
				FileCopy(src, dst);
		}
		add_mapping("script", src, rel);
	}
	
	String manifest_path = AppendFileName(export_dir, "project.exec.json");
	SaveExecutionManifest(manifest_path, manifest, true);
	
	PromptOK(full_assets
		? "Exported full execution project."
		: "Exported lightweight execution project.");
	return true;
}

Edit3D::ExecutionWindow::ExecutionWindow() {
	Title("Execution");
	Sizeable();
	Add(renderer.SizePos());
}

void Edit3D::ExecutionWindow::Init(Edit3D* o) {
	owner = o;
	if (owner && owner->render_ctx.conf)
		conf = *owner->render_ctx.conf;
	conf.dump_grid_path.Clear();
	conf.dump_grid_done = false;
	ctx.conf = &conf;
	ctx.state = owner ? owner->state : nullptr;
	ctx.anim = owner ? owner->anim : nullptr;
	ctx.video = owner ? &owner->video : nullptr;
	ctx.show_hud = false;
	ctx.show_hud_help = false;
	ctx.selection_gizmo_enabled = false;
	ctx.selection_kind = 0;
	renderer.ctx = &ctx;
	renderer.SetViewMode(VIEWMODE_PERSPECTIVE);
	renderer.SetCameraSource(CAMSRC_PROGRAM);
	renderer.SetCameraInputEnabled(false);
	renderer.WhenInput = [this](const String& type, const Point& p, dword flags, int key) {
		if (owner)
			owner->DispatchInputEvent(type, p, flags, key, 0);
	};
	if (owner)
		owner->RegisterExternalRenderer(&renderer);
	WhenClose = THISBACK(CloseWindow);
	tc.Set(-1000/60, THISBACK(RefreshFrame));
}

void Edit3D::ExecutionWindow::RefreshFrame() {
	if (owner && owner->exec_exit_requested) {
		CloseWindow();
		return;
	}
	renderer.Refresh();
}

void Edit3D::ExecutionWindow::CloseWindow() {
	tc.Kill();
	if (owner)
		owner->UnregisterExternalRenderer(&renderer);
	if (owner) {
		Edit3D* o = owner;
		owner = nullptr;
		o->PostCallback([o] { o->exec_win.Clear(); });
	}
}

void Edit3D::DockInit() {
	dock_tree = &Dockable(v0.tree, "SceneCraft Explorer");
	dock_props = &Dockable(v0.props, "Properties");
	dock_time = &Dockable(v0.time, "Timeline");
	dock_video = &Dockable(v1, "Video Import");
	dock_pool = &Dockable(file_pool, "File Pool");
	dock_assets = &Dockable(asset_browser, "Assets");
	dock_texture = &Dockable(texture_edit, "Texture");
	dock_script = &Dockable(script_editor, "Script Editor");
	dock_tools = &Dockable(tool_panel, "Tools");

	dock_tree->SizeHint(Size(260, 600));
	dock_props->SizeHint(Size(360, 600));
	dock_time->SizeHint(Size(900, 200));
	dock_video->SizeHint(Size(900, 300));
	dock_pool->SizeHint(Size(320, 600));
	dock_assets->SizeHint(Size(360, 600));
	dock_texture->SizeHint(Size(320, 420));
	dock_script->SizeHint(Size(480, 600));
	dock_tools->SizeHint(Size(260, 120));

	DockLeft(*dock_tools, 0);
	DockLeft(*dock_tree, 0);
	DockLeft(*dock_props, 1);
	DockBottom(*dock_time);
	DockBottom(*dock_video);
	DockRight(*dock_pool);
	DockRight(*dock_assets);
	DockRight(*dock_texture);
	DockRight(*dock_script);

	Close(*dock_video);
	Close(*dock_pool);
	Close(*dock_assets);
	Close(*dock_texture);
	Close(*dock_script);
	v0.grid.SetFocus();

	if (GetLayouts().Find("Default") < 0)
		SaveLayout("Default");

	String cfg = ModelerAppConfigPath();
	if (FileExists(cfg))
		LoadFromFile(*this, cfg);
	ribbon.SetDisplayMode(ribbon_display_mode);
	ribbon.SetQuickAccessPos(ribbon_qat_pos);
	v0.RestoreTreeOpenState();
	SetEditTool(edit_tool);
}

bool Edit3D::Key(dword key, int count) {
	if (key & K_UP)
		return DockWindow::Key(key, count);
	Ctrl* focus = GetFocusCtrl();
	if (focus && dynamic_cast<EditField*>(focus))
		return DockWindow::Key(key, count);
	if (key == K_F1) {
		show_hud_help = !show_hud_help;
		RefrehRenderers();
		return true;
	}
	if (key == (K_CTRL | K_H)) {
		show_hud = !show_hud;
		RefrehRenderers();
		return true;
	}
	bool alt = key & K_ALT;
	bool ctrl = key & K_CTRL;
	bool shift = key & K_SHIFT;
	dword base = key & ~(K_ALT|K_CTRL|K_SHIFT);
	if (alt) {
		GeomObject* obj = v0.selected_obj;
		if (!obj || obj->is_locked)
			return DockWindow::Key(key, count);
		if (!ctrl) {
			double step = shift ? transform_nudge_large : transform_nudge_small;
			vec3 delta(0, 0, 0);
			if (base == K_LEFT) delta = VEC_LEFT * (float)step;
			else if (base == K_RIGHT) delta = VEC_RIGHT * (float)step;
			else if (base == K_UP) delta = VEC_UP * (float)step;
			else if (base == K_DOWN) delta = VEC_DOWN * (float)step;
			else if (base == K_PAGEUP) delta = VEC_FWD * (float)step;
			else if (base == K_PAGEDOWN) delta = VEC_BWD * (float)step;
			if (delta != vec3(0, 0, 0) && ApplyTransformDelta(obj, delta, transform_use_local))
				return true;
		}
		else {
			double step = shift ? transform_rot_large : transform_rot_small;
			if (transform_angle_snap && transform_angle_step > 1e-12) {
				if (shift) {
					double mult = max(1.0, floor(transform_rot_large / transform_angle_step + 0.5));
					step = transform_angle_step * mult;
				}
				else {
					step = transform_angle_step;
				}
			}
			int axis = -1;
			double angle = step;
			if (base == K_LEFT) { axis = 1; angle = -step; }
			else if (base == K_RIGHT) { axis = 1; angle = step; }
			else if (base == K_UP) { axis = 0; angle = -step; }
			else if (base == K_DOWN) { axis = 0; angle = step; }
			else if (base == K_PAGEUP) { axis = 2; angle = step; }
			else if (base == K_PAGEDOWN) { axis = 2; angle = -step; }
			if (axis >= 0 && ApplyTransformRotation(obj, axis, angle, transform_use_local))
				return true;
		}
	}
	return DockWindow::Key(key, count);
}

void Edit3D::DragAndDrop(Point p, PasteClip& d) {
	if (AcceptText(d)) {
		if (d.IsPaste()) {
			String path = GetString(d);
			if (!path.IsEmpty())
				AddAssetFromPath(path);
		}
		d.SetAction(DND_COPY);
		return;
	}
	DockWindow::DragAndDrop(p, d);
}

void Edit3D::SetView(ViewType view) {
	this->view = view;
	if (this->view == VIEW_GEOMPROJECT) {
		if (dock_video)
			Close(*dock_video);
		v0.grid.Show();
		v0.grid.SetFocus();
	}
	else if (this->view == VIEW_VIDEOIMPORT) {
		v0.grid.Hide();
		if (dock_video)
			ActivateDockableChild(v1);
	}
}

void Edit3D::RefrehToolbar() {
	// Playback controls are hosted in RibbonBar.
}

static void FillEditableMeshFromMesh(GeomEditableMesh& out, const Mesh& mesh)
{
	out.points.Clear();
	out.lines.Clear();
	out.faces.Clear();
	out.points.SetCount(mesh.vertices.GetCount());
	for (int i = 0; i < mesh.vertices.GetCount(); i++)
		out.points[i] = mesh.vertices[i].position.Splice();
	if (mesh.use_quad) {
		int quad_count = mesh.indices.GetCount() / 4;
		out.faces.SetCount(quad_count * 2);
		for (int i = 0; i < quad_count; i++) {
			int a = mesh.indices[i * 4 + 0];
			int b = mesh.indices[i * 4 + 1];
			int c = mesh.indices[i * 4 + 2];
			int d = mesh.indices[i * 4 + 3];
			out.faces[i * 2 + 0].a = a;
			out.faces[i * 2 + 0].b = b;
			out.faces[i * 2 + 0].c = c;
			out.faces[i * 2 + 1].a = a;
			out.faces[i * 2 + 1].b = c;
			out.faces[i * 2 + 1].c = d;
		}
	}
	else {
		int tri_count = mesh.indices.GetCount() / 3;
		out.faces.SetCount(tri_count);
		for (int i = 0; i < tri_count; i++) {
			out.faces[i].a = mesh.indices[i * 3 + 0];
			out.faces[i].b = mesh.indices[i * 3 + 1];
			out.faces[i].c = mesh.indices[i * 3 + 2];
		}
	}
}

static GeomDirectory& GetCreateTargetDirectory(Edit3D& e)
{
	GeomDirectory* dir = nullptr;
	if (e.v0.selected_ref && e.v0.selected_ref->kind == GeomProjectCtrl::TreeNodeRef::K_VFS && e.v0.selected_ref->vfs)
		dir = GetNodeDirectory(*e.v0.selected_ref->vfs);
	if (!dir)
		dir = &e.state->GetActiveScene();
	return *dir;
}

static void SetDynamicProp(GeomObject& obj, const String& key, const Value& val)
{
	GeomDynamicProperties& props = obj.GetDynamicProperties();
	props.props.GetAdd(key, val);
}

static void SetDynamicProp(GeomDirectory& dir, const String& key, const Value& val)
{
	GeomDynamicProperties& props = dir.GetDynamicProperties();
	props.props.GetAdd(key, val);
}

static GeomObject& CreateModelObject(Edit3D& e, const String& base)
{
	GeomDirectory& dir = GetCreateTargetDirectory(e);
	String name = base;
	int idx = 1;
	while (dir.FindObject(name))
		name = base + "_" + IntStr(idx++);
	GeomObject& obj = dir.GetAddModel(name);
	obj.type = GeomObject::O_MODEL;
	return obj;
}

static GeomObject& CreateTaggedModel(Edit3D& e, const String& base, const String& tag)
{
	GeomObject& obj = CreateModelObject(e, base);
	if (!tag.IsEmpty())
		obj.ui_type = tag;
	return obj;
}

struct PrimitiveDialogBase : TopWindow {
	Button ok;
	Button cancel;
	int row_y = 10;
	int row_h = 24;
	int label_w = 120;
	int field_w = 120;
	int gap = 8;

	PrimitiveDialogBase()
	{
		Sizeable(false);
		ok.SetLabel("OK");
		cancel.SetLabel("Cancel");
		ok.WhenAction = [=] { AcceptBreak(IDOK); };
		cancel.WhenAction = [=] { RejectBreak(IDCANCEL); };
	}

	void AddButtons()
	{
		int y = row_y + gap;
		ok.SetRect(10, y, 80, 24);
		cancel.SetRect(100, y, 80, 24);
		Add(ok);
		Add(cancel);
		SetRect(0, 0, 220, y + 40);
	}

	void AddRow(const String& label, Ctrl& field)
	{
		Label& lbl = *new Label;
		lbl.SetLabel(label);
		Add(lbl);
		lbl.SetRect(10, row_y, label_w, row_h);
		field.SetRect(10 + label_w + gap, row_y, field_w, row_h);
		Add(field);
		row_y += row_h + 4;
	}

	void AddRowLeft(Ctrl& field)
	{
		field.SetRect(10, row_y, label_w + field_w + gap, row_h);
		Add(field);
		row_y += row_h + 4;
	}
};

static bool DialogCreateCube(double& size)
{
	struct Dlg : PrimitiveDialogBase {
		EditDoubleSpin size;
		Dlg()
		{
			Title("Create a cube");
			size.SetData(10.0);
			AddRow("Size:", size);
			AddButtons();
		}
	} dlg;
	if (dlg.Run() == IDOK) {
		size = (double)dlg.size.GetData();
		return true;
	}
	return false;
}

static bool DialogCreateSphere(double& radius, int& poly)
{
	struct Dlg : PrimitiveDialogBase {
		EditDoubleSpin radius;
		EditIntSpin poly;
		Dlg()
		{
			Title("Create a sphere");
			radius.SetData(5.0);
			poly.SetData(128);
			AddRow("Radius:", radius);
			AddRow("Polycount:", poly);
			AddButtons();
		}
	} dlg;
	if (dlg.Run() == IDOK) {
		radius = (double)dlg.radius.GetData();
		poly = (int)dlg.poly.GetData();
		return true;
	}
	return false;
}

static bool DialogCreateCylinder(double& radius, int& sides, double& length, bool& cap_top, bool& cap_bottom)
{
	struct Dlg : PrimitiveDialogBase {
		EditDoubleSpin radius;
		EditIntSpin sides;
		EditDoubleSpin length;
		Option cap_top;
		Option cap_bottom;
		Dlg()
		{
			Title("Create a cylinder");
			radius.SetData(5.0);
			sides.SetData(8);
			length.SetData(10.0);
			cap_top.SetLabel("Cap top");
			cap_bottom.SetLabel("Cap bottom");
			AddRow("Radius:", radius);
			AddRow("Side count:", sides);
			AddRow("Length:", length);
			AddRowLeft(cap_top);
			AddRowLeft(cap_bottom);
			AddButtons();
		}
	} dlg;
	if (dlg.Run() == IDOK) {
		radius = (double)dlg.radius.GetData();
		sides = (int)dlg.sides.GetData();
		length = (double)dlg.length.GetData();
		cap_top = dlg.cap_top;
		cap_bottom = dlg.cap_bottom;
		return true;
	}
	return false;
}

static bool DialogCreateCone(double& radius, int& sides, double& length, bool& cap_bottom)
{
	struct Dlg : PrimitiveDialogBase {
		EditDoubleSpin radius;
		EditIntSpin sides;
		EditDoubleSpin length;
		Option cap_bottom;
		Dlg()
		{
			Title("Create a cone");
			radius.SetData(5.0);
			sides.SetData(8);
			length.SetData(10.0);
			cap_bottom.SetLabel("Cap bottom");
			AddRow("Radius:", radius);
			AddRow("Side count:", sides);
			AddRow("Length:", length);
			AddRowLeft(cap_bottom);
			AddButtons();
		}
	} dlg;
	if (dlg.Run() == IDOK) {
		radius = (double)dlg.radius.GetData();
		sides = (int)dlg.sides.GetData();
		length = (double)dlg.length.GetData();
		cap_bottom = dlg.cap_bottom;
		return true;
	}
	return false;
}

static bool DialogCreatePlane(int& tiles, double& width, double& length, int& repeat)
{
	struct Dlg : PrimitiveDialogBase {
		EditIntSpin tiles;
		EditDoubleSpin width;
		EditDoubleSpin length;
		EditIntSpin repeat;
		Dlg()
		{
			Title("Create a plane");
			tiles.SetData(4);
			width.SetData(30.0);
			length.SetData(30.0);
			repeat.SetData(1);
			AddRow("Tile count:", tiles);
			AddRow("Width:", width);
			AddRow("Length:", length);
			AddRow("Repeat count:", repeat);
			AddButtons();
		}
	} dlg;
	if (dlg.Run() == IDOK) {
		tiles = (int)dlg.tiles.GetData();
		width = (double)dlg.width.GetData();
		length = (double)dlg.length.GetData();
		repeat = (int)dlg.repeat.GetData();
		return true;
	}
	return false;
}

static bool DialogCreateCamera(String& cam_type, bool& collides, bool& first_active)
{
	struct Dlg : PrimitiveDialogBase {
		Option opt_simple;
		Option opt_fps;
		Option opt_third;
		Option opt_free;
		Option opt_viewer;
		Option opt_panorama;
		Option collides;
		Option first_active;
		void Select(Option& opt)
		{
			opt_simple = false;
			opt_fps = false;
			opt_third = false;
			opt_free = false;
			opt_viewer = false;
			opt_panorama = false;
			opt = true;
		}
		Dlg()
		{
			Title("Create a camera");
			opt_simple.SetLabel("Simple camera");
			opt_fps.SetLabel("First person shooter camera");
			opt_third.SetLabel("Third person camera");
			opt_free.SetLabel("Free-flying camera");
			opt_viewer.SetLabel("Model viewer camera");
			opt_panorama.SetLabel("Panorama camera");
			Select(opt_simple);
			opt_simple.WhenAction = [this] { Select(opt_simple); };
			opt_fps.WhenAction = [this] { Select(opt_fps); };
			opt_third.WhenAction = [this] { Select(opt_third); };
			opt_free.WhenAction = [this] { Select(opt_free); };
			opt_viewer.WhenAction = [this] { Select(opt_viewer); };
			opt_panorama.WhenAction = [this] { Select(opt_panorama); };
			collides.SetLabel("Collides against geometry when moved");
			first_active.SetLabel("When starting the scene, this is the first active camera");
			first_active = true;
			AddRowLeft(opt_simple);
			AddRowLeft(opt_fps);
			AddRowLeft(opt_third);
			AddRowLeft(opt_free);
			AddRowLeft(opt_viewer);
			AddRowLeft(opt_panorama);
			AddRowLeft(collides);
			AddRowLeft(first_active);
			AddButtons();
		}
		String GetType() const
		{
			if (opt_fps) return "fps";
			if (opt_third) return "third_person";
			if (opt_free) return "free_flying";
			if (opt_viewer) return "model_viewer";
			if (opt_panorama) return "panorama";
			return "simple";
		}
	} dlg;
	if (dlg.Run() == IDOK) {
		cam_type = dlg.GetType();
		collides = dlg.collides;
		first_active = dlg.first_active;
		return true;
	}
	return false;
}

static bool DialogCreateWater(int& tiles, double& width, double& length)
{
	struct Dlg : PrimitiveDialogBase {
		EditIntSpin tiles;
		EditDoubleSpin width;
		EditDoubleSpin length;
		Dlg()
		{
			Title("Create a Water Surface");
			tiles.SetData(10);
			width.SetData(1000.0);
			length.SetData(1000.0);
			AddRow("Tile count:", tiles);
			AddRow("Width:", width);
			AddRow("Length:", length);
			AddButtons();
		}
	} dlg;
	if (dlg.Run() == IDOK) {
		tiles = (int)dlg.tiles.GetData();
		width = (double)dlg.width.GetData();
		length = (double)dlg.length.GetData();
		return true;
	}
	return false;
}

static bool DialogCreateTree()
{
	struct Dlg : PrimitiveDialogBase {
		Label info;
		Dlg()
		{
			Title("Generate new tree");
			info.SetLabel("Generate a new procedural tree node with default settings.");
			AddRowLeft(info);
			AddButtons();
		}
	} dlg;
	return dlg.Run() == IDOK;
}

static bool DialogCreateTerrain(double& side_length, double& tile_size, double& max_height,
                                String& topology, bool& create_trees, bool& create_grass)
{
	struct Dlg : PrimitiveDialogBase {
		EditDoubleSpin side_len;
		EditDoubleSpin tile;
		EditDoubleSpin max_h;
		DropList topo;
		Option create_trees;
		Option create_grass;
		Dlg()
		{
			Title("Create a terrain");
			side_len.SetData(1500.0);
			tile.SetData(20.0);
			max_h.SetData(100.0);
			topo.Add("Hills");
			topo.Add("Desert");
			topo.Add("Flat");
			topo.SetIndex(0);
			create_trees.SetLabel("Create Trees");
			create_grass.SetLabel("Create Grass");
			create_trees = true;
			create_grass = true;
			AddRow("Side length:", side_len);
			AddRow("Tile size:", tile);
			AddRow("Max height:", max_h);
			AddRow("Topology:", topo);
			AddRowLeft(create_trees);
			AddRowLeft(create_grass);
			AddButtons();
		}
	} dlg;
	if (dlg.Run() == IDOK) {
		side_length = (double)dlg.side_len.GetData();
		tile_size = (double)dlg.tile.GetData();
		max_height = (double)dlg.max_h.GetData();
		topology = AsString(dlg.topo.GetData());
		create_trees = dlg.create_trees;
		create_grass = dlg.create_grass;
		return true;
	}
	return false;
}

static bool DialogCreateRoomMesh(String& ceiling_type, double& wall_height, String& ceiling_tex)
{
	struct Dlg : PrimitiveDialogBase {
		Label info;
		DropList ceiling;
		EditDoubleSpin wall_h;
		EditString ceiling_tex;
		Dlg()
		{
			Title("Create a Room");
			info.SetLabel("Room map editor is not implemented yet.");
			AddRowLeft(info);
			ceiling.Add("Arcade");
			ceiling.Add("Flat");
			ceiling.Add("No ceiling");
			ceiling.SetIndex(0);
			wall_h.SetData(1.5);
			AddRow("Ceiling type:", ceiling);
			AddRow("Wall height:", wall_h);
			AddRow("Ceiling texture:", ceiling_tex);
			AddButtons();
		}
	} dlg;
	if (dlg.Run() == IDOK) {
		ceiling_type = AsString(dlg.ceiling.GetData());
		wall_height = (double)dlg.wall_h.GetData();
		ceiling_tex = AsString(dlg.ceiling_tex.GetData());
		return true;
	}
	return false;
}

static bool GetRibbonDropValue(ModelerAppRibbon& ribbon, const char* id, String& out)
{
	Ctrl* c = ribbon.FindControl(id);
	DropList* dl = dynamic_cast<DropList*>(c);
	if (!dl || IsNull(dl->GetData()))
		return false;
	out = AsString(dl->GetData());
	return true;
}

static bool GetRibbonIntValue(ModelerAppRibbon& ribbon, const char* id, int& out)
{
	Ctrl* c = ribbon.FindControl(id);
	EditIntSpin* ed = dynamic_cast<EditIntSpin*>(c);
	if (!ed)
		return false;
	out = (int)ed->GetData();
	return true;
}

static bool GetRibbonDoubleValue(ModelerAppRibbon& ribbon, const char* id, double& out)
{
	Ctrl* c = ribbon.FindControl(id);
	EditDoubleSpin* ed = dynamic_cast<EditDoubleSpin*>(c);
	if (!ed)
		return false;
	out = (double)ed->GetData();
	return true;
}

static bool GetRibbonBoolValue(ModelerAppRibbon& ribbon, const char* id, bool& out)
{
	Ctrl* c = ribbon.FindControl(id);
	Option* opt = dynamic_cast<Option*>(c);
	if (!opt)
		return false;
	out = (bool)*opt;
	return true;
}

static bool SetRibbonDropValue(ModelerAppRibbon& ribbon, const char* id, const Value& v)
{
	Ctrl* c = ribbon.FindControl(id);
	DropList* dl = dynamic_cast<DropList*>(c);
	if(!dl)
		return false;
	dl->SetData(v);
	return true;
}

static bool SetRibbonIntValue(ModelerAppRibbon& ribbon, const char* id, int v)
{
	Ctrl* c = ribbon.FindControl(id);
	EditIntSpin* ed = dynamic_cast<EditIntSpin*>(c);
	if(!ed)
		return false;
	ed->SetData(v);
	return true;
}

static bool SetRibbonDoubleValue(ModelerAppRibbon& ribbon, const char* id, double v)
{
	Ctrl* c = ribbon.FindControl(id);
	EditDoubleSpin* ed = dynamic_cast<EditDoubleSpin*>(c);
	if(!ed)
		return false;
	ed->SetData(v);
	return true;
}

static bool SetRibbonBoolValue(ModelerAppRibbon& ribbon, const char* id, bool v)
{
	Ctrl* c = ribbon.FindControl(id);
	Option* opt = dynamic_cast<Option*>(c);
	if(!opt)
		return false;
	opt->Set(v ? 1 : 0);
	return true;
}

static void SyncRibbonSceneSelector(Edit3D& e)
{
	Ctrl* c = e.ribbon.FindControl("scene_selector");
	DropList* dl = dynamic_cast<DropList*>(c);
	if (!dl)
		return;
	Event<> cb = dl->WhenAction;
	dl->WhenAction = Event<>();
	dl->Clear();
	dl->Add("new_scene", "New 3D Scene");
	int selected = 0;
	if (e.prj) {
		for (int i = 0; i < e.prj->GetSceneCount(); i++) {
			GeomScene& scene = e.prj->GetScene(i);
			String id = "scene_" + IntStr(i);
			String label = scene.name.IsEmpty() ? Format("Scene #%d", i) : scene.name;
			dl->Add(id, label);
			if (e.state && i == e.state->active_scene)
				selected = i + 1;
		}
	}
	if (dl->GetCount() > 0)
		dl->SetIndex(minmax(selected, 0, dl->GetCount() - 1));
	dl->WhenAction = cb;
}

static void SyncRibbonPublishTarget(Edit3D& e)
{
	if (!e.state || !e.state->HasActiveScene())
		return;
	String target = "win_exe";
	if (GeomDynamicProperties* props = e.state->GetActiveScene().FindDynamicProperties()) {
		int i = props->props.Find("publish_target");
		if (i >= 0)
			target = AsString(props->props[i]);
	}
	SetRibbonDropValue(e.ribbon, "publish_target", target);
}

void Edit3D::CreatePrimitiveCube(double size)
{
	if (!state)
		return;
	PushUndo("Create Cube");
	GeomDirectory& dir = GetCreateTargetDirectory(*this);
	String base = "cube";
	String name = base;
	int idx = 1;
	while (dir.FindObject(name))
		name = base + "_" + IntStr(idx++);
	GeomObject& obj = dir.GetAddModel(name);
	obj.type = GeomObject::O_MODEL;
	GeomEditableMesh& emesh = obj.GetEditableMesh();
	Mesh mesh;
	MeshFactory::CreateBox(mesh, (float)size, (float)size, (float)size);
	FillEditableMeshFromMesh(emesh, mesh);
	state->UpdateObjects();
	RefreshData();
}

void Edit3D::CreatePrimitiveSphere(double radius, int slices, int stacks)
{
	if (!state)
		return;
	PushUndo("Create Sphere");
	GeomDirectory& dir = GetCreateTargetDirectory(*this);
	String base = "sphere";
	String name = base;
	int idx = 1;
	while (dir.FindObject(name))
		name = base + "_" + IntStr(idx++);
	GeomObject& obj = dir.GetAddModel(name);
	obj.type = GeomObject::O_MODEL;
	GeomEditableMesh& emesh = obj.GetEditableMesh();
	Mesh mesh;
	MeshFactory::CreateSphere(mesh, slices, stacks, (float)radius);
	FillEditableMeshFromMesh(emesh, mesh);
	state->UpdateObjects();
	RefreshData();
}

void Edit3D::CreatePrimitiveCylinder(double radius, double length, int slices)
{
	if (!state)
		return;
	PushUndo("Create Cylinder");
	GeomDirectory& dir = GetCreateTargetDirectory(*this);
	String base = "cylinder";
	String name = base;
	int idx = 1;
	while (dir.FindObject(name))
		name = base + "_" + IntStr(idx++);
	GeomObject& obj = dir.GetAddModel(name);
	obj.type = GeomObject::O_MODEL;
	GeomEditableMesh& emesh = obj.GetEditableMesh();
	Mesh mesh;
	MeshFactory::CreateCylinder(mesh, slices, (float)radius, (float)length);
	FillEditableMeshFromMesh(emesh, mesh);
	state->UpdateObjects();
	RefreshData();
}

void Edit3D::CreatePrimitiveCone(double radius, double length, int slices)
{
	if (!state)
		return;
	PushUndo("Create Cone");
	GeomDirectory& dir = GetCreateTargetDirectory(*this);
	String base = "cone";
	String name = base;
	int idx = 1;
	while (dir.FindObject(name))
		name = base + "_" + IntStr(idx++);
	GeomObject& obj = dir.GetAddModel(name);
	obj.type = GeomObject::O_MODEL;
	GeomEditableMesh& emesh = obj.GetEditableMesh();
	Mesh mesh;
	MeshFactory::CreateCone(mesh, slices, (float)radius, (float)length);
	FillEditableMeshFromMesh(emesh, mesh);
	state->UpdateObjects();
	RefreshData();
}

GeomObject* Edit3D::CreatePrimitivePlane(int tiles, double width, double length, int repeat)
{
	if (!state)
		return nullptr;
	PushUndo("Create Plane");
	GeomDirectory& dir = GetCreateTargetDirectory(*this);
	String base = "plane";
	String name = base;
	int idx = 1;
	while (dir.FindObject(name))
		name = base + "_" + IntStr(idx++);
	GeomObject& obj = dir.GetAddModel(name);
	obj.type = GeomObject::O_MODEL;
	GeomEditableMesh& emesh = obj.GetEditableMesh();
	Mesh mesh;
	MeshFactory::CreateGridTriangles(mesh, tiles, tiles, (float)width, (float)length, (float)repeat, (float)repeat);
	FillEditableMeshFromMesh(emesh, mesh);
	state->UpdateObjects();
	RefreshData();
	return &obj;
}

void Edit3D::Toolbar(Bar& bar) {
	bar.Add(true,  t_("Stop"),  ImagesImg::Stop(),  THISBACK(Stop)).Key(K_F6);
	
	if (anim->is_playing)
		bar.Add(true, t_("Pause"), ImagesImg::Pause(), THISBACK(Pause)).Key(K_F5);
	else
		bar.Add(true,  t_("Play"),  ImagesImg::Play(),  THISBACK(Play)).Key(K_F5);
	bar.Separator();
	bar.Add(t_("Repeat"), THISBACK(ToggleRepeatPlayback)).Check(repeat_playback);
	
}

void Edit3D::UpdateRibbonContext()
{
	bool has_obj = v0.selected_obj;
	bool camera_context = has_obj && v0.selected_obj->IsCamera();
	bool mesh_context = false;
	bool material_context = false;
	if(has_obj) {
		mesh_context = v0.selected_obj->IsModel();
		if(mesh_context) {
			Model* mdl = v0.selected_obj->mdl.Get();
			if(mdl && mdl->materials.GetCount() > 0)
				material_context = true;
			else if(v0.selected_obj->Find2DLayer())
				material_context = true;
		}
	}
	ribbon.ShowContext("camera", camera_context);
	ribbon.ShowContext("object", has_obj);
	ribbon.ShowContext("mesh", mesh_context);
	ribbon.ShowContext("material", material_context);
	SyncRibbonSceneSelector(*this);
	SyncRibbonPublishTarget(*this);
}

void Edit3D::SyncRibbonLightmapControls()
{
	if(!state || !state->HasActiveScene())
		return;
	GeomScene& scene = state->GetActiveScene();
	GeomDynamicProperties* props = scene.FindDynamicProperties();
	if(!props)
		return;

	auto find_prop = [&](const char* key) -> const Value* {
		int i = props->props.Find(key);
		return i >= 0 ? &props->props[i] : nullptr;
	};

	if(const Value* v = find_prop("lightmap_mode"))
		SetRibbonDropValue(ribbon, "lightmap_mode", *v);
	if(const Value* v = find_prop("lightmap_texture_size"))
		SetRibbonDropValue(ribbon, "lightmap_texture_size", *v);
	if(const Value* v = find_prop("lightmap_subsampling"))
		SetRibbonDropValue(ribbon, "lightmap_subsampling", *v);
	if(const Value* v = find_prop("lightmap_resolution"))
		SetRibbonIntValue(ribbon, "lightmap_resolution", (int)*v);
	if(const Value* v = find_prop("lightmap_shadow_opacity"))
		SetRibbonDoubleValue(ribbon, "lightmap_shadow_opacity", (double)*v);
	if(const Value* v = find_prop("lightmap_ambient_enabled"))
		SetRibbonBoolValue(ribbon, "ambient_light_enabled", (bool)*v);
	if(const Value* v = find_prop("lightmap_smooth_normals"))
		SetRibbonBoolValue(ribbon, "smooth_normals_enabled", (bool)*v);
	if(const Value* v = find_prop("lightmap_color_bleeding"))
		SetRibbonBoolValue(ribbon, "color_bleeding_enabled", (bool)*v);
}

void Edit3D::StoreRibbonLightmapSettings(bool bake)
{
	if(!state || !state->HasActiveScene())
		return;

	GeomScene& scene = state->GetActiveScene();
	String mode, tex_size, subsampling;
	int resolution = 0;
	double shadow_opacity = 0.0;
	bool ambient = false;
	bool smooth = false;
	bool color_bleed = false;

	GetRibbonDropValue(ribbon, "lightmap_mode", mode);
	GetRibbonDropValue(ribbon, "lightmap_texture_size", tex_size);
	GetRibbonDropValue(ribbon, "lightmap_subsampling", subsampling);
	GetRibbonIntValue(ribbon, "lightmap_resolution", resolution);
	GetRibbonDoubleValue(ribbon, "lightmap_shadow_opacity", shadow_opacity);
	GetRibbonBoolValue(ribbon, "ambient_light_enabled", ambient);
	GetRibbonBoolValue(ribbon, "smooth_normals_enabled", smooth);
	GetRibbonBoolValue(ribbon, "color_bleeding_enabled", color_bleed);

	SetDynamicProp(scene, "lightmap_mode", mode);
	SetDynamicProp(scene, "lightmap_texture_size", tex_size);
	SetDynamicProp(scene, "lightmap_subsampling", subsampling);
	SetDynamicProp(scene, "lightmap_resolution", resolution);
	SetDynamicProp(scene, "lightmap_shadow_opacity", shadow_opacity);
	SetDynamicProp(scene, "lightmap_ambient_enabled", ambient);
	SetDynamicProp(scene, "lightmap_smooth_normals", smooth);
	SetDynamicProp(scene, "lightmap_color_bleeding", color_bleed);

	if(bake) {
		String summary;
		bool ok = BakeSceneLightmaps(summary);
		if(summary.IsEmpty())
			summary = ok ? "Lightmap bake complete." : "Lightmap bake failed.";
		PromptOK(summary);
	}
}

bool Edit3D::BakeSceneLightmaps(String& summary)
{
	if(!state || !state->HasActiveScene()) {
		summary = "No active scene.";
		return false;
	}

	GeomScene& scene = state->GetActiveScene();
	GeomDynamicProperties* props = scene.FindDynamicProperties();
	if(!props) {
		summary = "No scene lightmap settings found.";
		return false;
	}

	auto get_prop = [&](const char* key) -> const Value* {
		int i = props->props.Find(key);
		return i >= 0 ? &props->props[i] : nullptr;
	};
	auto get_int_prop = [&](const char* key, int def) -> int {
		if(const Value* v = get_prop(key))
			return (int)*v;
		return def;
	};
	auto get_str_prop = [&](const char* key, const String& def) -> String {
		if(const Value* v = get_prop(key))
			return (String)*v;
		return def;
	};
	auto get_bool_prop = [&](const char* key, bool def) -> bool {
		if(const Value* v = get_prop(key))
			return (bool)*v;
		return def;
	};
	auto get_double_prop = [&](const char* key, double def) -> double {
		if(const Value* v = get_prop(key))
			return (double)*v;
		return def;
	};

	String tex_size_str = get_str_prop("lightmap_texture_size", "256");
	const char* tex_end = nullptr;
	int tex_size = ScanInt(tex_size_str, &tex_end);
	if(!tex_end || *tex_end)
		tex_size = 256;
	tex_size = minmax(tex_size, 16, 2048);

	String mode = ToLower(get_str_prop("lightmap_mode", "wide"));
	String subsampling = ToLower(get_str_prop("lightmap_subsampling", "none"));
	int resolution = max(1, get_int_prop("lightmap_resolution", 1000));
	double shadow_opacity = minmax(get_double_prop("lightmap_shadow_opacity", 0.8), 0.0, 1.0);
	bool ambient = get_bool_prop("lightmap_ambient_enabled", true);
	bool smooth = get_bool_prop("lightmap_smooth_normals", true);
	bool color_bleed = get_bool_prop("lightmap_color_bleeding", true);

	int subsample = 1;
	if(subsampling == "4x") subsample = 2;
	else if(subsampling == "9x") subsample = 3;
	else if(subsampling == "16x") subsample = 4;
	else if(subsampling == "25x") subsample = 5;

	double mode_gain = 1.0;
	if(mode == "diffuse") mode_gain = 0.92;
	else if(mode == "shadows") mode_gain = 0.75;
	else if(mode == "gi") mode_gain = 1.08;
	double resolution_gain = min(1.25, 0.75 + (double)resolution / 2000.0);
	double smooth_gain = smooth ? 1.04 : 0.96;
	double bleed_gain = color_bleed ? 1.03 : 0.97;
	double ambient_gain = ambient ? 1.0 : 0.86;

	PushUndo("Bake Lightmaps");

	int model_count = 0;
	int material_count = 0;
	for(GeomObject& obj : GeomObjectCollection(scene)) {
		if(!obj.IsModel() || !obj.mdl)
			continue;
		Model& mdl = *obj.mdl;
		if(mdl.materials.IsEmpty())
			continue;

		model_count++;
		for(int i = 0; i < mdl.materials.GetCount(); i++) {
			int mat_id = mdl.materials.GetKey(i);
			Material& mat = mdl.materials[i];
			String tex_key = Format("gen_lightmap_%llx_%d", (long long)obj.key, mat_id);

			ImageBuffer ib(tex_size, tex_size);
			for(int y = 0; y < tex_size; y++) {
				double v = tex_size > 1 ? (double)y / (double)(tex_size - 1) : 0.0;
				RGBA* row = ib[y];
				for(int x = 0; x < tex_size; x++) {
					double u = tex_size > 1 ? (double)x / (double)(tex_size - 1) : 0.0;
					double gx = 0.85 + 0.15 * cos(u * M_PI * subsample);
					double gy = 0.85 + 0.15 * sin(v * M_PI * subsample);
					double g = gx * gy;
					double shade = mode_gain * resolution_gain * smooth_gain * bleed_gain * ambient_gain * g;
					shade *= (1.0 - 0.6 * shadow_opacity);
					shade = minmax(shade, 0.05, 1.0);
					byte c = (byte)minmax((int)floor(shade * 255.0 + 0.5), 0, 255);
					RGBA& px = row[x];
					px.r = c;
					px.g = c;
					px.b = c;
					px.a = 255;
				}
			}

			Image img = ib;
			int tex_id = mdl.FindTexture(tex_key);
			if(tex_id >= 0)
				mdl.textures.Get(tex_id).img.Set(img);
			else
				tex_id = mdl.AddTexture(img, tex_key);
			mat.tex_id[TEXTYPE_LIGHTMAP] = tex_id;
			mat.tex_filter[TEXTYPE_LIGHTMAP] = GVar::FILTER_LINEAR;
			material_count++;
		}
	}

	SetDynamicProp(scene, "lightmap_last_bake_utc", AsString(GetUtcTime()));
	SetDynamicProp(scene, "lightmap_last_bake_models", model_count);
	SetDynamicProp(scene, "lightmap_last_bake_materials", material_count);

	state->UpdateObjects();
	RefreshData();
	RefrehRenderers();

	if(material_count <= 0) {
		summary = "No model materials available for lightmap baking.";
		return false;
	}

	summary = Format("Lightmap bake complete.\nModels: %d\nMaterials: %d\nTexture size: %d",
	                 model_count, material_count, tex_size);
	return true;
}

void Edit3D::FocusSelectedNode()
{
	PointcloudPose pose;
	bool ok = false;
	if (v0.selected_obj && state) {
		const GeomObjectState* os = state->FindObjectStateByKey(v0.selected_obj->key);
		if (os) {
			pose.position = os->position;
			pose.orientation = os->orientation;
			ok = true;
		}
	}
	if (!ok && v0.selected_ref && state) {
		if (v0.selected_ref->kind == GeomProjectCtrl::TreeNodeRef::K_PROGRAM) {
			GeomCamera& cam = state->GetProgram();
			pose.position = cam.position;
			pose.orientation = cam.orientation;
			ok = true;
		}
		if (v0.selected_ref->kind == GeomProjectCtrl::TreeNodeRef::K_FOCUS) {
			GeomCamera& cam = state->GetFocus();
			pose.position = cam.position;
			pose.orientation = cam.orientation;
			ok = true;
		}
	}
	if (ok && state) {
		GeomCamera& focus = state->GetFocus();
		focus.position = pose.position;
		focus.orientation = pose.orientation;
		RefrehRenderers();
	}
}

bool Edit3D::HandleRibbonAction(const String& id) {
	String sid = ToLower(id);
	if (sid == "new_file") {
		LoadEmptyProject();
		return true;
	}
	if (sid == "open_file") {
		OpenScene3D();
		return true;
	}
	if (sid == "save_file") {
		SaveScene3DInteractive();
		return true;
	}
	if (sid == "undo") {
		Undo();
		return true;
	}
	if (sid == "redo") {
		Redo();
		return true;
	}
	if (sid == "play_timeline") {
		Play();
		return true;
	}
	if (sid == "pause_timeline") {
		Pause();
		return true;
	}
	if (sid == "stop_timeline") {
		Stop();
		return true;
	}
	if (sid == "repeat_playback") {
		ToggleRepeatPlayback();
		return true;
	}
	if (sid == "select_objects") {
		SetEditTool(TOOL_OBJ_SELECT);
		return true;
	}
	if (sid == "move_camera") {
		SetEditTool(TOOL_VIEW);
		return true;
	}
	if (sid == "move_object") {
		SetEditTool(TOOL_OBJ_MOVE);
		return true;
	}
	if (sid == "rotate_object") {
		SetEditTool(TOOL_OBJ_ROTATE);
		return true;
	}
	if (sid == "view_perspective" || sid == "view_top" || sid == "view_front" || sid == "view_left") {
		int mode = VIEWMODE_PERSPECTIVE;
		if (sid == "view_top")
			mode = VIEWMODE_XY;
		else if (sid == "view_front")
			mode = VIEWMODE_XZ;
		else if (sid == "view_left")
			mode = VIEWMODE_YZ;
		for (int i = 0; i < 4; i++) {
			if (v0.rends[i]) {
							v0.rends[i]->SetViewMode((ViewMode)mode);
				v0.RefreshRenderer(i);
			}
		}
		return true;
	}
	if (sid == "create_camera") {
		if (!state)
			return false;
		String cam_type;
		bool collides = false;
		bool first_active = true;
		if (!DialogCreateCamera(cam_type, collides, first_active))
			return false;
		GeomDirectory& dir = state->GetActiveScene();
		String base = "camera";
		String name = base;
		int idx = 1;
		while (dir.FindObject(name))
			name = base + "_" + IntStr(idx++);
		GeomObject& cam = dir.GetAddCamera(name);
		SetDynamicProp(cam, "camera_type", cam_type);
		SetDynamicProp(cam, "collides_geometry", collides);
		SetDynamicProp(cam, "first_active", first_active);
		state->UpdateObjects();
		if (first_active) {
			for (int i = 0; i < state->objs.GetCount(); i++) {
				if (state->objs[i].obj && state->objs[i].obj->key == cam.key) {
					state->active_camera_obj_i = i;
					break;
				}
			}
		}
		RefreshData();
		return true;
	}
	if (sid == "create_cube") {
		double size = 10.0;
		if (!DialogCreateCube(size))
			return false;
		CreatePrimitiveCube(size);
		return true;
	}
	if (sid == "create_sphere") {
		double radius = 5.0;
		int poly = 128;
		if (!DialogCreateSphere(radius, poly))
			return false;
		int slices = max(3, poly);
		int stacks = max(2, poly / 2);
		CreatePrimitiveSphere(radius, slices, stacks);
		return true;
	}
	if (sid == "create_cylinder") {
		double radius = 5.0;
		int sides = 8;
		double length = 10.0;
		bool cap_top = false;
		bool cap_bottom = false;
		if (!DialogCreateCylinder(radius, sides, length, cap_top, cap_bottom))
			return false;
		CreatePrimitiveCylinder(radius, length, max(3, sides));
		return true;
	}
	if (sid == "create_cone") {
		double radius = 5.0;
		int sides = 8;
		double length = 10.0;
		bool cap_bottom = false;
		if (!DialogCreateCone(radius, sides, length, cap_bottom))
			return false;
		CreatePrimitiveCone(radius, length, max(3, sides));
		return true;
	}
	if (sid == "create_plane") {
		int tiles = 4;
		double width = 30.0;
		double length = 30.0;
		int repeat = 1;
		if (!DialogCreatePlane(tiles, width, length, repeat))
			return false;
		CreatePrimitivePlane(max(1, tiles), width, length, max(1, repeat));
		return true;
	}
	if (sid == "create_terrain") {
		double side = 1500.0;
		double tile = 20.0;
		double max_h = 100.0;
		String topo;
		bool create_trees = true;
		bool create_grass = true;
		if (!DialogCreateTerrain(side, tile, max_h, topo, create_trees, create_grass))
			return false;
		int tiles = (tile > 1e-6) ? max(1, (int)floor(side / tile + 0.5)) : 1;
		GeomObject* obj = CreatePrimitivePlane(tiles, side, side, 1);
		if (obj) {
			obj->ui_type = "terrain";
			SetDynamicProp(*obj, "terrain_max_height", max_h);
			SetDynamicProp(*obj, "terrain_topology", topo);
			SetDynamicProp(*obj, "terrain_create_trees", create_trees);
			SetDynamicProp(*obj, "terrain_create_grass", create_grass);
		}
		return true;
	}
	if (sid == "create_room_mesh_from_2d_map") {
		String ceiling_type;
		double wall_height = 1.5;
		String ceiling_tex;
		if (!DialogCreateRoomMesh(ceiling_type, wall_height, ceiling_tex))
			return false;
		GeomObject& obj = CreateTaggedModel(*this, "room", "room_mesh");
		SetDynamicProp(obj, "room_mesh_pending", true);
		SetDynamicProp(obj, "room_ceiling_type", ceiling_type);
		SetDynamicProp(obj, "room_wall_height", wall_height);
		if (!ceiling_tex.IsEmpty())
			SetDynamicProp(obj, "room_ceiling_texture", ceiling_tex);
		state->UpdateObjects();
		RefreshData();
		return true;
	}
	if (sid == "create_water_surface") {
		int tiles = 10;
		double width = 1000.0;
		double length = 1000.0;
		if (!DialogCreateWater(tiles, width, length))
			return false;
		GeomObject* obj = CreatePrimitivePlane(max(1, tiles), width, length, 1);
		if (obj) {
			obj->ui_type = "water_surface";
			SetDynamicProp(*obj, "water_surface", true);
		}
		return true;
	}
	if (sid == "import_static_mesh") {
		FileSel fs;
		fs.Type("Meshes", "*.obj *.glb *.gltf *.fbx *.dae");
		if (!fs.ExecuteOpen("Please select a mesh to load"))
			return false;
		AddAssetFromPath(fs.Get());
		return true;
	}
	if (sid == "import_animated_mesh") {
		FileSel fs;
		fs.Type("Meshes", "*.obj *.glb *.gltf *.fbx *.dae");
		if (!fs.ExecuteOpen("Open mesh file"))
			return false;
		AddAssetFromPath(fs.Get());
		return true;
	}
	if (sid == "create_point_light") {
		GeomObject& obj = CreateTaggedModel(*this, "point_light", "light");
		obj.ui_type = "light.point";
		state->UpdateObjects();
		RefreshData();
		return true;
	}
	if (sid == "create_directional_light") {
		GeomScene& scene = state->GetActiveScene();
		for (GeomObject& existing : GeomObjectCollection(scene)) {
			if (existing.ui_type == "light.directional") {
				LOG("Directional light already exists in this scene.");
				return true;
			}
		}
		GeomObject& obj = CreateTaggedModel(*this, "directional_light", "light");
		obj.ui_type = "light.directional";
		state->UpdateObjects();
		RefreshData();
		return true;
	}
	if (sid == "create_tree") {
		if (!DialogCreateTree())
			return false;
		GeomObject& obj = CreateTaggedModel(*this, "tree", "tree");
		state->UpdateObjects();
		RefreshData();
		return true;
	}
	if (sid == "create_billboard") {
		GeomObject& obj = CreateTaggedModel(*this, "billboard", "billboard");
		state->UpdateObjects();
		RefreshData();
		return true;
	}
	if (sid == "create_vertical_billboard") {
		GeomObject& obj = CreateTaggedModel(*this, "billboard_v", "billboard_vertical");
		state->UpdateObjects();
		RefreshData();
		return true;
	}
	if (sid == "create_particle_system") {
		GeomObject& obj = CreateTaggedModel(*this, "particles", "particle_system");
		state->UpdateObjects();
		RefreshData();
		return true;
	}
	if (sid == "create_skybox") {
		GeomObject& obj = CreateTaggedModel(*this, "skybox", "skybox");
		state->UpdateObjects();
		RefreshData();
		return true;
	}
	if (sid == "create_3d_sound") {
		GeomObject& obj = CreateTaggedModel(*this, "sound3d", "sound3d");
		state->UpdateObjects();
		RefreshData();
		return true;
	}
	if (sid == "create_2d_overlay_item") {
		GeomObject& obj = CreateTaggedModel(*this, "overlay2d", "overlay2d");
		state->UpdateObjects();
		RefreshData();
		return true;
	}
	if (sid == "create_2d_touchscreen_input_item") {
		GeomObject& obj = CreateTaggedModel(*this, "touch2d", "touch2d");
		state->UpdateObjects();
		RefreshData();
		return true;
	}
	if (sid == "create_path") {
		GeomObject& obj = CreateTaggedModel(*this, "path", "path");
		state->UpdateObjects();
		RefreshData();
		return true;
	}
	if (sid == "create_path_node") {
		if (!v0.selected_obj) {
			LOG("Create path node requires selecting a path object first.");
			return false;
		}
		bool is_path = v0.selected_obj->ui_type == "path";
		if (!is_path) {
			LOG("Create path node is only available when a path object is selected.");
			return false;
		}
		GeomObject& obj = CreateTaggedModel(*this, "path_node", "path_node");
		SetDynamicProp(obj, "path_owner", v0.selected_obj->name);
		state->UpdateObjects();
		RefreshData();
		return true;
	}
	if (sid == "poly_select") {
		mesh_select_mode = MESHSEL_FACE;
		SetEditTool(TOOL_MESH_SELECT);
		tool_panel.Sync();
		RefrehRenderers();
		return true;
	}
	if (sid == "poly_select_by_rectangle") {
		mesh_select_mode = MESHSEL_FACE;
		SetEditTool(TOOL_MESH_SELECT);
		tool_panel.Sync();
		RefrehRenderers();
		return true;
	}
	if (sid == "poly_move_selected") {
		SetEditTool(TOOL_MESH_SELECT);
		tool_panel.Sync();
		RefrehRenderers();
		return true;
	}
	if (sid == "poly_rotate_selected") {
		SetEditTool(TOOL_MESH_SELECT);
		tool_panel.Sync();
		GeomObject* obj = GetFocusedMeshObject();
		if (obj && (!mesh_sel_points.IsEmpty() || !mesh_sel_lines.IsEmpty() || !mesh_sel_faces.IsEmpty())) {
			vec3 axis = vec3(0, 0, 1);
			if (state) {
				if (const GeomObjectState* os = state->FindObjectStateByKey(obj->key))
					axis = VectorTransform(vec3(0, 0, 1), os->orientation.GetInverse());
			}
			else if (GeomTransform* tr = obj->FindTransform()) {
				axis = VectorTransform(vec3(0, 0, 1), tr->orientation.GetInverse());
			}
			ApplyMeshSelectionRotate((float)(M_PI / 12.0), axis);
		}
		RefrehRenderers();
		return true;
	}
	if (sid == "poly_scale_selected") {
		SetEditTool(TOOL_MESH_SELECT);
		tool_panel.Sync();
		if (!mesh_sel_points.IsEmpty() || !mesh_sel_lines.IsEmpty() || !mesh_sel_faces.IsEmpty())
			ApplyMeshSelectionScale(1.1f);
		RefrehRenderers();
		return true;
	}
	if (sid == "poly_modify_uv") {
		mesh_select_mode = MESHSEL_VERTEX;
		SetEditTool(TOOL_MESH_SELECT);
		tool_panel.Sync();
		PromptOK("UV edit mode: select vertices and use mesh tools.");
		return true;
	}
	if (sid == "poly_edit_mode") {
		Ctrl* c = ribbon.FindControl("poly_edit_mode");
		if (c) {
			DropList* dl = dynamic_cast<DropList*>(c);
			if (dl && !IsNull(dl->GetData())) {
				String mode = ToLower(AsString(dl->GetData()));
				if (mode == "triangle")
					mesh_select_mode = MESHSEL_FACE;
				else if (mode == "point")
					mesh_select_mode = MESHSEL_VERTEX;
				tool_panel.Sync();
				RefrehRenderers();
				return true;
			}
		}
		return true;
	}
	if (sid == "poly_tools") {
		MenuBar::Execute([this](Bar& bar) {
			bar.Add(t_("Loop Select"), THISBACK(SelectMeshLoop));
			bar.Add(t_("Ring Select"), THISBACK(SelectMeshRing));
			bar.Add(t_("Expand Selection"), THISBACK(ExpandMeshSelection));
			bar.Add(t_("Contract Selection"), THISBACK(ContractMeshSelection));
			bar.Separator();
			bar.Add(t_("Extrude"), [this] { ExtrudeMeshSelection(0.1); });
			bar.Add(t_("Inset"), [this] { InsetMeshSelection(0.1); });
			bar.Add(t_("Spin"), THISBACK(SpinMeshSelection));
			bar.Add(t_("Screw"), THISBACK(ScrewMeshSelection));
		});
		return true;
	}
	if (sid == "camera_make_active") {
		if (!state || !v0.selected_obj || !v0.selected_obj->IsCamera())
			return false;
		for (int i = 0; i < state->objs.GetCount(); i++) {
			if (state->objs[i].obj && state->objs[i].obj->key == v0.selected_obj->key) {
				state->active_camera_obj_i = i;
				break;
			}
		}
		RefrehRenderers();
		return true;
	}
	if (sid == "focus_selected") {
		FocusSelectedNode();
		return true;
	}
	if (sid == "scene_add") {
		if (!prj || !state)
			return false;
		GeomScene& scene = prj->AddScene();
		int idx = prj->GetSceneCount() - 1;
		scene.name = Format("Scene #%d", idx);
		state->active_scene = idx;
		Data();
		v0.TimelineData();
		RefreshData();
		return true;
	}
	if (sid == "scene_delete") {
		if (!prj || !state)
			return false;
		int scene_count = prj->GetSceneCount();
		if (scene_count <= 1) {
			PromptOK("At least one scene must remain.");
			return true;
		}
		int active = state->active_scene;
		if (active < 0 || active >= scene_count)
			active = 0;
		GeomScene& scene = prj->GetScene(active);
		String scene_name = scene.name.IsEmpty() ? Format("Scene #%d", active) : scene.name;
		if (!PromptYesNo("Delete scene '" + scene_name + "'?"))
			return true;
		PushUndo("Delete scene");
		prj->val.sub.Remove(active);
		state->active_scene = min(active, max(prj->GetSceneCount() - 1, 0));
		state->UpdateObjects();
		Data();
		v0.TimelineData();
		RefreshData();
		return true;
	}
	if (sid == "scene_settings") {
		if (!state || !state->HasActiveScene())
			return false;
		GeomScene& scene = state->GetActiveScene();
		String name = scene.name.IsEmpty() ? scene.val.id : scene.name;
		EditText(name, "Scene Settings", "Scene name");
		name = TrimBoth(name);
		if (!name.IsEmpty())
			scene.name = name;
		String length_txt = IntStr(max(scene.length, 1));
		if (EditText(length_txt, "Scene Settings", "Length (frames)")) {
			int len = ScanInt(~length_txt);
			if (len > 0)
				scene.length = len;
		}
		Data();
		v0.TimelineData();
		RefreshData();
		return true;
	}
	if (sid == "scene_metrics") {
		if (!state || !state->HasActiveScene() || !prj)
			return false;
		GeomScene& scene = state->GetActiveScene();
		int object_count = 0;
		for (GeomObject& o : GeomObjectCollection(scene))
			object_count++;
		String info;
		info << "Scene: " << (scene.name.IsEmpty() ? scene.val.id : scene.name) << "\n";
		info << "Objects: " << object_count << "\n";
		info << "Length: " << scene.length << " frames\n";
		info << "FPS: " << prj->fps << "\n";
		info << "KPS: " << prj->kps;
		PromptOK(info);
		return true;
	}
	if (sid == "scene_post_effects") {
		if (!state || !state->HasActiveScene())
			return false;
		GeomScene& scene = state->GetActiveScene();
		bool enabled = false;
		if (GeomDynamicProperties* props = scene.FindDynamicProperties()) {
			int i = props->props.Find("postfx_enabled");
			if (i >= 0)
				enabled = (bool)props->props[i];
		}
		enabled = !enabled;
		SetDynamicProp(scene, "postfx_enabled", enabled);
		PromptOK(enabled ? "Scene post effects enabled." : "Scene post effects disabled.");
		return true;
	}
	if (sid == "scene_selector") {
		Ctrl* c = ribbon.FindControl("scene_selector");
		if (c) {
			DropList* dl = dynamic_cast<DropList*>(c);
			if (dl && !IsNull(dl->GetData())) {
				String sel = ToLower(AsString(dl->GetData()));
				if (sel.Find("new") >= 0) {
					HandleRibbonAction("scene_add");
					return true;
				}
				if (sel.StartsWith("scene_")) {
					int idx = ScanInt(sel.Mid(6));
					if (state && prj && idx >= 0 && idx < prj->GetSceneCount()) {
						state->active_scene = idx;
						state->UpdateObjects();
						Data();
						v0.TimelineData();
						RefreshData();
					}
					return true;
				}
			}
		}
		return true;
	}
	if (sid == "lightmap_mode" ||
	    sid == "lightmap_texture_size" ||
	    sid == "lightmap_subsampling" ||
	    sid == "lightmap_resolution" ||
	    sid == "lightmap_shadow_opacity" ||
	    sid == "ambient_light_enabled" ||
	    sid == "smooth_normals_enabled" ||
	    sid == "color_bleeding_enabled") {
		StoreRibbonLightmapSettings(false);
		return true;
	}
	if (sid == "calculate_lightmap") {
		StoreRibbonLightmapSettings(true);
		return true;
	}
	if (sid == "ambient_light_more") {
		LOG("Ambient light settings not implemented yet.");
		return true;
	}
	if (sid == "publish_target") {
		if (!state || !state->HasActiveScene())
			return false;
		String target;
		if (GetRibbonDropValue(ribbon, "publish_target", target))
			SetDynamicProp(state->GetActiveScene(), "publish_target", target);
		return true;
	}
	if (sid == "publishing_settings") {
		if (!state || !state->HasActiveScene())
			return false;
		GeomScene& scene = state->GetActiveScene();
		String out_dir;
		if (GeomDynamicProperties* props = scene.FindDynamicProperties()) {
			int i = props->props.Find("publish_output_dir");
			if (i >= 0)
				out_dir = AsString(props->props[i]);
		}
		if (out_dir.IsEmpty())
			out_dir = project_dir.IsEmpty() ? GetCurrentDirectory() : AppendFileName(project_dir, "exports");
		if (EditText(out_dir, "Publishing Settings", "Export directory"))
			SetDynamicProp(scene, "publish_output_dir", out_dir);
		return true;
	}
	if (sid == "publish_and_test") {
		ExportExecutionProject(true);
		return true;
	}
	if (sid.StartsWith("create_")) {
		CreateEditableMeshObject();
		return true;
	}
	LOG("Ribbon action not implemented: " << id);
	return false;
}

GeomScene& Edit3D::GetActiveScene() {
	return state->GetActiveScene();
}

void Edit3D::Serialize(Stream& s) {
	SerializeLayout(s);
	s % v0.props_cursor_by_tree;
	s % v0.tree_open_paths;
	s % recent_assets;
	s % show_hud;
	s % show_hud_help;
	s % show_hud_status;
	s % show_hud_debug;
	if (s.IsStoring()) {
		ribbon_display_mode = ribbon.GetDisplayMode();
		ribbon_qat_pos = ribbon.GetQuickAccessPos();
	}
	s % ribbon_display_mode;
	s % ribbon_qat_pos;
}

String Edit3D::SerializeScene3DState() const {
	Scene3DDocument doc;
	doc.version = SCENE3D_VERSION;
	doc.name = "ModelerApp";
	doc.project = prj;
	doc.active_scene = state ? state->active_scene : 0;
	doc.focus = state ? &state->GetFocus() : nullptr;
	doc.program = state ? &state->GetProgram() : nullptr;
	doc.created_utc = scene3d_created;
	doc.modified_utc = scene3d_modified;
	doc.data_dir = scene3d_data_dir.IsEmpty() ? String("data") : scene3d_data_dir;
	doc.external_files.Clear();
	for (const Scene3DExternalFile& file : scene3d_external_files)
		doc.external_files.Add(file);
	doc.meta.Clear();
	for (const Scene3DMetaEntry& entry : scene3d_meta)
		doc.meta.Add(entry);
	String json;
	DoVisitToJson(doc, json, false);
	return json;
}

bool Edit3D::RestoreScene3DState(const String& json) {
	Scene3DDocument doc;
	if (json.IsEmpty())
		return false;
	if (!VisitFromJson(doc, json.Begin()))
		return false;
	doc.project_val.FixParent();
	FixupScene3DOwners(*doc.project);
	PruneEmptyObjects(*doc.project);
	VisitCopy(*doc.project, *prj);
	state->prj = prj;
	state->active_scene = doc.active_scene;
	if (doc.focus)
		VisitCopy(*doc.focus, state->GetFocus());
	if (doc.program)
		VisitCopy(*doc.program, state->GetProgram());
	scene3d_created = doc.created_utc;
	scene3d_modified = doc.modified_utc;
	scene3d_data_dir = doc.data_dir;
	scene3d_external_files = pick(doc.external_files);
	scene3d_meta = pick(doc.meta);
	if (scene3d_data_dir.IsEmpty())
		scene3d_data_dir = "data";
	state->UpdateObjects();
	anim->Reset();
	Data();
	v0.TimelineData();
	v0.tree.OpenDeep(v0.tree_scenes);
	UpdateWindowTitle();
	return true;
}

void Edit3D::PushUndo(const String& note) {
	if (undo_guard)
		return;
	String json = SerializeScene3DState();
	if (json.IsEmpty())
		return;
	if (undo_stack.GetCount() > 0 && undo_stack[undo_stack.GetCount() - 1].json == json)
		return;
	UndoEntry& ent = undo_stack.Add();
	ent.json = json;
	ent.note = note;
	redo_stack.Clear();
	if (undo_stack.GetCount() > undo_limit) {
		undo_stack.Remove(0);
	}
}

void Edit3D::Undo() {
	if (undo_stack.GetCount() == 0)
		return;
	undo_guard = true;
	UndoEntry cur;
	cur.json = SerializeScene3DState();
	redo_stack.Add(pick(cur));
	UndoEntry ent = pick(undo_stack[undo_stack.GetCount() - 1]);
	undo_stack.Remove(undo_stack.GetCount() - 1);
	RestoreScene3DState(ent.json);
	undo_guard = false;
}

void Edit3D::Redo() {
	if (redo_stack.GetCount() == 0)
		return;
	undo_guard = true;
	UndoEntry cur;
	cur.json = SerializeScene3DState();
	undo_stack.Add(pick(cur));
	UndoEntry ent = pick(redo_stack[redo_stack.GetCount() - 1]);
	redo_stack.Remove(redo_stack.GetCount() - 1);
	RestoreScene3DState(ent.json);
	undo_guard = false;
}

void Edit3D::Exit() {
	v0.StorePropsCursor(v0.current_tree_path);
	v0.StoreTreeOpenState();
	StoreToFile(*this, ModelerAppConfigPath());
	TopWindow::Close();
}

void Edit3D::RefreshData() {
	PostCallback(THISBACK(Data));
}

void Edit3D::Stop() {
	anim->Reset();
	RefrehToolbar();
}

void Edit3D::Pause() {
	anim->Pause();
	RefrehToolbar();
}

void Edit3D::Play() {
	anim->Play();
	RefrehToolbar();
}

void Edit3D::ResetLayout() {
	String cfg = ModelerAppConfigPath();
	if (FileExists(cfg))
		DeleteFile(cfg);
	int ix = GetLayouts().Find("Default");
	if (ix >= 0)
		LoadLayout("Default");
	v0.props_cursor_by_tree.Clear();
	v0.tree_open_paths.Clear();
	v0.current_tree_path.Clear();
}

void Edit3D::ResetPropsCursor() {
	v0.props_cursor_by_tree.Clear();
	v0.current_tree_path.Clear();
}

void Edit3D::OpenAllDocksForTest() {
	auto ensure_visible = [this](DockableCtrl* dc) {
		if (!dc)
			return;
		RestoreDockerPos(*dc, true);
		if (dc->IsHidden())
			HideRestoreDocker(*dc);
		ActivateDockableChild(*dc);
	};
	ensure_visible(dock_tools);
	ensure_visible(dock_tree);
	ensure_visible(dock_props);
	ensure_visible(dock_time);
	ensure_visible(dock_video);
	ensure_visible(dock_pool);
	ensure_visible(dock_assets);
	ensure_visible(dock_texture);
	ensure_visible(dock_script);
}

void Edit3D::OnSceneEnd() {
	if (repeat_playback) {
		anim->Reset();
		anim->Play();
	}
	RefrehToolbar();
}

void Edit3D::ScheduleBuiltinDump(int builtin_index, int delay_ms) {
	debug_tc.Set(delay_ms, THISBACK1(DumpBuiltinState, builtin_index));
}

void Edit3D::DumpBuiltinState(int builtin_index) {
	Cout() << "Builtin dump index=" << builtin_index << "\n";
	Cout() << "timeline_scope=" << (int)timeline_scope
	       << " object_key=" << (int)timeline_object_key
	       << " component=" << (int)timeline_component
	       << " field=" << (int)timeline_transform_field << "\n";
	if (anim)
		Cout() << "anim pos=" << anim->position << " time=" << anim->time
		       << " playing=" << (int)anim->is_playing << "\n";
	if (state && state->HasActiveScene()) {
		GeomScene& scene = state->GetActiveScene();
		GeomSceneTimeline& tl = scene.GetTimeline();
		Cout() << "scene length=" << scene.length
		       << " tl.pos=" << tl.position
		       << " tl.time=" << tl.time
		       << " tl.play=" << (int)tl.is_playing << "\n";
		Cout() << "objects=" << state->objs.GetCount() << "\n";
	}
}

void Edit3D::OpenExecutionWindow() {
	if (exec_win) {
		exec_win->SetFocus();
		return;
	}
	exec_exit_requested = false;
	exec_win = MakeOne<ExecutionWindow>();
	exec_win->Init(this);
	exec_win->Open();
	Play();
}

void Edit3D::RequestExecutionExit() {
	exec_exit_requested = true;
	Pause();
}

void Edit3D::RegisterExternalRenderer(EditRendererBase* renderer) {
	if (!renderer)
		return;
	for (int i = 0; i < external_renderers.GetCount(); i++) {
		if (external_renderers[i] == renderer)
			return;
	}
	external_renderers.Add(renderer);
}

void Edit3D::UnregisterExternalRenderer(EditRendererBase* renderer) {
	for (int i = 0; i < external_renderers.GetCount(); i++) {
		if (external_renderers[i] == renderer) {
			external_renderers.Remove(i);
			return;
		}
	}
}

void Edit3D::RefrehRenderers() {
	if (view == VIEW_GEOMPROJECT) {
		v0.RefreshAll();
	}
	else if (view == VIEW_VIDEOIMPORT) {
		v1.RefreshRenderers();
	}
	for (int i = 0; i < external_renderers.GetCount(); i++) {
		EditRendererBase* rend = external_renderers[i];
		if (rend)
			rend->Refresh();
	}
}

void Edit3D::UpdateHud() {
	render_ctx.show_hud = show_hud;
	render_ctx.show_hud_help = show_hud_help;
	render_ctx.hud_lines.Clear();
	render_ctx.hud_help.Clear();
	render_ctx.selected_mesh_points.Clear();
	render_ctx.selected_mesh_lines.Clear();
	render_ctx.selected_mesh_faces.Clear();
	render_ctx.selected_2d_shapes.Clear();
	render_ctx.selection_center_valid = false;
	render_ctx.selection_gizmo_enabled = (edit_tool == TOOL_OBJ_SELECT || edit_tool == TOOL_OBJ_MOVE ||
	                                      edit_tool == TOOL_OBJ_ROTATE || edit_tool == TOOL_MESH_SELECT);
	render_ctx.selection_kind = 0;
	if (!mesh_sel_points.IsEmpty())
		render_ctx.selected_mesh_points.Append(mesh_sel_points);
	if (!mesh_sel_lines.IsEmpty())
		render_ctx.selected_mesh_lines.Append(mesh_sel_lines);
	if (!mesh_sel_faces.IsEmpty())
		render_ctx.selected_mesh_faces.Append(mesh_sel_faces);
	if (!select_2d_shapes.IsEmpty())
		render_ctx.selected_2d_shapes.Append(select_2d_shapes);
	auto apply_world = [&](GeomObject* obj, const vec3& local) -> bool {
		if (!obj)
			return false;
		vec3 pos = vec3(0);
		quat ori = Identity<quat>();
		vec3 scale = vec3(1);
		if (state) {
			if (const GeomObjectState* os = state->FindObjectStateByKey(obj->key)) {
				pos = os->position;
				ori = os->orientation;
				scale = os->scale;
			}
		}
		else if (GeomTransform* tr = obj->FindTransform()) {
			pos = tr->position;
			ori = tr->orientation;
			scale = tr->scale;
		}
		vec3 scaled(local[0] * scale[0], local[1] * scale[1], local[2] * scale[2]);
		render_ctx.selection_center_world = pos + VectorTransform(scaled, ori);
		render_ctx.selection_center_valid = true;
		return true;
	};
	vec3 local_center(0);
	if ((!mesh_sel_points.IsEmpty() || !mesh_sel_lines.IsEmpty() || !mesh_sel_faces.IsEmpty()) && GetMeshSelectionCenter(local_center)) {
		render_ctx.selection_kind = 1;
		apply_world(GetFocusedMeshObject(), local_center);
	}
	else if (!select_2d_shapes.IsEmpty() && Get2DSelectionCenter(local_center)) {
		render_ctx.selection_kind = 2;
		apply_world(GetFocused2DObject(), local_center);
	}
	if (!render_ctx.selection_center_valid && (edit_tool == TOOL_OBJ_MOVE || edit_tool == TOOL_OBJ_ROTATE || edit_tool == TOOL_OBJ_SELECT)) {
		vec3 pos = vec3(0);
		quat ori = Identity<quat>();
		if (GeomObject* obj = v0.selected_obj) {
			if (GeomTransform* tr = obj->FindTransform()) {
				pos = tr->position;
				ori = tr->orientation;
			}
			else if (state) {
				if (const GeomObjectState* os = state->FindObjectStateByKey(obj->key)) {
					pos = os->position;
					ori = os->orientation;
				}
			}
			render_ctx.selection_center_world = pos;
			render_ctx.selection_center_valid = true;
			render_ctx.selection_kind = 3;
		}
		else if (v0.selected_ref && state) {
			if (v0.selected_ref->kind == GeomProjectCtrl::TreeNodeRef::K_PROGRAM) {
				GeomCamera& cam = state->GetProgram();
				render_ctx.selection_center_world = cam.position;
				render_ctx.selection_center_valid = true;
				render_ctx.selection_kind = 3;
			}
			if (v0.selected_ref->kind == GeomProjectCtrl::TreeNodeRef::K_FOCUS) {
				GeomCamera& cam = state->GetFocus();
				render_ctx.selection_center_world = cam.position;
				render_ctx.selection_center_valid = true;
				render_ctx.selection_kind = 3;
			}
		}
	}
	if (!show_hud)
		return;
	if (show_hud_status) {
		EditRendererBase* rend = nullptr;
		if (active_view >= 0 && active_view < 4)
			rend = v0.rends[active_view];
		String view_mode = "N/A";
		String cam_src = "N/A";
		if (rend) {
			switch (rend->view_mode) {
			case VIEWMODE_YZ: view_mode = "YZ"; break;
			case VIEWMODE_XZ: view_mode = "XZ"; break;
			case VIEWMODE_XY: view_mode = "XY"; break;
			case VIEWMODE_PERSPECTIVE: view_mode = "Perspective"; break;
			default: break;
			}
			switch (rend->cam_src) {
			case CAMSRC_FOCUS: cam_src = "Focus"; break;
			case CAMSRC_PROGRAM: cam_src = "Program"; break;
			case CAMSRC_OBJECT: cam_src = "Selected"; break;
			default: break;
			}
		}
		render_ctx.hud_lines.Add(Format("View %d: %s | Cam: %s", active_view + 1, view_mode, cam_src));
	}
	if (show_hud_debug) {
		render_ctx.hud_lines.Add(Format("Gizmo: %s | Selection kind: %d",
			render_ctx.selection_gizmo_enabled ? "On" : "Off", render_ctx.selection_kind));
	}
	auto tool_name = [&](EditTool t) -> String {
		switch (t) {
		case TOOL_VIEW: return "View";
		case TOOL_OBJ_SELECT: return "Object Select";
		case TOOL_OBJ_MOVE: return "Object Move";
		case TOOL_OBJ_ROTATE: return "Object Rotate";
		case TOOL_MESH_SELECT: return "Mesh Select";
		case TOOL_POINT: return "Point";
		case TOOL_LINE: return "Line";
		case TOOL_FACE: return "Face";
		case TOOL_ERASE: return "Erase";
		case TOOL_JOIN: return "Join";
		case TOOL_SPLIT: return "Split";
		case TOOL_2D_SELECT: return "2D Select";
		case TOOL_2D_LINE: return "2D Line";
		case TOOL_2D_RECT: return "2D Rect";
		case TOOL_2D_CIRCLE: return "2D Circle";
		case TOOL_2D_POLY: return "2D Poly";
		case TOOL_2D_ERASE: return "2D Erase";
		default: return "Unknown";
		}
	};
	auto mesh_mode = [&]() -> String {
		switch (mesh_select_mode) {
		case MESHSEL_VERTEX: return "Vertex";
		case MESHSEL_EDGE: return "Edge";
		case MESHSEL_FACE: return "Face";
		default: return "Vertex";
		}
	};
	String snap = edit_snap_enable ? Format("Snap %.2f%s", edit_snap_step, edit_snap_local ? " local" : "") : "Snap off";
	render_ctx.hud_lines.Add("Tool: " + tool_name(edit_tool));
	render_ctx.hud_lines.Add("Mesh Select: " + mesh_mode());
	render_ctx.hud_lines.Add(String("Plane: ") + (edit_plane == PLANE_VIEW ? "View" :
		edit_plane == PLANE_XY ? "XY" : edit_plane == PLANE_XZ ? "XZ" : edit_plane == PLANE_YZ ? "YZ" : "Local"));
	render_ctx.hud_lines.Add(snap);
	render_ctx.hud_lines.Add(String() + "Auto-key: " + (auto_key ? "On" : "Off"));
	String hint;
	switch (edit_tool) {
	case TOOL_VIEW: hint = "Drag to move camera (view tool)"; break;
	case TOOL_OBJ_SELECT: hint = "Select object in tree, use gizmo to move/rotate"; break;
	case TOOL_OBJ_MOVE: hint = "Drag gizmo to move object"; break;
	case TOOL_OBJ_ROTATE: hint = "Drag gizmo to rotate object"; break;
	case TOOL_MESH_SELECT: hint = "Click to select mesh/skeleton"; break;
	case TOOL_POINT: hint = "Click to add vertex"; break;
	case TOOL_LINE: hint = "Click to add edge"; break;
	case TOOL_FACE: hint = "Click 3 points to add face"; break;
	case TOOL_ERASE: hint = "Click point to remove"; break;
	case TOOL_JOIN: hint = "Pick two points to connect"; break;
	case TOOL_SPLIT: hint = "Click edge to split"; break;
	case TOOL_2D_SELECT: hint = "Click shape to select"; break;
	case TOOL_2D_LINE: hint = "Drag to draw line"; break;
	case TOOL_2D_RECT: hint = "Drag to draw rect"; break;
	case TOOL_2D_CIRCLE: hint = "Drag to draw circle"; break;
	case TOOL_2D_POLY: hint = "Click to add vertices"; break;
	case TOOL_2D_ERASE: hint = "Click to delete shape"; break;
	default: break;
	}
	if (!hint.IsEmpty())
		render_ctx.hud_lines.Add("Hint: " + hint);
	if (show_hud_help) {
		render_ctx.hud_help.Add("F1: Toggle help");
		render_ctx.hud_help.Add("Ctrl+H: Toggle HUD");
		render_ctx.hud_help.Add("Alt+Arrow: Nudge move");
		render_ctx.hud_help.Add("Alt+Ctrl+Arrow: Nudge rotate");
		render_ctx.hud_help.Add("Shift+Click: Add to selection");
		render_ctx.hud_help.Add("Ctrl+Click: Toggle selection");
	}
}

void Edit3D::Update() {
	double dt = ts.Seconds();
	ts.Reset();
	UpdateHud();
	
	v0.Update(dt);
	if (dock_video && !dock_video->IsHidden())
		v1.Update(dt);
	if (conf.dump_grid_done && !conf.dump_grid_path.IsEmpty()) {
		conf.dump_grid_path.Clear();
		PostCallback(THISBACK(Exit));
	}
	
	if (hmd.IsRunning()) {
		hmd.Poll();
		if (record_pointcloud)
			UpdateHmdCameraPose();
	}

	if (state && state->HasActiveScene()) {
		GeomScene& scene = state->GetActiveScene();
		GeomSceneTimeline& tl = scene.GetTimeline();
		bool was_playing = tl.is_playing;
		if (tl.is_playing)
			tl.Update(*state, dt);
		if (tl.is_playing || was_playing)
			RefrehRenderers();
		if (view == VIEW_GEOMPROJECT && (tl.is_playing || was_playing))
			v0.SyncPropsValues();
	}

	EnsureScriptInstances();
	for (auto& inst : script_instances)
		UpdateScriptInstance(inst, false);
	for (auto& inst : script_instances)
		RunScriptFrame(inst, dt);
	DispatchFrameEvents(dt);
	input_state.BeginFrame();
}

void Edit3D::Data() {
	v0.Data();
	if (dock_video && !dock_video->IsHidden())
		v1.Data();
	if (dock_assets && !dock_assets->IsHidden())
		asset_browser.Data();
	SyncRibbonLightmapControls();
	SyncRibbonSceneSelector(*this);
	SyncRibbonPublishTarget(*this);
}

void Edit3D::SetProjectDir(String dir) {
	if (dir.IsEmpty())
		dir = GetCurrentDirectory();
	project_dir = NormalizePath(dir);
	EnsureScriptInstances();
	UpdateAssetBrowser();
}

String Edit3D::GetScriptAbsPath(const String& rel) const {
	if (IsFullPath(rel))
		return NormalizePath(rel);
	if (project_dir.IsEmpty())
		return NormalizePath(AppendFileName(GetCurrentDirectory(), rel));
	return NormalizePath(AppendFileName(project_dir, rel));
}

String Edit3D::EnsureScriptFile(GeomScript& script, String base_name) {
	if (base_name.IsEmpty())
		base_name = "script";
	String safe = ToVarName(base_name, '_');
	if (safe.IsEmpty())
		safe = "script";
	if (project_dir.IsEmpty())
		SetProjectDir(GetCurrentDirectory());
	String rel = script.file;
	if (rel.IsEmpty()) {
		String dir = AppendFileName(project_dir, "scripts");
		RealizeDirectory(dir);
		String name = safe;
		String rel_try = AppendFileName("scripts", name + ".py");
		String abs_try = AppendFileName(project_dir, rel_try);
		int idx = 1;
		while (FileExists(abs_try)) {
			rel_try = AppendFileName("scripts", name + "_" + IntStr(idx++) + ".py");
			abs_try = AppendFileName(project_dir, rel_try);
		}
		rel = rel_try;
		script.file = rel;
	}
	String abs = GetScriptAbsPath(rel);
	if (!FileExists(abs)) {
		String header;
		header << "# Script: " << base_name << "\n";
		header << "# Example:\n";
		header << "# def on_start():\n";
		header << "#     modeler.debug_generate_pointcloud()\n";
		header << "#     modeler.debug_simulate_observation()\n";
		header << "#     modeler.debug_run_localization()\n";
		header << "#\n";
		header << "# def on_frame(dt):\n";
		header << "#     pass\n";
		SaveFile(abs, header);
	}
	return rel;
}

GeomScript& Edit3D::AddScriptComponent(GeomObject& obj) {
	String id = "script";
	int idx = 1;
	hash_t script_hash = TypedStringHasher<GeomScript>("GeomScript");
	while (obj.val.Find(id, script_hash) >= 0)
		id = "script_" + IntStr(idx++);
	VfsValue& node = obj.val.Add(id, script_hash);
	GeomScript& script = node.GetExt<GeomScript>();
	EnsureScriptFile(script, obj.name.IsEmpty() ? id : obj.name);
	EnsureScriptInstances();
	return script;
}

GeomScript& Edit3D::AddScriptComponent(GeomDirectory& dir) {
	String id = "script";
	int idx = 1;
	hash_t script_hash = TypedStringHasher<GeomScript>("GeomScript");
	while (dir.val.Find(id, script_hash) >= 0)
		id = "script_" + IntStr(idx++);
	VfsValue& node = dir.val.Add(id, script_hash);
	GeomScript& script = node.GetExt<GeomScript>();
	String base = dir.name.IsEmpty() ? id : dir.name;
	EnsureScriptFile(script, base);
	EnsureScriptInstances();
	return script;
}

GeomScript& Edit3D::AddScriptComponent(GeomScene& scene) {
	String id = "script";
	int idx = 1;
	hash_t script_hash = TypedStringHasher<GeomScript>("GeomScript");
	while (scene.val.Find(id, script_hash) >= 0)
		id = "script_" + IntStr(idx++);
	VfsValue& node = scene.val.Add(id, script_hash);
	GeomScript& script = node.GetExt<GeomScript>();
	String base = scene.name.IsEmpty() ? id : scene.name;
	EnsureScriptFile(script, base);
	EnsureScriptInstances();
	return script;
}

void Edit3D::GetScriptsFromNode(VfsValue& node, Vector<GeomScript*>& out) {
	hash_t script_hash = TypedStringHasher<GeomScript>("GeomScript");
	for (auto& sub : node.sub) {
		if (IsVfsType(sub, script_hash))
			out.Add(&sub.GetExt<GeomScript>());
	}
}

void Edit3D::OpenScriptEditor(GeomScript& script) {
	script_editor.OpenScript(script);
	if (dock_script)
		ActivateDockableChild(*dock_script);
}

void Edit3D::RunScriptOnce(GeomScript& script) {
	EnsureScriptInstances();
	for (auto& inst : script_instances) {
		if (inst.script == &script) {
			UpdateScriptInstance(inst, true);
			RunScriptOnLoad(inst, !script.run_on_load);
			RunScriptOnStart(inst, !script.run_on_load);
			return;
		}
	}
}

void Edit3D::RegisterScriptVM(PyVM& vm) {
	::RegisterGeometry(vm);
	{
		PY_MODULE(modeler, vm)
		PY_MODULE_FUNC(log, ModelerLog, this);
		PY_MODULE_FUNC(debug_generate_pointcloud, ModelerDebugGeneratePointcloud, this);
		PY_MODULE_FUNC(debug_simulate_observation, ModelerDebugSimulateObservation, this);
		PY_MODULE_FUNC(debug_run_localization, ModelerDebugRunLocalization, this);
		PY_MODULE_FUNC(debug_simulate_controller_observations, ModelerDebugSimulateControllerObservations, this);
		PY_MODULE_FUNC(debug_run_controller_localization, ModelerDebugRunControllerLocalization, this);
		PY_MODULE_FUNC(debug_clear_synthetic, ModelerDebugClearSynthetic, this);
		PY_MODULE_FUNC(debug_run_full_synthetic, ModelerDebugRunFullSynthetic, this);
		PY_MODULE_FUNC(set_position, ModelerSetPosition, this);
		PY_MODULE_FUNC(set_orientation, ModelerSetOrientation, this);
		PY_MODULE_FUNC(get_position, ModelerGetPosition, this);
		PY_MODULE_FUNC(add_transform_keyframe, ModelerAddTransformKeyframe, this);
		PY_MODULE_FUNC(set_timeline_expanded, ModelerSetTimelineExpanded, this);
		PY_MODULE_FUNC(get_gizmo_pixels, ModelerGetGizmoPixels, this);
		PY_MODULE_FUNC(set_renderer_version, ModelerSetRendererVersion, this);
		PY_MODULE_FUNC(get_renderer_version, ModelerGetRendererVersion, this);
		PY_MODULE_FUNC(set_renderer_wireframe, ModelerSetRendererWireframe, this);
		PY_MODULE_FUNC(get_renderer_wireframe, ModelerGetRendererWireframe, this);
		PY_MODULE_FUNC(set_show_grid, ModelerSetShowGrid, this);
		PY_MODULE_FUNC(get_show_grid, ModelerGetShowGrid, this);
		PY_MODULE_FUNC(set_hud_visible, ModelerSetHudVisible, this);
		PY_MODULE_FUNC(get_hud_visible, ModelerGetHudVisible, this);
		PY_MODULE_FUNC(create_object, ModelerCreateObject, this);
		PY_MODULE_FUNC(create_directory, ModelerCreateDirectory, this);
		PY_MODULE_FUNC(get_project_dir, ModelerGetProjectDir, this);
		PY_MODULE_FUNC(trace, ModelerTrace, this);
		PY_MODULE_FUNC(get_timer, ModelerGetTimer, this);
		PY_MODULE_FUNC(random, ModelerRandom, this);
	}
	{
		PY_MODULE(exec, vm)
		PY_MODULE_FUNC(exit, ModelerExecExit, this);
	}
	PyValue stage_obj = PyValue(new StageProxy(this, &vm));
	vm.GetGlobalsRW().GetDictRW().GetAdd(PyValue("stage")) = stage_obj;
	vm.GetGlobalsRW().GetDictRW().GetAdd(PyValue("trace")) = PyValue::Function("trace", ModelerTrace, this);
	vm.GetGlobalsRW().GetDictRW().GetAdd(PyValue("getTimer")) = PyValue::Function("getTimer", ModelerGetTimer, this);
	vm.GetGlobalsRW().GetDictRW().GetAdd(PyValue("random")) = PyValue::Function("random", ModelerRandom, this);
	vm.GetGlobalsRW().GetDictRW().GetAdd(PyValue("input")) = PyValue(new InputProxy(&input_state));
	PyValue root_obj = PyValue::None();
	if (state)
		root_obj = MakeDisplayObject(this, &state->GetActiveScene().val, &vm);
	vm.GetGlobalsRW().GetDictRW().GetAdd(PyValue("root")) = root_obj;
	vm.GetGlobalsRW().GetDictRW().GetAdd(PyValue("this")) = root_obj;
}

void Edit3D::EnsureScriptInstances() {
	if (!state || !state->prj || state->active_scene < 0 || state->active_scene >= state->prj->GetSceneCount())
		return;
	GeomScene& scene = GetActiveScene();
	Vector<GeomScript*> scripts;
	GetScriptsFromNode(scene.val, scripts);
	for (auto& sub : scene.val.sub) {
		hash_t dir_hash = TypedStringHasher<GeomDirectory>("GeomDirectory");
		if (IsVfsType(sub, dir_hash))
			GetScriptsFromNode(sub, scripts);
	}
	for (GeomObject& obj : GeomObjectCollection(scene))
		GetScriptsFromNode(obj.val, scripts);
	for (int i = script_instances.GetCount() - 1; i >= 0; i--) {
		bool found = false;
		for (int j = 0; j < scripts.GetCount(); j++) {
			if (scripts[j] == script_instances[i].script) {
				found = true;
				break;
			}
		}
		if (!found) {
			RemoveScriptEventHandlers(&script_instances[i].vm);
			script_instances.Remove(i);
		}
	}
	for (GeomScript* script : scripts) {
		bool found = false;
		for (auto& inst : script_instances) {
			if (inst.script == script) {
				found = true;
				break;
			}
		}
		if (!found) {
			ScriptInstance& inst = script_instances.Add();
			inst.script = script;
			inst.owner = script->val.owner;
			RegisterScriptVM(inst.vm);
		}
	}
}

void Edit3D::UpdateScriptInstance(ScriptInstance& inst, bool force_reload) {
	if (!inst.script)
		return;
	GeomScript& script = *inst.script;
	if (!script.enabled)
		return;
	EnsureScriptFile(script, "script");
	String abs = GetScriptAbsPath(script.file);
	Time mod = FileGetTime(abs);
	bool needs_reload = force_reload || !inst.loaded || mod != inst.file_time;
	if (!needs_reload)
		return;
	RemoveScriptEventHandlers(&inst.vm);
	inst.file_time = mod;
	inst.loaded = false;
	inst.has_load = false;
	inst.has_start = false;
	inst.has_frame = false;
	String code = LoadFile(abs);
	String err;
	Vector<PyIR> ir;
	if (!CompilePySource(code, abs, ir, err)) {
		LOG("Script compile failed: " + err);
		return;
	}
	inst.vm.Clear();
	RegisterScriptVM(inst.vm);
	if (state) {
		PyValue root_obj = MakeDisplayObject(this, &state->GetActiveScene().val, &inst.vm);
		inst.vm.GetGlobalsRW().GetDictRW().GetAdd(PyValue("root")) = root_obj;
		PyValue this_obj = root_obj;
		if (inst.owner) {
			PyValue owner_obj = MakeDisplayObject(this, inst.owner, &inst.vm);
			if (!owner_obj.IsNone())
				this_obj = owner_obj;
		}
		inst.vm.GetGlobalsRW().GetDictRW().GetAdd(PyValue("this")) = this_obj;
	}
	inst.vm.GetGlobalsRW().GetDictRW().GetAdd(PyValue("__project_dir__")) = PyValue(project_dir);
	inst.vm.GetGlobalsRW().GetDictRW().GetAdd(PyValue("__script_path__")) = PyValue(abs);
	if (!RunPyIR(inst.vm, ir, err)) {
		LOG("Script run failed: " + err);
		return;
	}
	inst.loaded = true;
	inst.main_ir = pick(ir);
	const VectorMap<PyValue, PyValue>& globals = inst.vm.GetGlobals().GetDict();
	int on_load_idx = globals.Find(PyValue("on_load"));
	int on_start_idx = globals.Find(PyValue("on_start"));
	int on_frame_idx = globals.Find(PyValue("on_frame"));
	inst.has_load = on_load_idx >= 0 && !globals[on_load_idx].IsNone();
	inst.has_start = on_start_idx >= 0 && !globals[on_start_idx].IsNone();
	inst.has_frame = on_frame_idx >= 0 && !globals[on_frame_idx].IsNone();
	if (inst.has_load) {
		Vector<PyIR> load_ir;
		if (CompilePySource("on_load()", abs, load_ir, err))
			inst.load_ir = pick(load_ir);
	}
	if (inst.has_start) {
		Vector<PyIR> start_ir;
		if (CompilePySource("on_start()", abs, start_ir, err))
			inst.start_ir = pick(start_ir);
	}
	if (inst.has_frame) {
		Vector<PyIR> frame_ir;
		if (CompilePySource("on_frame(__dt__)", abs, frame_ir, err))
			inst.frame_ir = pick(frame_ir);
	}
	RunScriptOnLoad(inst, false);
	RunScriptOnStart(inst, false);
}

void Edit3D::RunScriptOnLoad(ScriptInstance& inst, bool force) {
	if (!inst.script || !inst.loaded)
		return;
	if (!force && !inst.script->run_on_load)
		return;
	if (!inst.has_load)
		return;
	String err;
	if (!RunPyIR(inst.vm, inst.load_ir, err))
		LOG("Script on_load failed: " + err);
}

void Edit3D::RunScriptOnStart(ScriptInstance& inst, bool force) {
	if (!inst.script || !inst.loaded)
		return;
	if (!force && !inst.script->run_on_load)
		return;
	if (!inst.has_start)
		return;
	String err;
	if (!RunPyIR(inst.vm, inst.start_ir, err))
		LOG("Script on_start failed: " + err);
}

void Edit3D::RunScriptFrame(ScriptInstance& inst, double dt) {
	if (!inst.script || !inst.script->enabled || !inst.script->run_every_frame)
		return;
	if (!inst.loaded || !inst.has_frame)
		return;
	inst.vm.GetGlobalsRW().GetDictRW().GetAdd(PyValue("__dt__")) = PyValue(dt);
	String err;
	if (!RunPyIR(inst.vm, inst.frame_ir, err))
		LOG("Script on_frame failed: " + err);
}

void Edit3D::AddScriptEventHandler(const String& event, PyVM* vm, VfsValue* node, const PyValue& func) {
	if (!vm || (!func.IsFunction() && !func.IsBoundMethod()))
		return;
	ScriptEventHandler& h = script_event_handlers.Add();
	h.event = ToLower(event);
	h.func = func;
	h.vm = vm;
	h.node = node;
}

void Edit3D::RemoveScriptEventHandlers(PyVM* vm) {
	if (!vm)
		return;
	for (int i = script_event_handlers.GetCount() - 1; i >= 0; i--) {
		if (script_event_handlers[i].vm == vm)
			script_event_handlers.Remove(i);
	}
}

static bool RunPyCallback(PyVM& vm, const PyValue& func, const Vector<PyValue>& args, String& err) {
	if (!func.IsFunction() && !func.IsBoundMethod())
		return false;
	Vector<PyIR> ir;
	ir.Add(PyIR(PY_LOAD_CONST, func));
	for (const PyValue& a : args)
		ir.Add(PyIR(PY_LOAD_CONST, a));
	ir.Add(PyIR(PY_CALL_FUNCTION, (int)args.GetCount(), 0));
	ir.Add(PyIR(PY_RETURN_VALUE));
	return RunPyIR(vm, ir, err);
}

void Edit3D::DispatchScriptEvent(const String& event, VfsValue* node, const PyValue& payload) {
	String key = ToLower(event);
	for (const ScriptEventHandler& h : script_event_handlers) {
		if (h.event != key)
			continue;
		if (node && h.node && h.node != node)
			continue;
		if (!node && h.node) {
			// allow global dispatch to reach node handlers (enterFrame).
		}
		if (!h.vm)
			continue;
		Vector<PyValue> args;
		args.Add(payload);
		String err;
		if (!RunPyCallback(*h.vm, h.func, args, err))
			LOG("Script event failed: " + err);
	}
}

void Edit3D::DispatchInputEvent(const String& type, const Point& p, dword flags, int key, int view_i) {
	if (view_i >= 0 && view_i < 4)
		active_view = view_i;
	if (type.StartsWith("mouse")) {
		input_state.SetMouse(p, flags);
		if (type == "mouseDown")
			input_state.SetMouseDown(p, flags);
		else if (type == "mouseUp")
			input_state.SetMouseUp(p, flags);
		else if (type == "mouseWheel")
			input_state.AddWheel(key, flags);
	}
	else if (type == "keyDown") {
		int k = key & 0xFFFF;
		input_state.SetKey(k, true);
	}
	auto is_2d_tool = [&](EditTool t) {
		return t == TOOL_2D_SELECT || t == TOOL_2D_LINE || t == TOOL_2D_RECT || t == TOOL_2D_CIRCLE ||
		       t == TOOL_2D_POLY || t == TOOL_2D_ERASE;
	};
	auto project_world = [&](const vec3& world, Point& out) -> bool {
		if (view_i < 0 || view_i >= 4)
			return false;
		EditRendererBase* rend = v0.rends[view_i];
		if (!rend)
			return false;
		Size sz = rend->GetSize();
		if (sz.cx <= 0 || sz.cy <= 0)
			return false;
		GeomCamera& gcam = rend->GetGeomCamera();
		Camera cam;
		gcam.LoadCamera(rend->view_mode, cam, sz);
		mat4 view = cam.GetWorldMatrix();
		mat4 proj = cam.GetProjectionMatrix();
		vec4 clip = proj * (view * world.Embed());
		if (clip[3] == 0)
			return false;
		vec3 ndc = clip.Splice() / clip[3];
		if (ndc[0] < -1 || ndc[0] > 1 || ndc[1] < -1 || ndc[1] > 1)
			return false;
		out = Point(
			(int)floor((ndc[0] + 1) * 0.5 * (float)sz.cx + 0.5f),
			(int)floor((-ndc[1] + 1) * 0.5 * (float)sz.cy + 0.5f));
		return true;
	};
	auto selection_plane_normal = [&](GeomObject* obj) {
		vec3 normal = vec3(0, 0, 1);
		quat obj_ori = Identity<quat>();
		if (state) {
			if (const GeomObjectState* os = state->FindObjectStateByKey(obj->key))
				obj_ori = os->orientation;
		}
		else if (GeomTransform* tr = obj->FindTransform()) {
			obj_ori = tr->orientation;
		}
		auto set_plane_from_view = [&] {
			if (view_i < 0 || view_i >= 4)
				return;
			EditRendererBase* rend = v0.rends[view_i];
			if (!rend)
				return;
			switch (rend->view_mode) {
			case VIEWMODE_XY: normal = vec3(0, 0, 1); break;
			case VIEWMODE_XZ: normal = vec3(0, 1, 0); break;
			case VIEWMODE_YZ: normal = vec3(1, 0, 0); break;
			case VIEWMODE_PERSPECTIVE:
				normal = VectorTransform(VEC_FWD, obj_ori);
				break;
			default: break;
			}
		};
		switch (edit_plane) {
		case PLANE_XY: normal = vec3(0, 0, 1); break;
		case PLANE_XZ: normal = vec3(0, 1, 0); break;
		case PLANE_YZ: normal = vec3(1, 0, 0); break;
		case PLANE_LOCAL: normal = VectorTransform(VEC_FWD, obj_ori); break;
		default: set_plane_from_view(); break;
		}
		return normal;
	};
	auto get_2d_local = [&](GeomObject& obj, vec2& out) -> bool {
		vec3 plane_origin = vec3(0);
		vec3 plane_normal = vec3(0, 0, 1);
		vec3 obj_pos = vec3(0);
		quat obj_ori = Identity<quat>();
		vec3 obj_scale = vec3(1);
		if (state) {
			if (const GeomObjectState* os = state->FindObjectStateByKey(obj.key)) {
				obj_pos = os->position;
				obj_ori = os->orientation;
				obj_scale = os->scale;
			}
		}
		else if (GeomTransform* tr = obj.FindTransform()) {
			obj_pos = tr->position;
			obj_ori = tr->orientation;
			obj_scale = tr->scale;
		}
		plane_origin = obj_pos;
		auto set_plane_from_view = [&] {
			if (view_i < 0 || view_i >= 4)
				return;
			EditRendererBase* rend = v0.rends[view_i];
			if (!rend)
				return;
			switch (rend->view_mode) {
			case VIEWMODE_XY: plane_normal = vec3(0, 0, 1); break;
			case VIEWMODE_XZ: plane_normal = vec3(0, 1, 0); break;
			case VIEWMODE_YZ: plane_normal = vec3(1, 0, 0); break;
			case VIEWMODE_PERSPECTIVE:
				plane_normal = VectorTransform(VEC_FWD, obj_ori);
				break;
			default: break;
			}
		};
		switch (edit_plane) {
		case PLANE_XY: plane_normal = vec3(0, 0, 1); break;
		case PLANE_XZ: plane_normal = vec3(0, 1, 0); break;
		case PLANE_YZ: plane_normal = vec3(1, 0, 0); break;
		case PLANE_LOCAL: plane_normal = VectorTransform(VEC_FWD, obj_ori); break;
		default: set_plane_from_view(); break;
		}
		vec3 world;
		if (!ScreenToPlaneWorldPoint(view_i, p, plane_origin, plane_normal, world))
			return false;
		quat inv = obj_ori.GetInverse();
		vec3 v = VectorTransform(world - obj_pos, inv);
		if (obj_scale[0] != 0) v[0] /= obj_scale[0];
		if (obj_scale[1] != 0) v[1] /= obj_scale[1];
		if (obj_scale[2] != 0) v[2] /= obj_scale[2];
		out = vec2(v[0], v[1]);
		return true;
	};

	if (view == VIEW_GEOMPROJECT && (edit_tool == TOOL_MESH_SELECT || edit_tool == TOOL_OBJ_MOVE || edit_tool == TOOL_OBJ_ROTATE) && selection_dragging) {
		if (type == "mouseMove" && view_i == selection_drag_view) {
			if (selection_drag_mode == 1) {
				vec3 world;
				if (ScreenToPlaneWorldPoint(view_i, p, selection_drag_start_world, selection_drag_plane_normal, world)) {
					vec3 delta_world = world - selection_drag_start_world;
					GeomObject* obj = nullptr;
					bool is2d = false;
					vec3 center;
					if (GetSelectionCenterWorld(center, obj, is2d) && obj) {
						vec3 scale = vec3(1);
						quat ori = Identity<quat>();
						if (state) {
							if (const GeomObjectState* os = state->FindObjectStateByKey(obj->key)) {
								scale = os->scale;
								ori = os->orientation;
							}
						}
						else if (GeomTransform* tr = obj->FindTransform()) {
							scale = tr->scale;
							ori = tr->orientation;
						}
						quat inv = ori.GetInverse();
						vec3 local = VectorTransform(delta_world, inv);
						if (scale[0] != 0) local[0] /= scale[0];
						if (scale[1] != 0) local[1] /= scale[1];
						if (scale[2] != 0) local[2] /= scale[2];
						if (is2d)
							local[2] = 0;
						vec3 inc = local - selection_drag_applied_local;
						if (is2d)
							Apply2DSelectionDelta(vec2(inc[0], inc[1]));
						else
							ApplyMeshSelectionDelta(inc);
						selection_drag_applied_local = local;
					}
				}
				return;
			}
			if (selection_drag_mode == 2) {
				vec3 world;
				if (ScreenToPlaneWorldPoint(view_i, p, selection_drag_start_world, selection_drag_plane_normal, world)) {
					vec3 delta = world - selection_drag_start_world;
					if (GeomObject* obj = v0.selected_obj) {
						ApplyTransformDelta(obj, delta, false);
					}
				}
				return;
			}
			if (selection_drag_mode == 3) {
				if (GeomObject* obj = v0.selected_obj) {
					Point delta = p - selection_drag_start_mouse;
					double angle = (double)(delta.x + delta.y) * 0.005;
					int axis = 2;
					vec3 n = selection_drag_plane_normal;
					if (fabs(n[0]) > fabs(n[1]) && fabs(n[0]) > fabs(n[2]))
						axis = 0;
					else if (fabs(n[1]) > fabs(n[2]))
						axis = 1;
					ApplyTransformRotation(obj, axis, angle, transform_use_local);
				}
				return;
			}
		}
		if (type == "mouseUp") {
			selection_dragging = false;
			selection_drag_view = -1;
			selection_drag_applied_local = vec3(0);
			selection_drag_mode = 0;
			return;
		}
	}

	if (view == VIEW_GEOMPROJECT && (edit_tool == TOOL_MESH_SELECT || edit_tool == TOOL_OBJ_MOVE || edit_tool == TOOL_OBJ_ROTATE) && type == "mouseDown" && !sculpt_mode && !weight_paint_mode) {
		vec3 center_world;
		GeomObject* obj = nullptr;
		bool is2d = false;
		bool has_center = false;
		if (edit_tool == TOOL_MESH_SELECT) {
			has_center = GetSelectionCenterWorld(center_world, obj, is2d) && obj;
		}
		else if (v0.selected_obj) {
			GeomTransform* tr = v0.selected_obj->FindTransform();
			if (tr) {
				center_world = tr->position;
				has_center = true;
			}
			else if (state) {
				if (const GeomObjectState* os = state->FindObjectStateByKey(v0.selected_obj->key)) {
					center_world = os->position;
					has_center = true;
				}
			}
		}
		if (has_center && edit_tool == TOOL_MESH_SELECT) {
			Point screen;
			if (project_world(center_world, screen)) {
				int dx = screen.x - p.x;
				int dy = screen.y - p.y;
				if (dx * dx + dy * dy <= 100) {
					selection_drag_plane_normal = selection_plane_normal(obj ? obj : v0.selected_obj);
					vec3 world;
					if (ScreenToPlaneWorldPoint(view_i, p, center_world, selection_drag_plane_normal, world)) {
						selection_dragging = true;
						selection_drag_view = view_i;
						selection_drag_start_world = world;
						selection_drag_applied_local = vec3(0);
						selection_drag_mode = 1;
						return;
					}
				}
			}
		}
		if ((edit_tool == TOOL_OBJ_MOVE || edit_tool == TOOL_OBJ_ROTATE) && v0.selected_obj && has_center) {
			Point screen;
			if (project_world(center_world, screen)) {
				int dx = screen.x - p.x;
				int dy = screen.y - p.y;
				if (dx * dx + dy * dy <= 100) {
					selection_drag_plane_normal = selection_plane_normal(v0.selected_obj);
					vec3 world;
					if (ScreenToPlaneWorldPoint(view_i, p, center_world, selection_drag_plane_normal, world)) {
						selection_dragging = true;
						selection_drag_view = view_i;
						selection_drag_start_world = world;
						selection_drag_applied_local = vec3(0);
						selection_drag_mode = (edit_tool == TOOL_OBJ_MOVE) ? 2 : 3;
						selection_drag_start_pos = v0.selected_obj->FindTransform() ? v0.selected_obj->FindTransform()->position : vec3(0);
						selection_drag_start_ori = v0.selected_obj->FindTransform() ? v0.selected_obj->FindTransform()->orientation : Identity<quat>();
						selection_drag_start_mouse = p;
						return;
					}
				}
			}
		}
	}

	if (view == VIEW_GEOMPROJECT && is_2d_tool(edit_tool)) {
		GeomObject* obj = v0.selected_obj;
		if (!obj)
			return;
		Geom2DLayer* layer = obj->Find2DLayer();
		if (!layer)
			return;
		vec2 local;
		if (!get_2d_local(*obj, local))
			return;
		if (type == "mouseDown") {
			if (edit_tool == TOOL_2D_SELECT) {
				double radius = 0.05;
				int picked = Pick2DShape(*layer, local, radius);
				bool add = (flags & K_SHIFT) != 0;
				bool toggle = (flags & K_CTRL) != 0;
				Select2DShape(picked, add, toggle);
				RefrehRenderers();
				return;
			}
			if (edit_tool == TOOL_2D_ERASE) {
				double best = edit_pick_radius_px * edit_pick_radius_px;
				int best_idx = -1;
				auto dist2_seg = [](const vec2& a, const vec2& b, const vec2& p) {
					vec2 d = b - a;
					float len2 = Dot(d, d);
					if (len2 <= 1e-6f) {
						vec2 s = p - a;
						return (double)Dot(s, s);
					}
					float t = Dot(p - a, d) / len2;
					t = Clamp(t, 0.0f, 1.0f);
					vec2 c = a + d * t;
					vec2 s = p - c;
					return (double)Dot(s, s);
				};
				for (int i = 0; i < layer->shapes.GetCount(); i++) {
					const Geom2DShape& s = layer->shapes[i];
					if (s.type == Geom2DShape::S_LINE && s.points.GetCount() >= 2) {
						double d2 = dist2_seg(s.points[0], s.points[1], local);
						if (d2 < best) {
							best = d2;
							best_idx = i;
						}
					}
					else if ((s.type == Geom2DShape::S_RECT || s.type == Geom2DShape::S_POLY) && s.points.GetCount() >= 2) {
						for (int k = 1; k < s.points.GetCount(); k++) {
							double d2 = dist2_seg(s.points[k - 1], s.points[k], local);
							if (d2 < best) {
								best = d2;
								best_idx = i;
							}
						}
					}
					else if (s.type == Geom2DShape::S_CIRCLE && s.points.GetCount() >= 1) {
						float r = s.radius;
						if (r <= 0 && s.points.GetCount() >= 2) {
							vec2 d2 = s.points[1] - s.points[0];
							r = sqrt(Dot(d2, d2));
						}
						if (r > 0) {
							vec2 diff = local - s.points[0];
							float d = sqrt(Dot(diff, diff));
							double d2 = (double)fabs(d - r);
							if (d2 < best) {
								best = d2;
								best_idx = i;
							}
						}
					}
				}
				if (best_idx >= 0) {
					layer->shapes.Remove(best_idx);
					AutoKey2DEdit(obj);
					RefrehRenderers();
				}
				return;
			}
			if (edit_tool == TOOL_2D_POLY) {
				if (!draw2d_active) {
					draw2d_poly.Clear();
					draw2d_poly.Add(local);
					draw2d_active = true;
					draw2d_obj = obj;
				}
				else {
					vec2 diff = draw2d_poly[0] - local;
					float d = sqrt(Dot(diff, diff));
					if (draw2d_poly.GetCount() >= 3 && d < 0.05f) {
						Geom2DShape shape;
						shape.type = Geom2DShape::S_POLY;
						shape.points.SetCount(draw2d_poly.GetCount());
						for (int i = 0; i < draw2d_poly.GetCount(); i++)
							shape.points[i] = draw2d_poly[i];
						shape.closed = true;
						layer->shapes.Add(pick(shape));
						AutoKey2DEdit(obj);
						draw2d_poly.Clear();
						draw2d_active = false;
						draw2d_obj = nullptr;
						RefrehRenderers();
					}
					else {
						draw2d_poly.Add(local);
					}
				}
				return;
			}
			draw2d_active = true;
			draw2d_start = local;
			draw2d_last = local;
			draw2d_obj = obj;
			draw2d_view = view_i;
			return;
		}
		if (type == "mouseMove" && draw2d_active && draw2d_obj == obj) {
			draw2d_last = local;
			return;
		}
		if (type == "mouseUp" && draw2d_active && draw2d_obj == obj) {
			Geom2DShape shape;
			switch (edit_tool) {
			case TOOL_2D_LINE:
				shape.type = Geom2DShape::S_LINE;
				shape.points.Add(draw2d_start);
				shape.points.Add(draw2d_last);
				break;
			case TOOL_2D_RECT:
				shape.type = Geom2DShape::S_RECT;
				shape.points.Add(draw2d_start);
				shape.points.Add(draw2d_last);
				break;
			case TOOL_2D_CIRCLE:
				shape.type = Geom2DShape::S_CIRCLE;
				shape.points.Add(draw2d_start);
				shape.points.Add(draw2d_last);
				{
					vec2 diff = draw2d_last - draw2d_start;
					shape.radius = sqrt(Dot(diff, diff));
				}
				break;
			default:
				break;
			}
			if (shape.points.GetCount() >= 2)
				layer->shapes.Add(pick(shape));
			AutoKey2DEdit(obj);
			draw2d_active = false;
			draw2d_obj = nullptr;
			RefrehRenderers();
			return;
		}
	}

	if (view == VIEW_GEOMPROJECT && type == "mouseDown" &&
	    (edit_tool == TOOL_POINT || edit_tool == TOOL_LINE || edit_tool == TOOL_FACE ||
	     edit_tool == TOOL_ERASE || edit_tool == TOOL_JOIN || edit_tool == TOOL_SPLIT)) {
		GeomObject* obj = v0.selected_obj;
		if (obj) {
			vec3 plane_origin = vec3(0);
			vec3 plane_normal = vec3(0, 0, 1);
			vec3 obj_pos = vec3(0);
			quat obj_ori = Identity<quat>();
			vec3 obj_scale = vec3(1);
			if (state) {
				if (const GeomObjectState* os = state->FindObjectStateByKey(obj->key)) {
					obj_pos = os->position;
					obj_ori = os->orientation;
					obj_scale = os->scale;
				}
			}
			else if (GeomTransform* tr = obj->FindTransform()) {
				obj_pos = tr->position;
				obj_ori = tr->orientation;
				obj_scale = tr->scale;
			}
			plane_origin = obj_pos;
			auto set_plane_from_view = [&] {
				if (view_i < 0 || view_i >= 4)
					return;
				EditRendererBase* rend = v0.rends[view_i];
				if (!rend)
					return;
				switch (rend->view_mode) {
				case VIEWMODE_XY: plane_normal = vec3(0, 0, 1); break;
				case VIEWMODE_XZ: plane_normal = vec3(0, 1, 0); break;
				case VIEWMODE_YZ: plane_normal = vec3(1, 0, 0); break;
				case VIEWMODE_PERSPECTIVE:
					plane_normal = VectorTransform(VEC_FWD, obj_ori);
					break;
				default: break;
				}
			};
			switch (edit_plane) {
			case PLANE_XY: plane_normal = vec3(0, 0, 1); break;
			case PLANE_XZ: plane_normal = vec3(0, 1, 0); break;
			case PLANE_YZ: plane_normal = vec3(1, 0, 0); break;
			case PLANE_LOCAL: plane_normal = VectorTransform(VEC_FWD, obj_ori); break;
			default: set_plane_from_view(); break;
			}
			vec3 world;
			if (ScreenToPlaneWorldPoint(view_i, p, plane_origin, plane_normal, world)) {
				GeomEditableMesh& mesh = obj->GetEditableMesh();
				auto world_to_local = [&](const vec3& w) {
					quat inv = obj_ori.GetInverse();
					vec3 v = VectorTransform(w - obj_pos, inv);
					vec3 out = v;
					if (obj_scale[0] != 0) out[0] /= obj_scale[0];
					if (obj_scale[1] != 0) out[1] /= obj_scale[1];
					if (obj_scale[2] != 0) out[2] /= obj_scale[2];
					return out;
				};
				int picked = PickNearestPoint(mesh, view_i, p, edit_pick_radius_px);
				auto snap_local = [&](vec3 local) {
					if (!edit_snap_enable)
						return local;
					double s = edit_snap_step;
					if (s <= 0.0)
						return local;
					auto snap = [&](double v) { return (float)(floor(v / s + 0.5) * s); };
					if (edit_snap_local) {
						local[0] = snap(local[0]);
						local[1] = snap(local[1]);
						local[2] = snap(local[2]);
					}
					return local;
				};
				if (edit_tool == TOOL_POINT) {
					vec3 local = snap_local(world_to_local(world));
					mesh.points.Add(local);
					state->UpdateObjects();
					AutoKeyMeshEdit(obj);
					RefrehRenderers();
				}
				else if (edit_tool == TOOL_LINE) {
					int idx = picked;
					if (idx < 0) {
						vec3 local = snap_local(world_to_local(world));
						idx = mesh.points.GetCount();
						mesh.points.Add(local);
					}
					if (edit_line_start < 0) {
						edit_line_start = idx;
					}
					else {
						if (edit_line_start != idx) {
							GeomEdge edge;
							edge.a = edit_line_start;
							edge.b = idx;
							mesh.lines.Add(edge);
						}
						edit_line_start = idx;
					}
					state->UpdateObjects();
					AutoKeyMeshEdit(obj);
					RefrehRenderers();
				}
				else if (edit_tool == TOOL_FACE) {
					int idx = picked;
					if (idx < 0) {
						vec3 local = snap_local(world_to_local(world));
						idx = mesh.points.GetCount();
						mesh.points.Add(local);
					}
					edit_face_points.Add(idx);
					if (edit_face_points.GetCount() >= 3) {
						GeomFace face;
						face.a = edit_face_points[0];
						face.b = edit_face_points[1];
						face.c = edit_face_points[2];
						mesh.faces.Add(face);
						edit_face_points.Clear();
					}
					state->UpdateObjects();
					AutoKeyMeshEdit(obj);
					RefrehRenderers();
				}
				else if (edit_tool == TOOL_ERASE) {
					if (picked >= 0) {
						RemoveEditablePoint(mesh, picked);
						state->UpdateObjects();
						AutoKeyMeshEdit(obj);
						RefrehRenderers();
					}
				}
				else if (edit_tool == TOOL_JOIN) {
					if (picked >= 0) {
						if (edit_join_start < 0) {
							edit_join_start = picked;
						}
						else {
							if (edit_join_start != picked && !HasLine(mesh, edit_join_start, picked)) {
								GeomEdge edge;
								edge.a = edit_join_start;
								edge.b = picked;
								mesh.lines.Add(edge);
							}
							edit_join_start = picked;
						}
						state->UpdateObjects();
						AutoKeyMeshEdit(obj);
						RefrehRenderers();
					}
				}
				else if (edit_tool == TOOL_SPLIT) {
					int line_idx = PickNearestLine(mesh, view_i, p, edit_line_pick_radius_px);
					if (line_idx >= 0) {
						mesh.lines.Remove(line_idx);
						state->UpdateObjects();
						AutoKeyMeshEdit(obj);
						RefrehRenderers();
					}
				}
			}
		}
	}
	if (view == VIEW_GEOMPROJECT && type == "mouseDown" && edit_tool == TOOL_MESH_SELECT && !sculpt_mode) {
		GeomObject* obj = v0.selected_obj;
		if (obj) {
			if (GeomEditableMesh* mesh = obj->FindEditableMesh()) {
				bool add = (flags & K_SHIFT) != 0;
				bool toggle = (flags & K_CTRL) != 0;
				if (!add && !toggle)
					ClearMeshSelection();
				if (mesh_select_mode == MESHSEL_VERTEX) {
					int picked = PickNearestPoint(*mesh, view_i, p, edit_pick_radius_px);
					if (picked >= 0) {
						if (toggle)
							ToggleMeshPoint(picked);
						else if (FindIndex(mesh_sel_points, picked) < 0)
							mesh_sel_points.Add(picked);
					}
					RefrehRenderers();
					return;
				}
				if (mesh_select_mode == MESHSEL_EDGE) {
					int picked = PickNearestLine(*mesh, view_i, p, edit_line_pick_radius_px);
					if (picked >= 0) {
						if (toggle)
							ToggleMeshLine(picked);
						else if (FindIndex(mesh_sel_lines, picked) < 0)
							mesh_sel_lines.Add(picked);
					}
					RefrehRenderers();
					return;
				}
				if (mesh_select_mode == MESHSEL_FACE) {
					int picked = PickNearestFace(*mesh, view_i, p);
					if (picked >= 0) {
						if (toggle)
							ToggleMeshFace(picked);
						else if (FindIndex(mesh_sel_faces, picked) < 0)
							mesh_sel_faces.Add(picked);
					}
					RefrehRenderers();
					return;
				}
			}
			GeomSkeleton* sk = obj->FindSkeleton();
			if (sk) {
				EditRendererBase* rend = v0.rends[view_i];
				if (rend) {
					Size sz = rend->GetSize();
					if (sz.cx > 0 && sz.cy > 0) {
						GeomCamera& gcam = rend->GetGeomCamera();
						Camera cam;
						gcam.LoadCamera(rend->view_mode, cam, sz);
						mat4 view = cam.GetWorldMatrix();
						mat4 proj = cam.GetProjectionMatrix();
						auto project_point = [&](const vec3& world, Point& out) -> bool {
							vec4 clip = proj * (view * world.Embed());
							if (clip[3] == 0)
								return false;
							vec3 ndc = clip.Splice() / clip[3];
							if (ndc[0] < -1 || ndc[0] > 1 || ndc[1] < -1 || ndc[1] > 1)
								return false;
							out = Point(
								(int)floor((ndc[0] + 1) * 0.5 * (float)sz.cx + 0.5f),
								(int)floor((-ndc[1] + 1) * 0.5 * (float)sz.cy + 0.5f));
							return true;
						};
						auto dist2_segment = [](const Point& a, const Point& b, const Point& p) -> double {
							double ax = a.x;
							double ay = a.y;
							double bx = b.x;
							double by = b.y;
							double px = p.x;
							double py = p.y;
							double dx = bx - ax;
							double dy = by - ay;
							double len2 = dx * dx + dy * dy;
							if (len2 <= 1e-6) {
								double sx = px - ax;
								double sy = py - ay;
								return sx * sx + sy * sy;
							}
							double t = ((px - ax) * dx + (py - ay) * dy) / len2;
							if (t < 0.0) t = 0.0;
							if (t > 1.0) t = 1.0;
							double cx = ax + t * dx;
							double cy = ay + t * dy;
							double sx = px - cx;
							double sy = py - cy;
							return sx * sx + sy * sy;
						};
						vec3 obj_pos = vec3(0);
						quat obj_ori = Identity<quat>();
						vec3 obj_scale = vec3(1);
						if (state) {
							if (const GeomObjectState* os = state->FindObjectStateByKey(obj->key)) {
								obj_pos = os->position;
								obj_ori = os->orientation;
								obj_scale = os->scale;
							}
						}
						else if (GeomTransform* tr = obj->FindTransform()) {
							obj_pos = tr->position;
							obj_ori = tr->orientation;
							obj_scale = tr->scale;
						}
						double best = edit_line_pick_radius_px * edit_line_pick_radius_px;
						VfsValue* best_bone = nullptr;
						auto pick_bone = [&](auto&& pick_bone, VfsValue& bone_node,
						                     const vec3& parent_pos, const quat& parent_ori) -> void {
							if (!IsVfsType(bone_node, AsTypeHash<GeomBone>()))
								return;
							GeomBone& bone = bone_node.GetExt<GeomBone>();
							vec3 scaled(bone.position[0] * obj_scale[0],
							            bone.position[1] * obj_scale[1],
							            bone.position[2] * obj_scale[2]);
							vec3 pos = parent_pos + VectorTransform(scaled, parent_ori);
							Point a2, b2;
							if (project_point(parent_pos, a2) && project_point(pos, b2)) {
								double d2 = dist2_segment(a2, b2, p);
								if (d2 < best) {
									best = d2;
									best_bone = &bone_node;
								}
							}
							quat ori = parent_ori * bone.orientation;
							for (auto& sub : bone_node.sub) {
								if (IsVfsType(sub, AsTypeHash<GeomBone>()))
									pick_bone(pick_bone, sub, pos, ori);
							}
						};
						for (auto& sub : sk->val.sub) {
							if (IsVfsType(sub, AsTypeHash<GeomBone>()))
								pick_bone(pick_bone, sub, obj_pos, obj_ori);
						}
						if (best_bone) {
							selected_bone = best_bone;
							render_ctx.selected_bone = selected_bone;
							RefrehRenderers();
						}
					}
				}
			}
		}
	}
	if (view == VIEW_GEOMPROJECT && type == "mouseDown" && edit_tool == TOOL_MESH_SELECT && sculpt_mode) {
		GeomObject* obj = v0.selected_obj;
		if (obj) {
			GeomEditableMesh* mesh = obj->FindEditableMesh();
			if (mesh) {
				vec3 ray_o, ray_d;
				if (ScreenToRay(view_i, p, ray_o, ray_d)) {
					vec3 obj_pos = vec3(0);
					quat obj_ori = Identity<quat>();
					vec3 obj_scale = vec3(1);
					if (state) {
						if (const GeomObjectState* os = state->FindObjectStateByKey(obj->key)) {
							obj_pos = os->position;
							obj_ori = os->orientation;
							obj_scale = os->scale;
						}
					}
					else if (GeomTransform* tr = obj->FindTransform()) {
						obj_pos = tr->position;
						obj_ori = tr->orientation;
						obj_scale = tr->scale;
					}
					quat inv = obj_ori.GetInverse();
					vec3 local_ray_o = VectorTransform(ray_o - obj_pos, inv);
					vec3 local_ray_d = VectorTransform(ray_d, inv);
					if (obj_scale[0] != 0) local_ray_o[0] /= obj_scale[0];
					if (obj_scale[1] != 0) local_ray_o[1] /= obj_scale[1];
					if (obj_scale[2] != 0) local_ray_o[2] /= obj_scale[2];
					if (obj_scale[0] != 0) local_ray_d[0] /= obj_scale[0];
					if (obj_scale[1] != 0) local_ray_d[1] /= obj_scale[1];
					if (obj_scale[2] != 0) local_ray_d[2] /= obj_scale[2];
					local_ray_d.Normalize();
					vec3 center = local_ray_o + local_ray_d * 2.0f;
					double r = sculpt_radius;
					bool hit = false;
					double best_t = 1e9;
					if (fabs(Dot(local_ray_d, local_ray_d)) > 1e-6) {
						double b = 2.0 * Dot(local_ray_o, local_ray_d);
						double c = Dot(local_ray_o, local_ray_o) - r * r;
						double disc = b * b - 4.0 * c;
						if (disc >= 0.0) {
							double t0 = (-b - sqrt(disc)) * 0.5;
							double t1 = (-b + sqrt(disc)) * 0.5;
							double t = t0;
							if (t < 0) t = t1;
							if (t > 0) {
								best_t = t;
								hit = true;
							}
						}
					}
					if (hit) {
						center = local_ray_o + local_ray_d * best_t;
					}
					for (vec3& pt : mesh->points) {
						vec3 diff = pt - center;
						float dist = diff.GetLength();
						if (dist > sculpt_radius || dist <= 1e-6f)
							continue;
						float falloff = 1.0f - dist / (float)sculpt_radius;
						float s = (float)sculpt_strength * falloff;
						if (!sculpt_add)
							s = -s;
						pt += diff.GetNormalized() * s;
					}
					state->UpdateObjects();
					AutoKeyMeshEdit(obj);
					RefrehRenderers();
				}
			}
		}
	}
	if (view == VIEW_GEOMPROJECT && type == "mouseDown" && edit_tool == TOOL_MESH_SELECT && weight_paint_mode) {
		GeomObject* obj = v0.selected_obj;
		if (obj && selected_bone) {
			if (!IsVfsType(*selected_bone, AsTypeHash<GeomBone>()))
				return;
			GeomBone& bone = selected_bone->GetExt<GeomBone>();
			String bone_name = bone.name.IsEmpty() ? selected_bone->id : bone.name;
			GeomEditableMesh* mesh = obj->FindEditableMesh();
			if (mesh) {
				vec3 ray_o, ray_d;
				if (ScreenToRay(view_i, p, ray_o, ray_d)) {
					vec3 obj_pos = vec3(0);
					quat obj_ori = Identity<quat>();
					vec3 obj_scale = vec3(1);
					if (state) {
						if (const GeomObjectState* os = state->FindObjectStateByKey(obj->key)) {
							obj_pos = os->position;
							obj_ori = os->orientation;
							obj_scale = os->scale;
						}
					}
					else if (GeomTransform* tr = obj->FindTransform()) {
						obj_pos = tr->position;
						obj_ori = tr->orientation;
						obj_scale = tr->scale;
					}
					quat inv = obj_ori.GetInverse();
					vec3 local_ray_o = VectorTransform(ray_o - obj_pos, inv);
					vec3 local_ray_d = VectorTransform(ray_d, inv);
					if (obj_scale[0] != 0) local_ray_o[0] /= obj_scale[0];
					if (obj_scale[1] != 0) local_ray_o[1] /= obj_scale[1];
					if (obj_scale[2] != 0) local_ray_o[2] /= obj_scale[2];
					if (obj_scale[0] != 0) local_ray_d[0] /= obj_scale[0];
					if (obj_scale[1] != 0) local_ray_d[1] /= obj_scale[1];
					if (obj_scale[2] != 0) local_ray_d[2] /= obj_scale[2];
					local_ray_d.Normalize();
					int closest = -1;
					double best = 1e9;
					for (int i = 0; i < mesh->points.GetCount(); i++) {
						vec3 p0 = mesh->points[i];
						vec3 diff = p0 - local_ray_o;
						double t = Dot(diff, local_ray_d);
						vec3 proj = local_ray_o + local_ray_d * t;
						double d2 = (p0 - proj).GetLength();
						if (d2 < best) {
							best = d2;
							closest = i;
						}
					}
					if (closest >= 0) {
						vec3 center = mesh->points[closest];
						GeomSkinWeights& sw = obj->GetSkinWeights();
						Vector<float>& w = sw.weights.GetAdd(bone_name);
						w.SetCount(mesh->points.GetCount(), 0.0f);
						for (int i = 0; i < mesh->points.GetCount(); i++) {
							vec3 diff = mesh->points[i] - center;
							float dist = diff.GetLength();
							if (dist > weight_radius)
								continue;
							float falloff = 1.0f - dist / (float)weight_radius;
							float delta = (float)weight_strength * falloff;
							if (!weight_add)
								delta = -delta;
							float nv = w[i] + delta;
							w[i] = Clamp(nv, 0.0f, 1.0f);
						}
						state->UpdateObjects();
						RefrehRenderers();
					}
				}
			}
		}
	}
	PyValue payload = PyValue::Dict();
	payload.SetItem(PyValue("type"), PyValue(type));
	payload.SetItem(PyValue("x"), PyValue(p.x));
	payload.SetItem(PyValue("y"), PyValue(p.y));
	payload.SetItem(PyValue("flags"), PyValue((int64)flags));
	payload.SetItem(PyValue("key"), PyValue(key));
	payload.SetItem(PyValue("view"), PyValue(view_i));
	if (anim) {
		payload.SetItem(PyValue("time"), PyValue(anim->time));
		payload.SetItem(PyValue("frame"), PyValue(anim->position));
	}
	DispatchScriptEvent(type, nullptr, payload);
}

void Edit3D::DispatchFrameEvents(double dt) {
	double time = anim ? anim->time : 0.0;
	int frame = anim ? anim->position : 0;
	for (const ScriptEventHandler& h : script_event_handlers) {
		if (h.event != "enterframe")
			continue;
		if (!h.vm)
			continue;
		PyValue payload = PyValue::Dict();
		payload.SetItem(PyValue("type"), PyValue("enterFrame"));
		payload.SetItem(PyValue("dt"), PyValue(dt));
		payload.SetItem(PyValue("time"), PyValue(time));
		payload.SetItem(PyValue("frame"), PyValue(frame));
		if (h.node)
			payload.SetItem(PyValue("target"), MakeDisplayObject(this, h.node, h.vm));
		Vector<PyValue> args;
		args.Add(payload);
		String err;
		if (!RunPyCallback(*h.vm, h.func, args, err))
			LOG("Script enterFrame failed: " + err);
	}
}

void Edit3D::SetEditTool(EditTool tool) {
	edit_tool = tool;
	edit_line_start = -1;
	edit_join_start = -1;
	edit_face_points.Clear();
	draw2d_active = false;
	draw2d_poly.Clear();
	draw2d_obj = nullptr;
	bool cam_input = (edit_tool == TOOL_VIEW);
	for (int i = 0; i < 4; i++) {
		if (v0.rends[i])
			v0.rends[i]->SetCameraInputEnabled(cam_input);
	}
	tool_panel.Sync();
}

void Edit3D::ApplyRendererCameraSource(int view_i, CameraSource src, hash_t object_key) {
	if (view_i < 0 || view_i >= 4)
		return;
	EditRendererBase* rend = v0.rends[view_i];
	if (!rend)
		return;
	rend->cam_src = src;
	if (src == CAMSRC_OBJECT)
		rend->SetCameraObjectKey(object_key);
	RefrehRenderers();
}

void Edit3D::ApplyToolPanelCameraSource(CameraSource src) {
	int view_idx = 0;
	Value v = tool_panel.cam_view.GetData();
	if (!IsNull(v))
		view_idx = (int)v;
	int rend_i = view_idx <= 0 ? active_view : view_idx - 1;
	if (rend_i < 0 || rend_i >= 4)
		rend_i = active_view;
	hash_t obj_key = 0;
	if (src == CAMSRC_OBJECT) {
		if (v0.selected_obj && v0.selected_obj->IsCamera())
			obj_key = v0.selected_obj->key;
	}
	if (src == CAMSRC_OBJECT && obj_key == 0) {
		src = CAMSRC_FOCUS;
	}
	ApplyRendererCameraSource(rend_i, src, obj_key);
	tool_panel.Sync();
}

void Edit3D::CreateEditableMeshObject() {
	if (!state)
		return;
	PushUndo("Create Editable Mesh");
	GeomDirectory* dir = nullptr;
	if (v0.selected_ref && v0.selected_ref->kind == GeomProjectCtrl::TreeNodeRef::K_VFS && v0.selected_ref->vfs)
		dir = GetNodeDirectory(*v0.selected_ref->vfs);
	if (!dir)
		dir = &state->GetActiveScene();
	String base = "editable_mesh";
	String name = base;
	int idx = 1;
	while (dir->FindObject(name))
		name = base + "_" + IntStr(idx++);
	GeomObject& obj = dir->GetAddModel(name);
	obj.type = GeomObject::O_MODEL;
	obj.GetEditableMesh();
	state->UpdateObjects();
	RefreshData();
}

void Edit3D::Create2DLayerObject() {
	if (!state)
		return;
	PushUndo("Create 2D Layer");
	GeomDirectory* dir = nullptr;
	if (v0.selected_ref && v0.selected_ref->kind == GeomProjectCtrl::TreeNodeRef::K_VFS && v0.selected_ref->vfs)
		dir = GetNodeDirectory(*v0.selected_ref->vfs);
	if (!dir)
		dir = &state->GetActiveScene();
	String base = "layer2d";
	String name = base;
	int idx = 1;
	while (dir->FindObject(name))
		name = base + "_" + IntStr(idx++);
	GeomObject& obj = dir->GetAddModel(name);
	obj.type = GeomObject::O_MODEL;
	obj.Get2DLayer();
	state->UpdateObjects();
	RefreshData();
}

GeomObject* Edit3D::GetFocusedMeshObject() {
	if (!state || state->focus_mode != 1 || !state->focus_object_key)
		return nullptr;
	GeomObject* obj = state->FindObjectByKey(state->focus_object_key);
	if (!obj || !obj->FindEditableMesh())
		return nullptr;
	return obj;
}

GeomObject* Edit3D::GetFocused2DObject() {
	if (!state || state->focus_mode != 1 || !state->focus_object_key)
		return nullptr;
	GeomObject* obj = state->FindObjectByKey(state->focus_object_key);
	if (!obj || !obj->Find2DLayer())
		return nullptr;
	return obj;
}

void Edit3D::Clear2DSelection() {
	select_2d_shapes.Clear();
	select_2d_primary = -1;
	sel2d_offset = vec2(0);
}

void Edit3D::Select2DShape(int idx, bool add, bool toggle) {
	if (idx < 0) {
		if (!add && !toggle)
			Clear2DSelection();
		return;
	}
	if (toggle) {
		Toggle2DShape(idx);
		return;
	}
	if (!add)
		Clear2DSelection();
	if (FindIndex(select_2d_shapes, idx) < 0)
		select_2d_shapes.Add(idx);
	select_2d_primary = idx;
	sel2d_offset = vec2(0);
}

void Edit3D::Toggle2DShape(int idx) {
	int pos = FindIndex(select_2d_shapes, idx);
	if (pos >= 0)
		select_2d_shapes.Remove(pos);
	else
		select_2d_shapes.Add(idx);
	select_2d_primary = idx;
	sel2d_offset = vec2(0);
}

Rectf Edit3D::Get2DShapeBounds(const Geom2DShape& shape) const {
	if (shape.points.IsEmpty())
		return Rectf(0, 0, 0, 0);
	float minx = shape.points[0][0];
	float maxx = shape.points[0][0];
	float miny = shape.points[0][1];
	float maxy = shape.points[0][1];
	for (int i = 1; i < shape.points.GetCount(); i++) {
		minx = min(minx, shape.points[i][0]);
		maxx = max(maxx, shape.points[i][0]);
		miny = min(miny, shape.points[i][1]);
		maxy = max(maxy, shape.points[i][1]);
	}
	if (shape.type == Geom2DShape::S_CIRCLE && shape.radius > 0) {
		minx = shape.points[0][0] - shape.radius;
		maxx = shape.points[0][0] + shape.radius;
		miny = shape.points[0][1] - shape.radius;
		maxy = shape.points[0][1] + shape.radius;
	}
	return Rectf(minx, miny, maxx, maxy);
}

void Edit3D::Translate2DShape(Geom2DShape& shape, const vec2& delta) {
	for (vec2& p : shape.points)
		p += delta;
}

int Edit3D::Pick2DShape(const Geom2DLayer& layer, const vec2& local, double radius) const {
	double best = radius * radius;
	int best_idx = -1;
	auto dist2_seg = [](const vec2& a, const vec2& b, const vec2& p) {
		vec2 d = b - a;
		float len2 = Dot(d, d);
		if (len2 <= 1e-6f) {
			vec2 s = p - a;
			return (double)Dot(s, s);
		}
		float t = Dot(p - a, d) / len2;
		t = Clamp(t, 0.0f, 1.0f);
		vec2 c = a + d * t;
		vec2 s = p - c;
		return (double)Dot(s, s);
	};
	for (int i = 0; i < layer.shapes.GetCount(); i++) {
		const Geom2DShape& s = layer.shapes[i];
		if (s.type == Geom2DShape::S_LINE && s.points.GetCount() >= 2) {
			double d2 = dist2_seg(s.points[0], s.points[1], local);
			if (d2 < best) {
				best = d2;
				best_idx = i;
			}
		}
		else if ((s.type == Geom2DShape::S_RECT || s.type == Geom2DShape::S_POLY) && s.points.GetCount() >= 2) {
			for (int k = 1; k < s.points.GetCount(); k++) {
				double d2 = dist2_seg(s.points[k - 1], s.points[k], local);
				if (d2 < best) {
					best = d2;
					best_idx = i;
				}
			}
			if (s.closed) {
				double d2 = dist2_seg(s.points.Top(), s.points[0], local);
				if (d2 < best) {
					best = d2;
					best_idx = i;
				}
			}
		}
		else if (s.type == Geom2DShape::S_CIRCLE && s.points.GetCount() >= 1) {
			float r = s.radius;
			if (r <= 0 && s.points.GetCount() >= 2) {
				vec2 d2 = s.points[1] - s.points[0];
				r = sqrt(Dot(d2, d2));
			}
			if (r > 0) {
				vec2 diff = local - s.points[0];
				float d = sqrt(Dot(diff, diff));
				double d2 = (double)fabs(d - r);
				if (d2 < best) {
					best = d2;
					best_idx = i;
				}
			}
		}
	}
	return best_idx;
}

void Edit3D::Align2DSelection(int mode) {
	GeomObject* obj = GetFocused2DObject();
	if (!obj || select_2d_shapes.IsEmpty())
		return;
	Geom2DLayer* layer = obj->Find2DLayer();
	if (!layer)
		return;
	PushUndo("Align 2D shapes");
	Rectf bounds = Get2DShapeBounds(layer->shapes[select_2d_shapes[0]]);
	for (int i = 1; i < select_2d_shapes.GetCount(); i++)
		bounds.Union(Get2DShapeBounds(layer->shapes[select_2d_shapes[i]]));
	for (int idx : select_2d_shapes) {
		Geom2DShape& s = layer->shapes[idx];
		Rectf b = Get2DShapeBounds(s);
		vec2 delta(0, 0);
		Pointf c0 = bounds.CenterPoint();
		Pointf c1 = b.CenterPoint();
		switch (mode) {
		case 0: delta[0] = (float)(bounds.left - b.left); break;
		case 1: delta[0] = (float)(c0.x - c1.x); break;
		case 2: delta[0] = (float)(bounds.right - b.right); break;
		case 3: delta[1] = (float)(bounds.top - b.top); break;
		case 4: delta[1] = (float)(c0.y - c1.y); break;
		case 5: delta[1] = (float)(bounds.bottom - b.bottom); break;
		default: break;
		}
		Translate2DShape(s, delta);
	}
	AutoKey2DEdit(obj);
	RefrehRenderers();
}

void Edit3D::Distribute2DSelection(bool horizontal) {
	GeomObject* obj = GetFocused2DObject();
	if (!obj || select_2d_shapes.GetCount() < 3)
		return;
	Geom2DLayer* layer = obj->Find2DLayer();
	if (!layer)
		return;
	PushUndo("Distribute 2D shapes");
	Vector<int> ids;
	ids.Append(select_2d_shapes);
	Sort(ids, [&](int a, int b) {
		Rectf ra = Get2DShapeBounds(layer->shapes[a]);
		Rectf rb = Get2DShapeBounds(layer->shapes[b]);
		return horizontal ? ra.CenterPoint().x < rb.CenterPoint().x : ra.CenterPoint().y < rb.CenterPoint().y;
	});
	Rectf first = Get2DShapeBounds(layer->shapes[ids[0]]);
	Rectf last = Get2DShapeBounds(layer->shapes[ids.Top()]);
	double span = horizontal ? (last.CenterPoint().x - first.CenterPoint().x) : (last.CenterPoint().y - first.CenterPoint().y);
	if (fabs(span) < 1e-6)
		return;
	for (int i = 1; i < ids.GetCount() - 1; i++) {
		double t = (double)i / (double)(ids.GetCount() - 1);
		Rectf b = Get2DShapeBounds(layer->shapes[ids[i]]);
		vec2 delta(0, 0);
		if (horizontal)
			delta[0] = (float)(first.CenterPoint().x + span * t - b.CenterPoint().x);
		else
			delta[1] = (float)(first.CenterPoint().y + span * t - b.CenterPoint().y);
		Translate2DShape(layer->shapes[ids[i]], delta);
	}
	AutoKey2DEdit(obj);
	RefrehRenderers();
}

namespace {

static float PolyArea(const Vector<vec2>& pts) {
	if (pts.GetCount() < 3)
		return 0.0f;
	double area = 0.0;
	for (int i = 0; i < pts.GetCount(); i++) {
		const vec2& a = pts[i];
		const vec2& b = pts[(i + 1) % pts.GetCount()];
		area += (double)a[0] * (double)b[1] - (double)a[1] * (double)b[0];
	}
	return (float)(area * 0.5);
}

static void EnsureCCW(Vector<vec2>& pts) {
	if (PolyArea(pts) < 0.0f)
		Reverse(pts);
}

static Vector<vec2> ShapeToPolygon(const Geom2DShape& shape) {
	Vector<vec2> pts;
	switch (shape.type) {
	case Geom2DShape::S_LINE: {
		if (shape.points.GetCount() < 2)
			return pts;
		vec2 a = shape.points[0];
		vec2 b = shape.points[1];
		vec2 d = b - a;
		float len = sqrt(Dot(d, d));
		if (len < 1e-6f)
			return pts;
		vec2 n(-d[1] / len, d[0] / len);
		float w = shape.width > 0 ? shape.width : 1.0f;
		float h = w * 0.5f;
		pts << (a + n * h) << (b + n * h) << (b - n * h) << (a - n * h);
		break;
	}
	case Geom2DShape::S_RECT: {
		if (shape.points.GetCount() < 2)
			return pts;
		vec2 a = shape.points[0];
		vec2 b = shape.points[1];
		vec2 p0(min(a[0], b[0]), min(a[1], b[1]));
		vec2 p1(max(a[0], b[0]), min(a[1], b[1]));
		vec2 p2(max(a[0], b[0]), max(a[1], b[1]));
		vec2 p3(min(a[0], b[0]), max(a[1], b[1]));
		pts << p0 << p1 << p2 << p3;
		break;
	}
	case Geom2DShape::S_CIRCLE: {
		if (shape.points.IsEmpty())
			return pts;
		vec2 c = shape.points[0];
		float r = shape.radius;
		if (r <= 0 && shape.points.GetCount() >= 2) {
			vec2 d2 = shape.points[1] - shape.points[0];
			r = sqrt(Dot(d2, d2));
		}
		if (r <= 0)
			return pts;
		const int steps = 24;
		pts.SetCount(steps);
		for (int i = 0; i < steps; i++) {
			float a = (float)i / (float)steps * 2.0f * (float)M_PI;
			pts[i] = c + vec2(cos(a), sin(a)) * r;
		}
		break;
	}
	case Geom2DShape::S_POLY: {
		if (shape.points.GetCount() < 3)
			return pts;
		pts.Append(shape.points);
		break;
	}
	default: break;
	}
	if (pts.GetCount() >= 3)
		EnsureCCW(pts);
	return pts;
}

static void CleanupPolygon(Vector<vec2>& pts) {
	const float eps = 1e-4f;
	for (int i = 0; i < pts.GetCount(); ) {
		int j = (i + 1) % pts.GetCount();
		vec2 d = pts[j] - pts[i];
		if (sqrt(Dot(d, d)) < eps)
			pts.Remove(j);
		else
			i++;
		if (pts.GetCount() < 3)
			break;
	}
	if (pts.GetCount() >= 3) {
		vec2 d = pts.Top() - pts[0];
		if (sqrt(Dot(d, d)) < eps)
			pts.Drop();
	}
}

static bool PointInPolygon(const Vector<vec2>& poly, const vec2& p) {
	if (poly.GetCount() < 3)
		return false;
	bool inside = false;
	for (int i = 0, j = poly.GetCount() - 1; i < poly.GetCount(); j = i++) {
		const vec2& a = poly[i];
		const vec2& b = poly[j];
		bool intersect = ((a[1] > p[1]) != (b[1] > p[1])) &&
		                 (p[0] < (b[0] - a[0]) * (p[1] - a[1]) / (b[1] - a[1] + 1e-9f) + a[0]);
		if (intersect)
			inside = !inside;
	}
	return inside;
}

struct GHNode : Moveable<GHNode> {
	vec2 pt = vec2(0);
	double alpha = 0.0;
	bool intersect = false;
	bool entry = false;
	bool visited = false;
	GHNode* next = nullptr;
	GHNode* prev = nullptr;
	GHNode* neighbor = nullptr;
};

static GHNode* BuildPolygonList(const Vector<vec2>& poly, Array<GHNode>& pool, Vector<GHNode*>& nodes) {
	if (poly.GetCount() < 3)
		return nullptr;
	nodes.SetCount(poly.GetCount());
	for (int i = 0; i < poly.GetCount(); i++) {
		GHNode& n = pool.Add();
		n.pt = poly[i];
		n.intersect = false;
		nodes[i] = &n;
	}
	for (int i = 0; i < nodes.GetCount(); i++) {
		GHNode* n = nodes[i];
		n->next = nodes[(i + 1) % nodes.GetCount()];
		n->prev = nodes[(i - 1 + nodes.GetCount()) % nodes.GetCount()];
	}
	return nodes[0];
}

static bool SegmentIntersect(const vec2& p1, const vec2& p2, const vec2& q1, const vec2& q2,
                             vec2& out, double& t, double& u) {
	vec2 r = p2 - p1;
	vec2 s = q2 - q1;
	float denom = r[0] * s[1] - r[1] * s[0];
	if (fabs(denom) < 1e-8f)
		return false;
	vec2 qp = q1 - p1;
	t = (qp[0] * s[1] - qp[1] * s[0]) / denom;
	u = (qp[0] * r[1] - qp[1] * r[0]) / denom;
	if (t < -1e-6 || t > 1.0 + 1e-6 || u < -1e-6 || u > 1.0 + 1e-6)
		return false;
	if (t < 0) t = 0;
	if (t > 1) t = 1;
	if (u < 0) u = 0;
	if (u > 1) u = 1;
	out = p1 + r * (float)t;
	return true;
}

static void InsertIntersections(GHNode* edge_start, const Vector<GHNode*>& inserts) {
	if (!edge_start || inserts.IsEmpty())
		return;
	Vector<GHNode*> sorted;
	sorted.Append(inserts);
	Sort(sorted, [](const GHNode* a, const GHNode* b) { return a->alpha < b->alpha; });
	GHNode* cur = edge_start;
	for (GHNode* n : sorted) {
		n->prev = cur;
		n->next = cur->next;
		cur->next->prev = n;
		cur->next = n;
		cur = n;
	}
}

enum BoolOp {
	OP_UNION,
	OP_INTERSECT,
	OP_DIFF
};

static Vector<Vector<vec2>> PolygonBoolean(const Vector<vec2>& subject, const Vector<vec2>& clip, BoolOp op) {
	Vector<Vector<vec2>> result;
	if (subject.GetCount() < 3 || clip.GetCount() < 3)
		return result;
	Vector<vec2> subj;
	Vector<vec2> clp;
	subj.Append(subject);
	clp.Append(clip);
	EnsureCCW(subj);
	EnsureCCW(clp);
	Array<GHNode> pool;
	Vector<GHNode*> subj_nodes;
	Vector<GHNode*> clip_nodes;
	GHNode* subj_start = BuildPolygonList(subj, pool, subj_nodes);
	GHNode* clip_start = BuildPolygonList(clp, pool, clip_nodes);
	if (!subj_start || !clip_start)
		return result;
	VectorMap<GHNode*, Vector<GHNode*>> subj_inserts;
	VectorMap<GHNode*, Vector<GHNode*>> clip_inserts;
	Vector<GHNode*> subj_inters;
	Vector<GHNode*> clip_inters;
	for (int i = 0; i < subj_nodes.GetCount(); i++) {
		GHNode* s = subj_nodes[i];
		vec2 s1 = s->pt;
		vec2 s2 = s->next->pt;
		for (int j = 0; j < clip_nodes.GetCount(); j++) {
			GHNode* c = clip_nodes[j];
			vec2 c1 = c->pt;
			vec2 c2 = c->next->pt;
			vec2 ip;
			double ts = 0;
			double tc = 0;
			if (!SegmentIntersect(s1, s2, c1, c2, ip, ts, tc))
				continue;
			GHNode& ns = pool.Add();
			ns.pt = ip;
			ns.intersect = true;
			ns.alpha = ts;
			GHNode& nc = pool.Add();
			nc.pt = ip;
			nc.intersect = true;
			nc.alpha = tc;
			ns.neighbor = &nc;
			nc.neighbor = &ns;
			subj_inters.Add(&ns);
			clip_inters.Add(&nc);
			int si = subj_inserts.Find(s);
			if (si < 0) {
				subj_inserts.Add(s, Vector<GHNode*>());
				si = subj_inserts.GetCount() - 1;
			}
			subj_inserts[si].Add(&ns);
			int ci = clip_inserts.Find(c);
			if (ci < 0) {
				clip_inserts.Add(c, Vector<GHNode*>());
				ci = clip_inserts.GetCount() - 1;
			}
			clip_inserts[ci].Add(&nc);
		}
	}
	for (int i = 0; i < subj_inserts.GetCount(); i++)
		InsertIntersections(subj_inserts.GetKey(i), subj_inserts[i]);
	for (int i = 0; i < clip_inserts.GetCount(); i++)
		InsertIntersections(clip_inserts.GetKey(i), clip_inserts[i]);
	bool subj_in = PointInPolygon(clp, subj_start->pt);
	bool clip_in = PointInPolygon(subj, clip_start->pt);
	for (GHNode* n = subj_start; ; n = n->next) {
		if (n->intersect) {
			if (op == OP_INTERSECT)
				n->entry = !subj_in;
			else if (op == OP_UNION)
				n->entry = subj_in;
			else
				n->entry = !subj_in;
			subj_in = !subj_in;
		}
		if (n->next == subj_start)
			break;
	}
	for (GHNode* n = clip_start; ; n = n->next) {
		if (n->intersect) {
			if (op == OP_INTERSECT)
				n->entry = !clip_in;
			else if (op == OP_UNION)
				n->entry = clip_in;
			else
				n->entry = clip_in;
			clip_in = !clip_in;
		}
		if (n->next == clip_start)
			break;
	}
	if (subj_inters.IsEmpty()) {
		bool subj_inside = PointInPolygon(clp, subj[0]);
		bool clip_inside = PointInPolygon(subj, clp[0]);
		if (op == OP_INTERSECT) {
			if (subj_inside)
				result.Add(pick(subj));
			else if (clip_inside)
				result.Add(pick(clp));
		}
		else if (op == OP_UNION) {
			if (subj_inside)
				result.Add(pick(clp));
			else if (clip_inside)
				result.Add(pick(subj));
			else {
				result.Add(pick(subj));
				result.Add(pick(clp));
			}
		}
		else if (op == OP_DIFF) {
			if (!subj_inside)
				result.Add(pick(subj));
		}
		return result;
	}
	for (GHNode* n : subj_inters) {
		if (!n->entry || n->visited)
			continue;
		Vector<vec2> poly;
		GHNode* cur = n;
		bool in_subject = true;
		do {
			if (cur->visited && cur->intersect)
				break;
			poly.Add(cur->pt);
			if (cur->intersect) {
				cur->visited = true;
				cur = cur->neighbor;
				if (cur->visited)
					break;
				in_subject = !in_subject;
			}
			cur = (op == OP_DIFF && !in_subject) ? cur->prev : cur->next;
		} while (cur && cur != n);
		CleanupPolygon(poly);
		if (poly.GetCount() >= 3) {
			EnsureCCW(poly);
			result.Add(pick(poly));
		}
	}
	return result;
}

}

void Edit3D::Union2DSelection() {
	GeomObject* obj = GetFocused2DObject();
	if (!obj || select_2d_shapes.GetCount() < 2)
		return;
	Geom2DLayer* layer = obj->Find2DLayer();
	if (!layer)
		return;
	PushUndo("Union 2D shapes");
	Vector<Vector<vec2>> polys;
	polys.Add(ShapeToPolygon(layer->shapes[select_2d_shapes[0]]));
	for (int i = 1; i < select_2d_shapes.GetCount(); i++) {
		Vector<vec2> other = ShapeToPolygon(layer->shapes[select_2d_shapes[i]]);
		if (other.GetCount() < 3)
			continue;
		Vector<Vector<vec2>> next;
		for (int k = 0; k < polys.GetCount(); k++) {
			Vector<Vector<vec2>> out = PolygonBoolean(polys[k], other, OP_UNION);
			next.Append(out);
		}
		polys = pick(next);
	}
	if (polys.IsEmpty())
		return;
	const Geom2DShape& base = layer->shapes[select_2d_shapes[0]];
	for (int i = 0; i < polys.GetCount(); i++) {
		Vector<vec2> poly = pick(polys[i]);
		if (poly.GetCount() < 3)
			continue;
		Geom2DShape shape;
		shape.type = Geom2DShape::S_POLY;
		shape.points = pick(poly);
		shape.closed = true;
		shape.stroke = base.stroke;
		shape.width = base.width;
		shape.stroke_cap = base.stroke_cap;
		shape.stroke_join = base.stroke_join;
		layer->shapes.Add(pick(shape));
	}
	for (int i = select_2d_shapes.GetCount() - 1; i >= 0; i--)
		layer->shapes.Remove(select_2d_shapes[i]);
	Clear2DSelection();
	AutoKey2DEdit(obj);
	RefrehRenderers();
}

void Edit3D::Intersect2DSelection() {
	GeomObject* obj = GetFocused2DObject();
	if (!obj || select_2d_shapes.GetCount() < 2)
		return;
	Geom2DLayer* layer = obj->Find2DLayer();
	if (!layer)
		return;
	PushUndo("Intersect 2D shapes");
	Vector<Vector<vec2>> polys;
	polys.Add(ShapeToPolygon(layer->shapes[select_2d_shapes[0]]));
	for (int i = 1; i < select_2d_shapes.GetCount(); i++) {
		Vector<vec2> other = ShapeToPolygon(layer->shapes[select_2d_shapes[i]]);
		if (other.GetCount() < 3)
			continue;
		Vector<Vector<vec2>> next;
		for (int k = 0; k < polys.GetCount(); k++) {
			Vector<Vector<vec2>> out = PolygonBoolean(polys[k], other, OP_INTERSECT);
			next.Append(out);
		}
		polys = pick(next);
	}
	if (polys.IsEmpty())
		return;
	const Geom2DShape& base = layer->shapes[select_2d_shapes[0]];
	for (int i = 0; i < polys.GetCount(); i++) {
		Vector<vec2> poly = pick(polys[i]);
		if (poly.GetCount() < 3)
			continue;
		Geom2DShape shape;
		shape.type = Geom2DShape::S_POLY;
		shape.points = pick(poly);
		shape.closed = true;
		shape.stroke = base.stroke;
		shape.width = base.width;
		shape.stroke_cap = base.stroke_cap;
		shape.stroke_join = base.stroke_join;
		layer->shapes.Add(pick(shape));
	}
	for (int i = select_2d_shapes.GetCount() - 1; i >= 0; i--)
		layer->shapes.Remove(select_2d_shapes[i]);
	Clear2DSelection();
	AutoKey2DEdit(obj);
	RefrehRenderers();
}

void Edit3D::Subtract2DSelection() {
	GeomObject* obj = GetFocused2DObject();
	if (!obj || select_2d_shapes.GetCount() < 2)
		return;
	Geom2DLayer* layer = obj->Find2DLayer();
	if (!layer)
		return;
	PushUndo("Subtract 2D shapes");
	Vector<Vector<vec2>> polys;
	polys.Add(ShapeToPolygon(layer->shapes[select_2d_shapes[0]]));
	for (int i = 1; i < select_2d_shapes.GetCount(); i++) {
		Vector<vec2> other = ShapeToPolygon(layer->shapes[select_2d_shapes[i]]);
		if (other.GetCount() < 3)
			continue;
		Vector<Vector<vec2>> next;
		for (int k = 0; k < polys.GetCount(); k++) {
			Vector<Vector<vec2>> out = PolygonBoolean(polys[k], other, OP_DIFF);
			next.Append(out);
		}
		polys = pick(next);
	}
	if (!polys.IsEmpty()) {
		const Geom2DShape& base = layer->shapes[select_2d_shapes[0]];
		for (int i = 0; i < polys.GetCount(); i++) {
			Vector<vec2> poly = pick(polys[i]);
			if (poly.GetCount() < 3)
				continue;
			Geom2DShape shape;
			shape.type = Geom2DShape::S_POLY;
			shape.points = pick(poly);
			shape.closed = true;
			shape.stroke = base.stroke;
			shape.width = base.width;
			shape.stroke_cap = base.stroke_cap;
			shape.stroke_join = base.stroke_join;
			layer->shapes.Add(pick(shape));
		}
	}
	for (int i = select_2d_shapes.GetCount() - 1; i >= 0; i--)
		layer->shapes.Remove(select_2d_shapes[i]);
	Clear2DSelection();
	AutoKey2DEdit(obj);
	RefrehRenderers();
}

bool Edit3D::GetMeshSelectionCenter(vec3& out) {
	GeomObject* obj = GetFocusedMeshObject();
	if (!obj)
		return false;
	const GeomEditableMesh* mesh = obj->FindEditableMesh();
	if (!mesh)
		return false;
	Index<int> verts;
	for (int id : mesh_sel_points)
		verts.FindAdd(id);
	for (int id : mesh_sel_lines) {
		if (id < 0 || id >= mesh->lines.GetCount())
			continue;
		const GeomEdge& e = mesh->lines[id];
		verts.FindAdd(e.a);
		verts.FindAdd(e.b);
	}
	for (int id : mesh_sel_faces) {
		if (id < 0 || id >= mesh->faces.GetCount())
			continue;
		const GeomFace& f = mesh->faces[id];
		verts.FindAdd(f.a);
		verts.FindAdd(f.b);
		verts.FindAdd(f.c);
	}
	if (verts.IsEmpty())
		return false;
	vec3 center(0, 0, 0);
	int cnt = 0;
	for (int id : verts) {
		if (id < 0 || id >= mesh->points.GetCount())
			continue;
		center += mesh->points[id];
		cnt++;
	}
	if (cnt == 0)
		return false;
	center /= (float)cnt;
	out = center;
	return true;
}

bool Edit3D::Get2DSelectionCenter(vec3& out) {
	GeomObject* obj = GetFocused2DObject();
	if (!obj)
		return false;
	Geom2DLayer* layer = obj->Find2DLayer();
	if (!layer || select_2d_shapes.IsEmpty())
		return false;
	Rectf bounds = Get2DShapeBounds(layer->shapes[select_2d_shapes[0]]);
	for (int i = 1; i < select_2d_shapes.GetCount(); i++)
		bounds.Union(Get2DShapeBounds(layer->shapes[select_2d_shapes[i]]));
	Pointf c = bounds.CenterPoint();
	out = vec3((float)c.x, (float)c.y, 0.0f);
	return true;
}

bool Edit3D::GetSelectionCenterWorld(vec3& out, GeomObject*& obj, bool& is2d) {
	obj = nullptr;
	is2d = false;
	vec3 local(0);
	if (!mesh_sel_points.IsEmpty() || !mesh_sel_lines.IsEmpty() || !mesh_sel_faces.IsEmpty()) {
		obj = GetFocusedMeshObject();
		if (!obj)
			return false;
		if (!GetMeshSelectionCenter(local))
			return false;
		is2d = false;
	}
	else if (!select_2d_shapes.IsEmpty()) {
		obj = GetFocused2DObject();
		if (!obj)
			return false;
		if (!Get2DSelectionCenter(local))
			return false;
		is2d = true;
	}
	else {
		return false;
	}
	vec3 pos = vec3(0);
	quat ori = Identity<quat>();
	vec3 scale = vec3(1);
	if (state) {
		if (const GeomObjectState* os = state->FindObjectStateByKey(obj->key)) {
			pos = os->position;
			ori = os->orientation;
			scale = os->scale;
		}
	}
	else if (GeomTransform* tr = obj->FindTransform()) {
		pos = tr->position;
		ori = tr->orientation;
		scale = tr->scale;
	}
	vec3 scaled(local[0] * scale[0], local[1] * scale[1], local[2] * scale[2]);
	out = pos + VectorTransform(scaled, ori);
	return true;
}

void Edit3D::ApplyMeshSelectionDelta(const vec3& delta) {
	if (fabs(delta[0]) < 1e-6f && fabs(delta[1]) < 1e-6f && fabs(delta[2]) < 1e-6f)
		return;
	GeomObject* obj = GetFocusedMeshObject();
	if (!obj)
		return;
	GeomEditableMesh* mesh = obj->FindEditableMesh();
	if (!mesh)
		return;
	Index<int> verts;
	for (int id : mesh_sel_points)
		verts.FindAdd(id);
	for (int id : mesh_sel_lines) {
		if (id < 0 || id >= mesh->lines.GetCount())
			continue;
		const GeomEdge& e = mesh->lines[id];
		verts.FindAdd(e.a);
		verts.FindAdd(e.b);
	}
	for (int id : mesh_sel_faces) {
		if (id < 0 || id >= mesh->faces.GetCount())
			continue;
		const GeomFace& f = mesh->faces[id];
		verts.FindAdd(f.a);
		verts.FindAdd(f.b);
		verts.FindAdd(f.c);
	}
	if (verts.IsEmpty())
		return;
	PushUndo("Move mesh selection");
	for (int id : verts) {
		if (id < 0 || id >= mesh->points.GetCount())
			continue;
		mesh->points[id] += delta;
	}
	mesh_sel_offset += delta;
	AutoKeyMeshEdit(obj);
	RefrehRenderers();
}

void Edit3D::ApplyMeshSelectionRotate(float angle_rad, const vec3& axis_local) {
	if (fabs(angle_rad) < 1e-6f)
		return;
	GeomObject* obj = GetFocusedMeshObject();
	if (!obj)
		return;
	GeomEditableMesh* mesh = obj->FindEditableMesh();
	if (!mesh)
		return;
	Index<int> verts;
	for (int id : mesh_sel_points)
		verts.FindAdd(id);
	for (int id : mesh_sel_lines) {
		if (id < 0 || id >= mesh->lines.GetCount())
			continue;
		const GeomEdge& e = mesh->lines[id];
		verts.FindAdd(e.a);
		verts.FindAdd(e.b);
	}
	for (int id : mesh_sel_faces) {
		if (id < 0 || id >= mesh->faces.GetCount())
			continue;
		const GeomFace& f = mesh->faces[id];
		verts.FindAdd(f.a);
		verts.FindAdd(f.b);
		verts.FindAdd(f.c);
	}
	if (verts.IsEmpty())
		return;
	vec3 center;
	if (!GetMeshSelectionCenter(center))
		return;
	vec3 axis = safe_normalize(axis_local);
	PushUndo("Rotate mesh selection");
	quat rot = AxisAngleQuat(axis, angle_rad);
	for (int id : verts) {
		if (id < 0 || id >= mesh->points.GetCount())
			continue;
		vec3 p = mesh->points[id] - center;
		mesh->points[id] = center + VectorTransform(p, rot);
	}
	AutoKeyMeshEdit(obj);
	RefrehRenderers();
}

void Edit3D::ApplyMeshSelectionScale(float factor) {
	if (fabs(factor - 1.0f) < 1e-6f || factor <= 0.0f)
		return;
	GeomObject* obj = GetFocusedMeshObject();
	if (!obj)
		return;
	GeomEditableMesh* mesh = obj->FindEditableMesh();
	if (!mesh)
		return;
	Index<int> verts;
	for (int id : mesh_sel_points)
		verts.FindAdd(id);
	for (int id : mesh_sel_lines) {
		if (id < 0 || id >= mesh->lines.GetCount())
			continue;
		const GeomEdge& e = mesh->lines[id];
		verts.FindAdd(e.a);
		verts.FindAdd(e.b);
	}
	for (int id : mesh_sel_faces) {
		if (id < 0 || id >= mesh->faces.GetCount())
			continue;
		const GeomFace& f = mesh->faces[id];
		verts.FindAdd(f.a);
		verts.FindAdd(f.b);
		verts.FindAdd(f.c);
	}
	if (verts.IsEmpty())
		return;
	vec3 center;
	if (!GetMeshSelectionCenter(center))
		return;
	PushUndo("Scale mesh selection");
	for (int id : verts) {
		if (id < 0 || id >= mesh->points.GetCount())
			continue;
		vec3 p = mesh->points[id] - center;
		mesh->points[id] = center + p * factor;
	}
	AutoKeyMeshEdit(obj);
	RefrehRenderers();
}

void Edit3D::Apply2DSelectionDelta(const vec2& delta) {
	if (fabs(delta[0]) < 1e-6f && fabs(delta[1]) < 1e-6f)
		return;
	GeomObject* obj = GetFocused2DObject();
	if (!obj)
		return;
	Geom2DLayer* layer = obj->Find2DLayer();
	if (!layer || select_2d_shapes.IsEmpty())
		return;
	PushUndo("Move 2D selection");
	for (int id : select_2d_shapes) {
		if (id < 0 || id >= layer->shapes.GetCount())
			continue;
		Translate2DShape(layer->shapes[id], delta);
	}
	sel2d_offset += delta;
	AutoKey2DEdit(obj);
	RefrehRenderers();
}

void Edit3D::ClearMeshSelection() {
	mesh_sel_points.Clear();
	mesh_sel_lines.Clear();
	mesh_sel_faces.Clear();
	mesh_sel_offset = vec3(0);
}

void Edit3D::ToggleMeshPoint(int idx) {
	int pos = FindIndex(mesh_sel_points, idx);
	if (pos >= 0)
		mesh_sel_points.Remove(pos);
	else
		mesh_sel_points.Add(idx);
}

void Edit3D::ToggleMeshLine(int idx) {
	int pos = FindIndex(mesh_sel_lines, idx);
	if (pos >= 0)
		mesh_sel_lines.Remove(pos);
	else
		mesh_sel_lines.Add(idx);
}

void Edit3D::ToggleMeshFace(int idx) {
	int pos = FindIndex(mesh_sel_faces, idx);
	if (pos >= 0)
		mesh_sel_faces.Remove(pos);
	else
		mesh_sel_faces.Add(idx);
}

void Edit3D::SelectMeshLoop() {
	GeomObject* obj = GetFocusedMeshObject();
	if (!obj)
		return;
	GeomEditableMesh* mesh = obj->FindEditableMesh();
	if (!mesh || mesh_sel_lines.IsEmpty())
		return;
	Index<int> out;
	Vector<int> stack;
	stack.Append(mesh_sel_lines);
	while (!stack.IsEmpty()) {
		int id = stack.Top();
		stack.Drop();
		if (out.Find(id) >= 0)
			continue;
		out.Add(id);
		const GeomEdge& e = mesh->lines[id];
		for (int i = 0; i < mesh->lines.GetCount(); i++) {
			if (out.Find(i) >= 0)
				continue;
			const GeomEdge& o = mesh->lines[i];
			if (o.a == e.a || o.b == e.a || o.a == e.b || o.b == e.b)
				stack.Add(i);
		}
	}
	mesh_sel_lines = out.PickKeys();
	RefrehRenderers();
}

void Edit3D::SelectMeshRing() {
	GeomObject* obj = GetFocusedMeshObject();
	if (!obj)
		return;
	GeomEditableMesh* mesh = obj->FindEditableMesh();
	if (!mesh || mesh_sel_lines.IsEmpty())
		return;
	Index<int> out;
	for (int id : mesh_sel_lines) {
		if (id < 0 || id >= mesh->lines.GetCount())
			continue;
		const GeomEdge& e = mesh->lines[id];
		for (int fi = 0; fi < mesh->faces.GetCount(); fi++) {
			const GeomFace& f = mesh->faces[fi];
			int verts[3] = {f.a, f.b, f.c};
			int match = 0;
			for (int v : verts) {
				if (v == e.a || v == e.b)
					match++;
			}
			if (match == 2) {
				int other = -1;
				for (int v : verts)
					if (v != e.a && v != e.b)
						other = v;
				if (other >= 0) {
					for (int li = 0; li < mesh->lines.GetCount(); li++) {
						const GeomEdge& le = mesh->lines[li];
						if ((le.a == e.a && le.b == other) || (le.b == e.a && le.a == other) ||
						    (le.a == e.b && le.b == other) || (le.b == e.b && le.a == other))
							out.FindAdd(li);
					}
				}
			}
		}
	}
	mesh_sel_lines = out.PickKeys();
	RefrehRenderers();
}

void Edit3D::ExpandMeshSelection() {
	GeomObject* obj = GetFocusedMeshObject();
	if (!obj)
		return;
	GeomEditableMesh* mesh = obj->FindEditableMesh();
	if (!mesh)
		return;
	if (mesh_select_mode == MESHSEL_VERTEX) {
		Index<int> out;
		for (int id : mesh_sel_points)
			out.FindAdd(id);
		for (int id : mesh_sel_points) {
			for (const GeomEdge& e : mesh->lines) {
				if (e.a == id)
					out.FindAdd(e.b);
				else if (e.b == id)
					out.FindAdd(e.a);
			}
		}
		mesh_sel_points = out.PickKeys();
	}
	else if (mesh_select_mode == MESHSEL_EDGE) {
		Index<int> out;
		for (int id : mesh_sel_lines)
			out.FindAdd(id);
		for (int id : mesh_sel_lines) {
			if (id < 0 || id >= mesh->lines.GetCount())
				continue;
			const GeomEdge& e = mesh->lines[id];
			for (int i = 0; i < mesh->lines.GetCount(); i++) {
				const GeomEdge& o = mesh->lines[i];
				if (o.a == e.a || o.b == e.a || o.a == e.b || o.b == e.b)
					out.FindAdd(i);
			}
		}
		mesh_sel_lines = out.PickKeys();
	}
	else if (mesh_select_mode == MESHSEL_FACE) {
		Index<int> out;
		for (int id : mesh_sel_faces)
			out.FindAdd(id);
		for (int id : mesh_sel_faces) {
			if (id < 0 || id >= mesh->faces.GetCount())
				continue;
			const GeomFace& f = mesh->faces[id];
			for (int i = 0; i < mesh->faces.GetCount(); i++) {
				const GeomFace& o = mesh->faces[i];
				int shared = 0;
				if (o.a == f.a || o.a == f.b || o.a == f.c) shared++;
				if (o.b == f.a || o.b == f.b || o.b == f.c) shared++;
				if (o.c == f.a || o.c == f.b || o.c == f.c) shared++;
				if (shared >= 2)
					out.FindAdd(i);
			}
		}
		mesh_sel_faces = out.PickKeys();
	}
	RefrehRenderers();
}

void Edit3D::ContractMeshSelection() {
	GeomObject* obj = GetFocusedMeshObject();
	if (!obj)
		return;
	GeomEditableMesh* mesh = obj->FindEditableMesh();
	if (!mesh)
		return;
	if (mesh_select_mode == MESHSEL_VERTEX) {
		Index<int> keep;
		for (int id : mesh_sel_points) {
			bool all = true;
			for (const GeomEdge& e : mesh->lines) {
				if (e.a == id && FindIndex(mesh_sel_points, e.b) < 0) all = false;
				if (e.b == id && FindIndex(mesh_sel_points, e.a) < 0) all = false;
			}
			if (all)
				keep.Add(id);
		}
		mesh_sel_points = keep.PickKeys();
	}
	else if (mesh_select_mode == MESHSEL_EDGE) {
		Index<int> keep;
		for (int id : mesh_sel_lines) {
			const GeomEdge& e = mesh->lines[id];
			bool all = true;
			for (int i = 0; i < mesh->lines.GetCount(); i++) {
				const GeomEdge& o = mesh->lines[i];
				if (o.a == e.a || o.b == e.a || o.a == e.b || o.b == e.b) {
					if (FindIndex(mesh_sel_lines, i) < 0)
						all = false;
				}
			}
			if (all)
				keep.Add(id);
		}
		mesh_sel_lines = keep.PickKeys();
	}
	else if (mesh_select_mode == MESHSEL_FACE) {
		Index<int> keep;
		for (int id : mesh_sel_faces) {
			const GeomFace& f = mesh->faces[id];
			bool all = true;
			for (int i = 0; i < mesh->faces.GetCount(); i++) {
				const GeomFace& o = mesh->faces[i];
				int shared = 0;
				if (o.a == f.a || o.a == f.b || o.a == f.c) shared++;
				if (o.b == f.a || o.b == f.b || o.b == f.c) shared++;
				if (o.c == f.a || o.c == f.b || o.c == f.c) shared++;
				if (shared >= 2 && FindIndex(mesh_sel_faces, i) < 0)
					all = false;
			}
			if (all)
				keep.Add(id);
		}
		mesh_sel_faces = keep.PickKeys();
	}
	RefrehRenderers();
}

void Edit3D::ExtrudeMeshSelection(double amount) {
	GeomObject* obj = GetFocusedMeshObject();
	if (!obj)
		return;
	GeomEditableMesh* mesh = obj->FindEditableMesh();
	if (!mesh)
		return;
	Index<int> faces;
	for (int id : mesh_sel_faces)
		faces.FindAdd(id);
	if (faces.IsEmpty()) {
		for (int id : mesh_sel_lines) {
			for (int i = 0; i < mesh->faces.GetCount(); i++) {
				const GeomFace& f = mesh->faces[i];
				if ((f.a == mesh->lines[id].a || f.a == mesh->lines[id].b ||
				     f.b == mesh->lines[id].a || f.b == mesh->lines[id].b ||
				     f.c == mesh->lines[id].a || f.c == mesh->lines[id].b))
					faces.FindAdd(i);
			}
		}
	}
	if (faces.IsEmpty()) {
		for (int id : mesh_sel_points) {
			for (int i = 0; i < mesh->faces.GetCount(); i++) {
				const GeomFace& f = mesh->faces[i];
				if (f.a == id || f.b == id || f.c == id)
					faces.FindAdd(i);
			}
		}
	}
	if (faces.IsEmpty())
		return;
	PushUndo("Extrude mesh");
	for (int fi = 0; fi < faces.GetCount(); fi++) {
		int id = faces[fi];
		if (id < 0 || id >= mesh->faces.GetCount())
			continue;
		GeomFace f = mesh->faces[id];
		vec3 a = mesh->points[f.a];
		vec3 b = mesh->points[f.b];
		vec3 c = mesh->points[f.c];
		vec3 n = Cross(b - a, c - a);
		float len = n.GetLength();
		if (len > 1e-6f)
			n /= len;
		int a2 = mesh->points.GetCount(); mesh->points.Add(a + n * (float)amount);
		int b2 = mesh->points.GetCount(); mesh->points.Add(b + n * (float)amount);
		int c2 = mesh->points.GetCount(); mesh->points.Add(c + n * (float)amount);
		GeomFace top; top.a = a2; top.b = b2; top.c = c2;
		mesh->faces.Add(top);
		GeomFace s1; s1.a = f.a; s1.b = f.b; s1.c = b2; mesh->faces.Add(s1);
		GeomFace s2; s2.a = f.a; s2.b = b2; s2.c = a2; mesh->faces.Add(s2);
		GeomFace s3; s3.a = f.b; s3.b = f.c; s3.c = c2; mesh->faces.Add(s3);
		GeomFace s4; s4.a = f.b; s4.b = c2; s4.c = b2; mesh->faces.Add(s4);
		GeomFace s5; s5.a = f.c; s5.b = f.a; s5.c = a2; mesh->faces.Add(s5);
		GeomFace s6; s6.a = f.c; s6.b = a2; s6.c = c2; mesh->faces.Add(s6);
	}
	state->UpdateObjects();
	AutoKeyMeshEdit(obj);
	RefrehRenderers();
}

void Edit3D::InsetMeshSelection(double amount) {
	GeomObject* obj = GetFocusedMeshObject();
	if (!obj)
		return;
	GeomEditableMesh* mesh = obj->FindEditableMesh();
	if (!mesh || mesh_sel_faces.IsEmpty())
		return;
	PushUndo("Inset mesh");
	for (int id : mesh_sel_faces) {
		if (id < 0 || id >= mesh->faces.GetCount())
			continue;
		GeomFace f = mesh->faces[id];
		vec3 a = mesh->points[f.a];
		vec3 b = mesh->points[f.b];
		vec3 c = mesh->points[f.c];
		vec3 center = (a + b + c) / 3.0f;
		int a2 = mesh->points.GetCount(); mesh->points.Add(a + (center - a) * (float)amount);
		int b2 = mesh->points.GetCount(); mesh->points.Add(b + (center - b) * (float)amount);
		int c2 = mesh->points.GetCount(); mesh->points.Add(c + (center - c) * (float)amount);
		GeomFace inner; inner.a = a2; inner.b = b2; inner.c = c2;
		mesh->faces.Add(inner);
		GeomFace s1; s1.a = f.a; s1.b = f.b; s1.c = b2; mesh->faces.Add(s1);
		GeomFace s2; s2.a = f.a; s2.b = b2; s2.c = a2; mesh->faces.Add(s2);
		GeomFace s3; s3.a = f.b; s3.b = f.c; s3.c = c2; mesh->faces.Add(s3);
		GeomFace s4; s4.a = f.b; s4.b = c2; s4.c = b2; mesh->faces.Add(s4);
		GeomFace s5; s5.a = f.c; s5.b = f.a; s5.c = a2; mesh->faces.Add(s5);
		GeomFace s6; s6.a = f.c; s6.b = a2; s6.c = c2; mesh->faces.Add(s6);
	}
	state->UpdateObjects();
	AutoKeyMeshEdit(obj);
	RefrehRenderers();
}

static vec3 GetSpinAxis(Edit3D& e, GeomObject& obj) {
	vec3 axis = vec3(0, 0, 1);
	switch (e.edit_plane) {
	case Edit3D::PLANE_XY: axis = vec3(0, 0, 1); break;
	case Edit3D::PLANE_XZ: axis = vec3(0, 1, 0); break;
	case Edit3D::PLANE_YZ: axis = vec3(1, 0, 0); break;
	case Edit3D::PLANE_LOCAL: {
		GeomTransform* tr = obj.FindTransform();
		if (tr)
			axis = VectorTransform(VEC_FWD, tr->orientation);
		break;
	}
	default: break;
	}
	float len = axis.GetLength();
	if (len > 1e-6f)
		axis /= len;
	return axis;
}

void Edit3D::SpinMeshSelection() {
	GeomObject* obj = GetFocusedMeshObject();
	if (!obj)
		return;
	GeomEditableMesh* mesh = obj->FindEditableMesh();
	if (!mesh)
		return;
	Index<int> verts;
	for (int id : mesh_sel_points)
		verts.FindAdd(id);
	for (int id : mesh_sel_lines) {
		if (id < 0 || id >= mesh->lines.GetCount())
			continue;
		const GeomEdge& e = mesh->lines[id];
		verts.FindAdd(e.a);
		verts.FindAdd(e.b);
	}
	for (int id : mesh_sel_faces) {
		if (id < 0 || id >= mesh->faces.GetCount())
			continue;
		const GeomFace& f = mesh->faces[id];
		verts.FindAdd(f.a);
		verts.FindAdd(f.b);
		verts.FindAdd(f.c);
	}
	if (verts.IsEmpty())
		return;
	PushUndo("Spin mesh");
	vec3 center(0, 0, 0);
	for (int id : verts)
		center += mesh->points[id];
	center /= (float)verts.GetCount();
	vec3 axis = GetSpinAxis(*this, *obj);
	int steps = 8;
	float angle = (float)(2.0 * M_PI / steps);
	Vector<int> prev_map;
	prev_map.SetCount(mesh->points.GetCount(), -1);
	for (int id : verts)
		prev_map[id] = id;
	for (int step = 1; step <= steps; step++) {
		quat rot = AxesQuat(axis * angle * step);
		Vector<int> cur_map;
		cur_map.SetCount(mesh->points.GetCount() + verts.GetCount(), -1);
		for (int id : verts) {
			vec3 p = mesh->points[id];
			vec3 r = VectorTransform(p - center, rot) + center;
			int idx = mesh->points.GetCount();
			mesh->points.Add(r);
			cur_map[id] = idx;
			if (prev_map[id] >= 0) {
				GeomEdge e;
				e.a = prev_map[id];
				e.b = idx;
				mesh->lines.Add(e);
			}
		}
		for (int id : mesh_sel_lines) {
			if (id < 0 || id >= mesh->lines.GetCount())
				continue;
			const GeomEdge& e = mesh->lines[id];
			if (cur_map[e.a] >= 0 && cur_map[e.b] >= 0) {
				GeomEdge ne;
				ne.a = cur_map[e.a];
				ne.b = cur_map[e.b];
				mesh->lines.Add(ne);
			}
		}
		prev_map = pick(cur_map);
	}
	state->UpdateObjects();
	AutoKeyMeshEdit(obj);
	RefrehRenderers();
}

void Edit3D::ScrewMeshSelection() {
	GeomObject* obj = GetFocusedMeshObject();
	if (!obj)
		return;
	GeomEditableMesh* mesh = obj->FindEditableMesh();
	if (!mesh)
		return;
	Index<int> verts;
	for (int id : mesh_sel_points)
		verts.FindAdd(id);
	for (int id : mesh_sel_lines) {
		if (id < 0 || id >= mesh->lines.GetCount())
			continue;
		const GeomEdge& e = mesh->lines[id];
		verts.FindAdd(e.a);
		verts.FindAdd(e.b);
	}
	if (verts.IsEmpty())
		return;
	PushUndo("Screw mesh");
	vec3 center(0, 0, 0);
	for (int id : verts)
		center += mesh->points[id];
	center /= (float)verts.GetCount();
	vec3 axis = GetSpinAxis(*this, *obj);
	int steps = 12;
	float angle = (float)(2.0 * M_PI / steps);
	float step_offset = 0.1f;
	Vector<int> prev_map;
	prev_map.SetCount(mesh->points.GetCount(), -1);
	for (int id : verts)
		prev_map[id] = id;
	for (int step = 1; step <= steps; step++) {
		quat rot = AxesQuat(axis * angle * step);
		Vector<int> cur_map;
		cur_map.SetCount(mesh->points.GetCount() + verts.GetCount(), -1);
		for (int id : verts) {
			vec3 p = mesh->points[id];
			vec3 r = VectorTransform(p - center, rot) + center + axis * (step_offset * step);
			int idx = mesh->points.GetCount();
			mesh->points.Add(r);
			cur_map[id] = idx;
			if (prev_map[id] >= 0) {
				GeomEdge e;
				e.a = prev_map[id];
				e.b = idx;
				mesh->lines.Add(e);
			}
		}
		for (int id : mesh_sel_lines) {
			if (id < 0 || id >= mesh->lines.GetCount())
				continue;
			const GeomEdge& e = mesh->lines[id];
			if (cur_map[e.a] >= 0 && cur_map[e.b] >= 0) {
				GeomEdge ne;
				ne.a = cur_map[e.a];
				ne.b = cur_map[e.b];
				mesh->lines.Add(ne);
			}
		}
		prev_map = pick(cur_map);
	}
	state->UpdateObjects();
	AutoKeyMeshEdit(obj);
	RefrehRenderers();
}

void Edit3D::AddMeshKeyframeAtCursor() {
	GeomObject* obj = GetFocusedMeshObject();
	if (!obj)
		return;
	GeomEditableMesh* mesh = obj->FindEditableMesh();
	if (!mesh)
		return;
	GeomMeshAnimation& anim = obj->GetMeshAnimation();
	int frame = this->anim ? this->anim->position : 0;
	GeomMeshKeyframe& kf = anim.GetAddKeyframe(frame);
	kf.frame_id = frame;
	kf.points.SetCount(mesh->points.GetCount());
	for (int i = 0; i < mesh->points.GetCount(); i++)
		kf.points[i] = mesh->points[i];
	RefrehRenderers();
}

void Edit3D::ClearMeshKeyframes() {
	GeomObject* obj = GetFocusedMeshObject();
	if (!obj)
		return;
	if (GeomMeshAnimation* anim = obj->FindMeshAnimation())
		anim->keyframes.Clear();
	RefrehRenderers();
}

void Edit3D::Add2DKeyframeAtCursor() {
	GeomObject* obj = GetFocused2DObject();
	if (!obj)
		return;
	Geom2DLayer* layer = obj->Find2DLayer();
	if (!layer)
		return;
	Geom2DAnimation& anim = obj->Get2DAnimation();
	int frame = this->anim ? this->anim->position : 0;
	Geom2DKeyframe& kf = anim.GetAddKeyframe(frame);
	kf.frame_id = frame;
	kf.shapes.SetCount(layer->shapes.GetCount());
	for (int i = 0; i < layer->shapes.GetCount(); i++) {
		const Geom2DShape& s = layer->shapes[i];
		Geom2DShape& d = kf.shapes[i];
		d.type = s.type;
		d.radius = s.radius;
		d.stroke = s.stroke;
		d.width = s.width;
		d.closed = s.closed;
		d.points.SetCount(s.points.GetCount());
		for (int k = 0; k < s.points.GetCount(); k++)
			d.points[k] = s.points[k];
	}
	RefrehRenderers();
}

void Edit3D::Clear2DKeyframes() {
	GeomObject* obj = GetFocused2DObject();
	if (!obj)
		return;
	if (Geom2DAnimation* anim = obj->Find2DAnimation())
		anim->keyframes.Clear();
	RefrehRenderers();
}

bool Edit3D::ApplyTransformDelta(GeomObject* obj, const vec3& delta, bool local_axes) {
	if (!obj || obj->is_locked)
		return false;
	PushUndo("Transform move");
	GeomTransform* tr = obj->FindTransform();
	if (!tr)
		return false;
	vec3 delta_world = delta;
	if (local_axes)
		delta_world = VectorTransform(delta, tr->orientation);
	vec3 pos = tr->position + delta_world;
	if (transform_snap_enable && transform_snap_step > 1e-12) {
		double s = transform_snap_step;
		auto snap = [&](double v) { return (float)(floor(v / s + 0.5) * s); };
		pos[0] = snap(pos[0]);
		pos[1] = snap(pos[1]);
		pos[2] = snap(pos[2]);
	}
	tr->position = pos;
	if (state)
		state->UpdateObjects();
	MaybeAutoKeyTransform(obj, true, false);
	v0.SyncPropsValues();
	RefrehRenderers();
	return true;
}

bool Edit3D::ApplyTransformRotation(GeomObject* obj, int axis, double angle_rad, bool local_axes) {
	if (!obj || obj->is_locked)
		return false;
	PushUndo("Transform rotate");
	GeomTransform* tr = obj->FindTransform();
	if (!tr)
		return false;
	quat rot = Identity<quat>();
	if (axis == 0)
		rot = MatQuat(XRotation((float)angle_rad));
	else if (axis == 1)
		rot = MatQuat(YRotation((float)angle_rad));
	else if (axis == 2)
		rot = MatQuat(ZRotation((float)angle_rad));
	else
		return false;
	if (local_axes)
		tr->orientation = tr->orientation * rot;
	else
		tr->orientation = rot * tr->orientation;
	if (transform_angle_snap && transform_angle_step > 1e-12) {
		vec3 axes = GetQuatAxes(tr->orientation);
		double s = transform_angle_step;
		auto snap = [&](double v) { return (float)(floor(v / s + 0.5) * s); };
		axes[0] = snap(axes[0]);
		axes[1] = snap(axes[1]);
		axes[2] = snap(axes[2]);
		tr->orientation = AxesQuat(axes);
	}
	if (state)
		state->UpdateObjects();
	MaybeAutoKeyTransform(obj, false, true);
	v0.SyncPropsValues();
	RefrehRenderers();
	return true;
}

void Edit3D::MaybeAutoKeyTransform(GeomObject* obj, bool pos, bool ori) {
	if (!obj || !auto_key || !anim)
		return;
	if (timeline_object_key != obj->key)
		return;
	if (!(timeline_component == TC_TRANSFORM || timeline_component == TC_NONE))
		return;
	if (pos && !auto_key_position)
		pos = false;
	if (ori && !auto_key_orientation)
		ori = false;
	if (!pos && !ori)
		return;
	GeomTransform* tr = obj->FindTransform();
	if (!tr)
		return;
	GeomTimeline& tl = obj->GetTimeline();
	int frame = anim->position;
	int idx = tl.keypoints.Find(frame);
	bool existed = idx >= 0;
	GeomKeypoint& kp = existed ? tl.keypoints[idx] : tl.GetAddKeypoint(frame);
	if (!existed) {
		kp.has_position = false;
		kp.has_orientation = false;
	}
	if (pos) {
		kp.position = tr->position;
		kp.has_position = true;
	}
	if (ori) {
		kp.orientation = tr->orientation;
		kp.has_orientation = true;
	}
	obj->read_enabled = true;
	v0.TimelineData();
}

void Edit3D::AutoKeyMeshEdit(GeomObject* obj) {
	if (!obj || !auto_key || !auto_key_mesh)
		return;
	if (timeline_object_key != obj->key)
		return;
	if (!(timeline_component == TC_MESH || timeline_component == TC_NONE))
		return;
	GeomEditableMesh* mesh = obj->FindEditableMesh();
	if (!mesh)
		return;
	GeomMeshAnimation& anim = obj->GetMeshAnimation();
	int frame = this->anim ? this->anim->position : 0;
	GeomMeshKeyframe& kf = anim.GetAddKeyframe(frame);
	kf.frame_id = frame;
	kf.points.SetCount(mesh->points.GetCount());
	for (int i = 0; i < mesh->points.GetCount(); i++)
		kf.points[i] = mesh->points[i];
	v0.TimelineData();
}

void Edit3D::AutoKey2DEdit(GeomObject* obj) {
	if (!obj || !auto_key || !auto_key_2d)
		return;
	if (timeline_object_key != obj->key)
		return;
	if (!(timeline_component == TC_2D || timeline_component == TC_NONE))
		return;
	Geom2DLayer* layer = obj->Find2DLayer();
	if (!layer)
		return;
	Geom2DAnimation& anim = obj->Get2DAnimation();
	int frame = this->anim ? this->anim->position : 0;
	Geom2DKeyframe& kf = anim.GetAddKeyframe(frame);
	kf.frame_id = frame;
	kf.shapes.SetCount(layer->shapes.GetCount());
	for (int i = 0; i < layer->shapes.GetCount(); i++) {
		const Geom2DShape& s = layer->shapes[i];
		Geom2DShape& d = kf.shapes[i];
		d.type = s.type;
		d.radius = s.radius;
		d.stroke = s.stroke;
		d.width = s.width;
		d.closed = s.closed;
		d.points.SetCount(s.points.GetCount());
		for (int k = 0; k < s.points.GetCount(); k++)
			d.points[k] = s.points[k];
	}
	v0.TimelineData();
}

bool Edit3D::ScreenToPlaneWorldPoint(int view_i, const Point& p, const vec3& origin, const vec3& normal, vec3& out) const {
	if (view_i < 0 || view_i >= 4)
		return false;
	EditRendererBase* rend = v0.rends[view_i];
	if (!rend)
		return false;
	Size sz = rend->GetSize();
	if (sz.cx <= 0 || sz.cy <= 0)
		return false;
	GeomCamera& gcam = rend->GetGeomCamera();
	Camera cam;
	gcam.LoadCamera(rend->view_mode, cam, sz);
	mat4 view = cam.GetWorldMatrix();
	mat4 proj = cam.GetProjectionMatrix();
	vec2 viewport_origin(0, 0);
	vec2 viewport_size((float)sz.cx, (float)sz.cy);
	vec3 pnear = Unproject(vec3(p.x, p.y, 0.0f), viewport_origin, viewport_size, view, proj);
	vec3 pfar = Unproject(vec3(p.x, p.y, 1.0f), viewport_origin, viewport_size, view, proj);
	vec3 dir = pfar - pnear;
	float denom = Dot(dir, normal);
	if (fabs(denom) < 1e-6)
		return false;
	float t = Dot(origin - pnear, normal) / denom;
	out = pnear + dir * t;
	return true;
}

bool Edit3D::ScreenToRay(int view_i, const Point& p, vec3& out_origin, vec3& out_dir) const {
	if (view_i < 0 || view_i >= 4)
		return false;
	EditRendererBase* rend = v0.rends[view_i];
	if (!rend)
		return false;
	Size sz = rend->GetSize();
	if (sz.cx <= 0 || sz.cy <= 0)
		return false;
	GeomCamera& gcam = rend->GetGeomCamera();
	Camera cam;
	gcam.LoadCamera(rend->view_mode, cam, sz);
	mat4 view = cam.GetWorldMatrix();
	mat4 proj = cam.GetProjectionMatrix();
	vec2 viewport_origin(0, 0);
	vec2 viewport_size((float)sz.cx, (float)sz.cy);
	vec3 pnear = Unproject(vec3(p.x, p.y, 0.0f), viewport_origin, viewport_size, view, proj);
	vec3 pfar = Unproject(vec3(p.x, p.y, 1.0f), viewport_origin, viewport_size, view, proj);
	vec3 dir = pfar - pnear;
	if (dir.GetLength() <= 1e-6f)
		return false;
	out_origin = pnear;
	out_dir = dir;
	out_dir.Normalize();
	return true;
}

void Edit3D::CreateSkeletonForSelected() {
	if (!v0.selected_obj)
		return;
	GeomObject& obj = *v0.selected_obj;
	GeomSkeleton* sk = obj.FindSkeleton();
	if (!sk) {
		sk = &obj.GetSkeleton();
		sk->name = "Skeleton";
		sk->val.id = "skeleton";
	}
	if (sk->val.sub.IsEmpty()) {
		VfsValue& n = sk->val.Add("root", AsTypeHash<GeomBone>());
		GeomBone& b = n.GetExt<GeomBone>();
		b.name = "root";
		b.position = vec3(0, 0, 0.3f);
	}
	state->UpdateObjects();
	RefreshData();
}

void Edit3D::AddBoneToSelectedSkeleton() {
	if (!v0.selected_obj)
		return;
	GeomObject& obj = *v0.selected_obj;
	GeomSkeleton* sk = obj.FindSkeleton();
	if (!sk) {
		CreateSkeletonForSelected();
		sk = obj.FindSkeleton();
		if (!sk)
			return;
	}
	VfsValue* parent = &sk->val;
	if (selected_bone && selected_bone->owner)
		parent = selected_bone;
	auto exists = [&](const String& name) {
		for (auto& s : parent->sub) {
			if (IsVfsType(s, AsTypeHash<GeomBone>())) {
				GeomBone& b = s.GetExt<GeomBone>();
				String n = b.name.IsEmpty() ? s.id : b.name;
				if (n == name)
					return true;
			}
		}
		return false;
	};
	String base = "bone";
	String name = base;
	int idx = 1;
	while (exists(name))
		name = base + "_" + IntStr(idx++);
	VfsValue& n = parent->Add(name, AsTypeHash<GeomBone>());
	GeomBone& b = n.GetExt<GeomBone>();
	b.name = name;
	b.position = vec3(0, 0, 0.3f);
	state->UpdateObjects();
	RefreshData();
}

void Edit3D::RemoveSelectedBone() {
	if (!selected_bone || !selected_bone->owner)
		return;
	selected_bone->owner->Remove(selected_bone);
	selected_bone = nullptr;
	render_ctx.selected_bone = nullptr;
	state->UpdateObjects();
	RefreshData();
}

void Edit3D::SetWeightPaintMode(bool enable) {
	weight_paint_mode = enable;
	render_ctx.show_weights = weight_paint_mode;
	render_ctx.weight_bone.Clear();
	if (selected_bone && IsVfsType(*selected_bone, AsTypeHash<GeomBone>())) {
		GeomBone& bone = selected_bone->GetExt<GeomBone>();
		String name = bone.name.IsEmpty() ? selected_bone->id : bone.name;
		render_ctx.weight_bone = name;
	}
	RefrehRenderers();
}

int Edit3D::PickNearestPoint(const GeomEditableMesh& mesh, int view_i, const Point& p, double radius_px) const {
	if (!state)
		return -1;
	GeomObject* obj = v0.selected_obj;
	if (!obj)
		return -1;
	EditRendererBase* rend = v0.rends[view_i];
	if (!rend)
		return -1;
	Size sz = rend->GetSize();
	if (sz.cx <= 0 || sz.cy <= 0)
		return -1;
	GeomCamera& gcam = rend->GetGeomCamera();
	Camera cam;
	gcam.LoadCamera(rend->view_mode, cam, sz);
	mat4 view = cam.GetWorldMatrix();
	mat4 proj = cam.GetProjectionMatrix();
	vec3 obj_pos = vec3(0);
	quat obj_ori = Identity<quat>();
	vec3 obj_scale = vec3(1);
	if (const GeomObjectState* os = state->FindObjectStateByKey(obj->key)) {
		obj_pos = os->position;
		obj_ori = os->orientation;
		obj_scale = os->scale;
	}
	double best = radius_px * radius_px;
	int best_idx = -1;
	for (int i = 0; i < mesh.points.GetCount(); i++) {
		vec3 local = mesh.points[i];
		vec3 scaled(local[0] * obj_scale[0], local[1] * obj_scale[1], local[2] * obj_scale[2]);
		vec3 world = VectorTransform(scaled, obj_ori) + obj_pos;
		vec4 clip = proj * (view * world.Embed());
		if (clip[3] == 0)
			continue;
		vec3 ndc = clip.Splice() / clip[3];
		if (ndc[0] < -1 || ndc[0] > 1 || ndc[1] < -1 || ndc[1] > 1)
			continue;
		Point pt(
			(int)floor((ndc[0] + 1) * 0.5 * (float)sz.cx + 0.5f),
			(int)floor((-ndc[1] + 1) * 0.5 * (float)sz.cy + 0.5f));
		double dx = pt.x - p.x;
		double dy = pt.y - p.y;
		double d2 = dx * dx + dy * dy;
		if (d2 < best) {
			best = d2;
			best_idx = i;
		}
	}
	return best_idx;
}

int Edit3D::PickNearestLine(const GeomEditableMesh& mesh, int view_i, const Point& p, double radius_px) const {
	if (!state)
		return -1;
	GeomObject* obj = v0.selected_obj;
	if (!obj)
		return -1;
	EditRendererBase* rend = v0.rends[view_i];
	if (!rend)
		return -1;
	Size sz = rend->GetSize();
	if (sz.cx <= 0 || sz.cy <= 0)
		return -1;
	GeomCamera& gcam = rend->GetGeomCamera();
	Camera cam;
	gcam.LoadCamera(rend->view_mode, cam, sz);
	mat4 view = cam.GetWorldMatrix();
	mat4 proj = cam.GetProjectionMatrix();
	vec3 obj_pos = vec3(0);
	quat obj_ori = Identity<quat>();
	vec3 obj_scale = vec3(1);
	if (const GeomObjectState* os = state->FindObjectStateByKey(obj->key)) {
		obj_pos = os->position;
		obj_ori = os->orientation;
		obj_scale = os->scale;
	}
	auto project_point = [&](const vec3& local, Point& out) -> bool {
		vec3 scaled(local[0] * obj_scale[0], local[1] * obj_scale[1], local[2] * obj_scale[2]);
		vec3 world = VectorTransform(scaled, obj_ori) + obj_pos;
		vec4 clip = proj * (view * world.Embed());
		if (clip[3] == 0)
			return false;
		vec3 ndc = clip.Splice() / clip[3];
		if (ndc[0] < -1 || ndc[0] > 1 || ndc[1] < -1 || ndc[1] > 1)
			return false;
		out = Point(
			(int)floor((ndc[0] + 1) * 0.5 * (float)sz.cx + 0.5f),
			(int)floor((-ndc[1] + 1) * 0.5 * (float)sz.cy + 0.5f));
		return true;
	};
	auto dist2_segment = [](const Point& a, const Point& b, const Point& p) -> double {
		double ax = a.x;
		double ay = a.y;
		double bx = b.x;
		double by = b.y;
		double px = p.x;
		double py = p.y;
		double dx = bx - ax;
		double dy = by - ay;
		double len2 = dx * dx + dy * dy;
		if (len2 <= 1e-6) {
			double sx = px - ax;
			double sy = py - ay;
			return sx * sx + sy * sy;
		}
		double t = ((px - ax) * dx + (py - ay) * dy) / len2;
		if (t < 0.0) t = 0.0;
		if (t > 1.0) t = 1.0;
		double cx = ax + t * dx;
		double cy = ay + t * dy;
		double sx = px - cx;
		double sy = py - cy;
		return sx * sx + sy * sy;
	};
	double best = radius_px * radius_px;
	int best_idx = -1;
	for (int i = 0; i < mesh.lines.GetCount(); i++) {
		const GeomEdge& e = mesh.lines[i];
		if (e.a < 0 || e.b < 0 || e.a >= mesh.points.GetCount() || e.b >= mesh.points.GetCount())
			continue;
		Point a2, b2;
		if (!project_point(mesh.points[e.a], a2))
			continue;
		if (!project_point(mesh.points[e.b], b2))
			continue;
		double d2 = dist2_segment(a2, b2, p);
		if (d2 < best) {
			best = d2;
			best_idx = i;
		}
	}
	return best_idx;
}

int Edit3D::PickNearestFace(const GeomEditableMesh& mesh, int view_i, const Point& p) const {
	if (!state)
		return -1;
	GeomObject* obj = v0.selected_obj;
	if (!obj)
		return -1;
	EditRendererBase* rend = v0.rends[view_i];
	if (!rend)
		return -1;
	Size sz = rend->GetSize();
	if (sz.cx <= 0 || sz.cy <= 0)
		return -1;
	GeomCamera& gcam = rend->GetGeomCamera();
	Camera cam;
	gcam.LoadCamera(rend->view_mode, cam, sz);
	mat4 view = cam.GetWorldMatrix();
	mat4 proj = cam.GetProjectionMatrix();
	vec3 obj_pos = vec3(0);
	quat obj_ori = Identity<quat>();
	vec3 obj_scale = vec3(1);
	if (const GeomObjectState* os = state->FindObjectStateByKey(obj->key)) {
		obj_pos = os->position;
		obj_ori = os->orientation;
		obj_scale = os->scale;
	}
	auto project = [&](const vec3& local, Pointf& out, float& z) -> bool {
		vec3 scaled(local[0] * obj_scale[0], local[1] * obj_scale[1], local[2] * obj_scale[2]);
		vec3 world = VectorTransform(scaled, obj_ori) + obj_pos;
		vec4 clip = proj * (view * world.Embed());
		if (clip[3] == 0)
			return false;
		vec3 ndc = clip.Splice() / clip[3];
		if (ndc[0] < -1 || ndc[0] > 1 || ndc[1] < -1 || ndc[1] > 1)
			return false;
		out = Pointf(
			(ndc[0] + 1) * 0.5 * (float)sz.cx,
			(-ndc[1] + 1) * 0.5 * (float)sz.cy);
		z = ndc[2];
		return true;
	};
	int best = -1;
	float best_z = 1e9f;
	for (int i = 0; i < mesh.faces.GetCount(); i++) {
		const GeomFace& f = mesh.faces[i];
		if (f.a < 0 || f.b < 0 || f.c < 0 || f.a >= mesh.points.GetCount() ||
		    f.b >= mesh.points.GetCount() || f.c >= mesh.points.GetCount())
			continue;
		Pointf pa, pb, pc;
		float za, zb, zc;
		if (!project(mesh.points[f.a], pa, za) ||
		    !project(mesh.points[f.b], pb, zb) ||
		    !project(mesh.points[f.c], pc, zc))
			continue;
		Pointf pt(p.x, p.y);
		float area = (float)((pb.x - pa.x) * (pc.y - pa.y) - (pb.y - pa.y) * (pc.x - pa.x));
		if (fabs(area) < 1e-6f)
			continue;
		float w0 = (float)((pb.x - pt.x) * (pc.y - pt.y) - (pb.y - pt.y) * (pc.x - pt.x));
		float w1 = (float)((pc.x - pt.x) * (pa.y - pt.y) - (pc.y - pt.y) * (pa.x - pt.x));
		float w2 = (float)((pa.x - pt.x) * (pb.y - pt.y) - (pa.y - pt.y) * (pb.x - pt.x));
		if ((w0 >= 0 && w1 >= 0 && w2 >= 0) || (w0 <= 0 && w1 <= 0 && w2 <= 0)) {
			float z = (za + zb + zc) / 3.0f;
			if (z < best_z) {
				best_z = z;
				best = i;
			}
		}
	}
	return best;
}

bool Edit3D::HasLine(const GeomEditableMesh& mesh, int a, int b) const {
	if (a < 0 || b < 0)
		return false;
	for (const GeomEdge& e : mesh.lines) {
		if ((e.a == a && e.b == b) || (e.a == b && e.b == a))
			return true;
	}
	return false;
}

void Edit3D::RemoveEditablePoint(GeomEditableMesh& mesh, int idx) {
	if (idx < 0 || idx >= mesh.points.GetCount())
		return;
	mesh.points.Remove(idx);
	for (int i = mesh.lines.GetCount() - 1; i >= 0; i--) {
		GeomEdge& e = mesh.lines[i];
		if (e.a == idx || e.b == idx)
			mesh.lines.Remove(i);
		else {
			if (e.a > idx) e.a--;
			if (e.b > idx) e.b--;
		}
	}
	for (int i = mesh.faces.GetCount() - 1; i >= 0; i--) {
		GeomFace& f = mesh.faces[i];
		if (f.a == idx || f.b == idx || f.c == idx)
			mesh.faces.Remove(i);
		else {
			if (f.a > idx) f.a--;
			if (f.b > idx) f.b--;
			if (f.c > idx) f.c--;
		}
	}
}

void Edit3D::CreateDefaultInit() {
	
	// Cler project
	prj->Clear();
	
	// Add scene
	GeomScene& scene = prj->AddScene();
	
	
}

void Edit3D::CreateDefaultPostInit() {
	GeomScene& scene = prj->GetScene(0);
	GeomObject* cam = scene.FindCamera("camera");
	
	if (cam) {
		GeomTimeline* tl = cam->FindTimeline();
		if (tl && !tl->keypoints.IsEmpty()) {
			GeomKeypoint& kp = tl->keypoints.Get(0);
			GeomCamera& program = state->GetProgram();
			program.position = kp.position;
			program.orientation = kp.orientation;
		}
	}
	else {
		GeomCamera& program = state->GetProgram();
		program.position = vec3(0,0,0);
		program.orientation = Identity<quat>();
	}
	
	state->active_scene = 0;
	
	Data();
	v0.TimelineData();
	v0.tree.OpenDeep(v0.tree_scenes);
}

void Edit3D::LoadEmptyProject() {
	scene3d_path.Clear();
	scene3d_created.Clear();
	scene3d_modified.Clear();
	scene3d_data_dir = "data";
	scene3d_external_files.Clear();
	scene3d_meta.Clear();
	scene3d_use_json = true;
	repeat_playback = false;
	if (project_dir.IsEmpty())
		SetProjectDir(GetCurrentDirectory());
	UpdateAssetBrowser();
	script_instances.Clear();
	CreateDefaultInit();
	CreateDefaultPostInit();
	UpdateWindowTitle();
	
}

void Edit3D::LoadTestCirclingCube() {
	GeomScene& scene = prj->GetScene(0);
	GeomObject& cam = scene.GetAddCamera("camera");
	GeomObject& mdl = scene.GetAddModel("some model");
	
	ModelBuilder mb;
	mb.AddBox(0, 1, 1);
	
	mdl.mdl = mb.Detach();
	
	scene.length = prj->kps * 4 + 1;
	float step = M_PIf * 2 / 4;
	float angle = 0;
	float cam_radius = 2;
	for(int i = 0; i < 5; i++) {
		GeomKeypoint& kp = cam.GetTimeline().GetAddKeypoint(i * prj->kps);
		kp.position = vec3(sin(angle), 0, cos(angle)) * cam_radius;
		kp.orientation = AxesQuat(angle, 0, 0);
		angle += step;
	}
}

void Edit3D::LoadTestOctree() {
	GeomScene& scene = prj->GetScene(0);
	GeomObject& cam = scene.GetAddCamera("camera");
	
	// Create octree
	GeomObject& omodel = scene.GetAddOctree("octree");
	Octree& o = omodel.octree.octree;
	o.Initialize(-3, 8); // 1 << 6 = 32x32x32 meters
	
	// Create points in form of sphere
	float radius = 100;
	bool random_points = 1;
	int points = 256;
	for(int i = 0; i < points; i++) {
		float yaw, pitch;
		if (random_points) {
			yaw = Randomf() * M_PIf * 2;
			float f = (Randomf() * 2 - 1);
			pitch = f * (M_PI / 2);
		}
		else {
			float f = ((float)i / (float)points);
			yaw = f * M_PI * 2;
			pitch = 0;//fmodf(f * M_PI * 40, M_PI) - M_PI/2;
		}
		
		vec3 pos = AxesDir(yaw, pitch) * radius;
		OctreeNode* n = o.GetAddNode(pos, -3);
		Pointcloud::Point& p = n->Add<Pointcloud::Point>();
		p.SetPosition(pos);
		//LOG(pos.ToString() + ": " + HexStr(n));
	}
	
	// Move camera linearly around sphere
	int seconds = 3;
	scene.length = prj->kps * seconds;
	int kp_step = 3;
	float step = M_PIf * 2 / (scene.length / kp_step - 1);
	float angle = 0;
	float cam_radius = radius + 2;
	for(int i = 0; i < scene.length; i += kp_step) {
		GeomKeypoint& kp = cam.GetTimeline().GetAddKeypoint(i);
		kp.position = vec3(sin(angle), 0, cos(angle)) * cam_radius;
		kp.orientation = AxesQuat(angle, 0, 0);
		angle += step;
	}

}

void Edit3D::LoadTestTimelineSphere() {
	GeomScene& scene = prj->GetScene(0);
	GeomObject& cam = scene.GetAddCamera("camera");
	GeomObject& mdl = scene.GetAddModel("sphere");

	ModelBuilder mb;
	mb.AddSphere(vec3(0, 0, 0), 1.0f, 24, 16);
	mdl.mdl = mb.Detach();
	mdl.asset_ref = "preset:sphere";

	scene.length = prj->kps * 3 + 1;
	GeomTimeline& tl = mdl.GetTimeline();
	tl.keypoints.Clear();

	GeomKeypoint& kp0 = tl.GetAddKeypoint(0);
	kp0.position = vec3(-1, 0, 0);
	kp0.orientation = Identity<quat>();
	kp0.has_position = true;
	kp0.has_orientation = false;

	GeomKeypoint& kp1 = tl.GetAddKeypoint(prj->kps);
	kp1.position = vec3(0, 0, 0);
	kp1.orientation = AxesQuat(0, 0, M_PIf * 0.5f);
	kp1.has_position = false;
	kp1.has_orientation = true;

	GeomKeypoint& kp2 = tl.GetAddKeypoint(prj->kps * 2);
	kp2.position = vec3(1, 0, 0);
	kp2.orientation = AxesQuat(0, 0, M_PIf);
	kp2.has_position = true;
	kp2.has_orientation = true;

	GeomKeypoint& kp3 = cam.GetTimeline().GetAddKeypoint(0);
	kp3.position = vec3(0, 0, 4);
	kp3.orientation = Identity<quat>();
	kp3.has_position = true;
	kp3.has_orientation = true;
}

void Edit3D::LoadTestProject(int test_i) {
	script_instances.Clear();
	CreateDefaultInit();
	
	switch (test_i) {
		case 0: LoadTestCirclingCube(); break;
		case 1: LoadTestOctree(); break;
		case 2: LoadTestTimelineSphere(); break;
		case 3: LoadTestHmdPointcloud(); break;
		default: break;
	}
	
	CreateDefaultPostInit();
}

void Edit3D::LoadTestHmdPointcloud() {
	GeomScene& scene = prj->GetScene(0);
	scene.name = "HMD Pointcloud";
	scene.GetAddCamera("hmd_camera");
	scene.GetAddOctree("hmd_pointcloud");
}

void Edit3D::LoadWmrStereoPointcloud(String directory) {
	video.SetWmrCamera();
	video.LoadDirectory(directory, 30);
	Data();
	v0.TimelineData();
	v0.tree.OpenDeep(v0.tree_scenes);
	
	SetView(VIEW_VIDEOIMPORT);
}

void Edit3D::UpdateWindowTitle() {
	String format = scene3d_use_json ? "JSON" : "Binary";
	String filename;
	if (!scene3d_path.IsEmpty())
		filename = " - " + GetFileName(scene3d_path);
	Title(Format("ModelerApp - Scene3D v%d (%s)%s", SCENE3D_VERSION, format, filename));
}

void Edit3D::SetScene3DFormat(bool use_json) {
	scene3d_use_json = use_json;
	UpdateWindowTitle();
}

void Edit3D::ToggleRepeatPlayback() {
	repeat_playback = !repeat_playback;
	RefrehToolbar();
}

void Edit3D::EnsureHmdSceneObjects() {
	GeomScene& scene = state->GetActiveScene();
	GeomDirectory& fusion_room = scene.GetAddDirectory("fusion_room");
	fusion_room.GetAddCamera("hmd_camera");
	hmd_pointcloud = &fusion_room.GetAddOctree("hmd_pointcloud");
	state->UpdateObjects();
	Data();
	v0.TimelineData();
	v0.tree.OpenDeep(v0.tree_scenes);
}

void Edit3D::EnsureSimSceneObjects() {
	GeomScene& scene = state->GetActiveScene();
	for (int i = scene.val.sub.GetCount() - 1; i >= 0; i--) {
		VfsValue& n = scene.val.sub[i];
		if (!IsVfsType(n, AsTypeHash<GeomObject>()))
			continue;
		GeomObject& o = n.GetExt<GeomObject>();
		if (o.name == "sim_fake_camera" || o.name == "sim_localized_camera")
			scene.val.sub.Remove(i);
	}
	GeomDirectory& sim_raw = scene.GetAddDirectory("sim_raw_space");
	GeomDirectory& fusion_room = scene.GetAddDirectory("fusion_room");
	GeomObject& fake_cam = sim_raw.GetAddCamera("sim_fake_camera");
	GeomObject& localized_cam = fusion_room.GetAddCamera("hmd_camera");
	sim_pointcloud_obj = &scene.GetAddOctree("sim_pointcloud");
	sim_observation_obj = &sim_raw.GetAddOctree("sim_observation");
	sim_controller_obj[0] = &fusion_room.GetAddOctree("sim_controller_0");
	sim_controller_obj[1] = &fusion_room.GetAddOctree("sim_controller_1");
	sim_controller_model_obj[0] = &fusion_room.GetAddModel("sim_controller_model_0");
	sim_controller_model_obj[1] = &fusion_room.GetAddModel("sim_controller_model_1");
	sim_hmd_pointcloud_obj = &fusion_room.GetAddOctree("hmd_pointcloud");
	for (int i = 0; i < 2; i++) {
		if (!sim_controller_model_obj[i])
			continue;
		GeomObject& obj = *sim_controller_model_obj[i];
		if (!obj.mdl) {
			ModelBuilder builder;
			builder.AddBox(vec3(0, 0, 0), vec3(0.06f, 0.02f, 0.12f), true);
			obj.mdl = builder.Detach();
		}
	}
	UpdateCameraObjectRender(fake_cam, sim_fake_hmd_pose, false);
	UpdateCameraObjectRender(localized_cam, sim_localized_pose, false);
	state->UpdateObjects();
	Data();
	v0.TimelineData();
	v0.tree.OpenDeep(v0.tree_scenes);
}

void Edit3D::TogglePointcloudRecording() {
	if (record_pointcloud)
		StopPointcloudRecording();
	else
		StartPointcloudRecording();
}

void Edit3D::StartPointcloudRecording() {
	if (record_pointcloud)
		return;
	if (!hmd.Start()) {
		PromptOK("Failed to initialise HMD camera.");
		return;
	}
	hmd.ResetTracking();
	hmd.recording = true;
	record_pointcloud = true;
	EnsureHmdSceneObjects();
	if (hmd_pointcloud) {
		const Octree* octree = hmd.GetPointcloud(true);
		hmd_pointcloud->octree_ptr = octree ? const_cast<Octree*>(octree) : 0;
	}
}

void Edit3D::StopPointcloudRecording() {
	if (!record_pointcloud)
		return;
	record_pointcloud = false;
	hmd.recording = false;
	if (hmd_pointcloud)
		hmd_pointcloud->octree_ptr = 0;
	hmd.Stop();
}

void Edit3D::UpdateHmdCameraPose() {
#if defined(flagPOSIX) && defined(flagSOFTHMD)
	GeomCamera& cam = state->GetProgram();
	FusionState fs;
	if (hmd.fusion.GetState(fs)) {
		cam.position = fs.position;
		cam.orientation = fs.orientation;
		return;
	}
	HMD::SoftHmdVisualTracker& tracker = hmd.fusion.GetBrightTracker();
	if (tracker.HasPose()) {
		cam.position = tracker.GetPosition();
		cam.orientation = tracker.GetOrientation();
	}
#endif
}

void Edit3D::RunSyntheticPointcloudSimDialog() {
	String log;
	bool ok = RunSyntheticPointcloudSim(log);
	log << "\nResult: " << (ok ? "OK" : "FAIL");
	String safe = log;
	safe.Replace("[", "(");
	safe.Replace("]", ")");
	PromptOK(safe);
}

void Edit3D::DebugGeneratePointcloud() {
	sim_state = BuildSyntheticPointcloud(sim_cfg);
	sim_has_state = true;
	sim_has_obs = false;
	sim_has_ctrl_obs = false;
	sim_fake_hmd_pose = PointcloudPose::MakeIdentity();
	sim_localized_pose = PointcloudPose::MakeIdentity();
	EnsureSimSceneObjects();
	GeomScene& scene = state->GetActiveScene();
	GeomDirectory& sim_raw = scene.GetAddDirectory("sim_raw_space");
	GeomDirectory& fusion_room = scene.GetAddDirectory("fusion_room");
	GeomTransform& raw_tr = sim_raw.GetTransform();
	raw_tr.position = sim_fake_hmd_pose.position;
	raw_tr.orientation = sim_fake_hmd_pose.orientation;
	GeomTransform& fusion_tr = fusion_room.GetTransform();
	fusion_tr.position = vec3(0);
	fusion_tr.orientation = Identity<quat>();
	if (sim_pointcloud_obj) {
		FillOctree(sim_pointcloud_obj->octree.octree, sim_state.reference.points, -3);
		sim_pointcloud_obj->octree_ptr = 0;
	}
	if (sim_observation_obj) {
		sim_observation_obj->octree.octree.Initialize(-3, 8);
		sim_observation_obj->octree_ptr = 0;
	}
	for (int i = 0; i < 2; i++) {
		if (sim_controller_obj[i]) {
			sim_controller_obj[i]->octree.octree.Initialize(-3, 8);
			sim_controller_obj[i]->octree_ptr = 0;
		}
	}
	GeomCamera& focus = state->GetFocus();
	focus.position = sim_fake_hmd_pose.position;
	focus.orientation = sim_fake_hmd_pose.orientation;
	GeomCamera& program = state->GetProgram();
	program.position = sim_localized_pose.position;
	program.orientation = sim_localized_pose.orientation;
	UpdateCameraObjectRender(scene.GetAddCamera("sim_fake_camera"), sim_fake_hmd_pose, false);
	UpdateCameraObjectRender(scene.GetAddCamera("hmd_camera"), sim_localized_pose, false);
	RefrehRenderers();
}

void Edit3D::GenerateSyntheticPointcloudFor(GeomObject& obj) {
	if (!obj.IsOctree())
		return;
	SyntheticPointcloudState synth_state = BuildSyntheticPointcloud(sim_cfg);
	FillOctree(obj.octree.octree, synth_state.reference.points, -3);
	obj.octree_ptr = 0;
	state->UpdateObjects();
	v0.RefreshAll();
}

void Edit3D::DebugSimulateObservation() {
	if (!sim_has_state)
		DebugGeneratePointcloud();
	sim_fake_hmd_pose = RandomHmdPose(sim_cfg);
	sim_state.hmd_pose_world = sim_fake_hmd_pose;
	sim_obs = SimulateHmdObservation(sim_state, sim_cfg);
	sim_has_obs = true;
	EnsureSimSceneObjects();
	GeomScene& scene = state->GetActiveScene();
	GeomDirectory& sim_raw = scene.GetAddDirectory("sim_raw_space");
	GeomTransform& raw_tr = sim_raw.GetTransform();
	raw_tr.position = sim_fake_hmd_pose.position;
	raw_tr.orientation = sim_fake_hmd_pose.orientation;
	state->UpdateObjects();
	RefreshSimObservation();
	GeomCamera& cam = state->GetFocus();
	cam.position = sim_fake_hmd_pose.position;
	cam.orientation = sim_fake_hmd_pose.orientation;
	UpdateCameraObjectRender(scene.GetAddCamera("sim_fake_camera"), sim_fake_hmd_pose, false);
	RefrehRenderers();
}

String Edit3D::RunLocalizationLog(bool show_dialog) {
	if (!sim_has_state)
		DebugGeneratePointcloud();
	if (!sim_has_obs)
		DebugSimulateObservation();
	PointcloudLocalizerStub localizer;
	PointcloudLocalizationResult loc = localizer.Locate(sim_state.reference, sim_obs);
	if (loc.ok) {
		sim_localized_pose = loc.pose;
		GeomScene& scene = state->GetActiveScene();
		GeomCamera& cam = state->GetProgram();
		cam.position = sim_localized_pose.position;
		cam.orientation = sim_localized_pose.orientation;
		UpdateCameraObjectRender(scene.GetAddCamera("hmd_camera"), sim_localized_pose, false);
		if (sim_hmd_pointcloud_obj) {
			GeomPointcloudEffectTransform& fx = sim_hmd_pointcloud_obj->GetAddPointcloudEffect("localization");
			if (!fx.locked) {
				fx.position = sim_localized_pose.position;
				fx.orientation = sim_localized_pose.orientation;
			}
			fx.enabled = true;
		}
		state->UpdateObjects();
		RefreshSimObservation();
	}
	String log;
	log << "Localization: ok=" << (int)loc.ok;
	log << " pos=" << Format("(%.3f, %.3f, %.3f)",
		loc.pose.position[0], loc.pose.position[1], loc.pose.position[2]);
	log << "\n";
	String safe = log;
	safe.Replace("[", "(");
	safe.Replace("]", ")");
	if (show_dialog)
		PromptOK(safe);
	return safe;
}

void Edit3D::RefreshSimObservation() {
	if (!sim_observation_obj || !sim_has_obs)
		return;
	if (!sim_obs_octree)
		sim_obs_octree = MakeOne<Octree>();
	FillOctree(*sim_obs_octree, sim_obs.points, -3);
	sim_observation_obj->octree_ptr = sim_obs_octree.Get();
	if (sim_hmd_pointcloud_obj)
		sim_hmd_pointcloud_obj->octree_ptr = sim_obs_octree.Get();
	RefrehRenderers();
}

String Edit3D::RunControllerLocalizationLog(bool show_dialog) {
	if (!sim_has_state)
		DebugGeneratePointcloud();
	if (!sim_has_ctrl_obs)
		DebugSimulateControllerObservations();
	ControllerPatternTrackerStub tracker;
	ControllerFusionStub fusion;
	String log;
	for (int i = 0; i < sim_ctrl_obs.GetCount() && i < sim_state.controllers.GetCount(); i++) {
		vec3 center_cam = ApplyInversePoseSimple(sim_fake_hmd_pose, sim_state.controller_poses_world[i].position);
		bool visible = VisibleInFrustumSimple(center_cam, sim_cfg);
		if (!visible) {
			log << "controller[" << i << "] ok=0 pos=(0.000, 0.000, 0.000) not_visible=1\n";
			continue;
		}
		if (sim_ctrl_obs[i].points.IsEmpty()) {
			log << "controller[" << i << "] ok=0 pos=(0.000, 0.000, 0.000)\n";
			continue;
		}
		ControllerPoseResult pose = tracker.Track(sim_state.controllers[i], sim_ctrl_obs[i]);
		ControllerFusionSample sample;
		sample.has_orientation = true;
		sample.orientation = pose.pose.orientation;
		pose = fusion.Fuse(pose, sample);
		log << "controller[" << i << "] ok=" << (int)pose.ok;
		log << " pos=" << Format("(%.3f, %.3f, %.3f)",
			pose.pose.position[0], pose.pose.position[1], pose.pose.position[2]) << "\n";
	}
	String safe = log;
	safe.Replace("[", "(");
	safe.Replace("]", ")");
	if (show_dialog)
		PromptOK(safe);
	return safe;
}

void Edit3D::DebugRunLocalization() {
	RunLocalizationLog(true);
	RefrehRenderers();
}

void Edit3D::DebugRunControllerLocalization() {
	RunControllerLocalizationLog(true);
}

void Edit3D::DebugSimulateControllerObservations() {
	if (!sim_has_state)
		DebugGeneratePointcloud();
	sim_state.controller_poses_world.SetCount(2);
	sim_state.hmd_pose_world = sim_fake_hmd_pose;
	sim_state.controller_poses_world[0] = RandomControllerPoseInFrustum(sim_fake_hmd_pose, sim_cfg, 0.4f, 1.2f);
	sim_state.controller_poses_world[1] = RandomControllerPoseInFrustum(sim_fake_hmd_pose, sim_cfg, 0.4f, 1.2f);
	sim_ctrl_obs = SimulateControllerObservations(sim_state, sim_cfg);
	sim_has_ctrl_obs = true;
	EnsureSimSceneObjects();
	for (int i = 0; i < sim_ctrl_obs.GetCount() && i < 2; i++) {
		if (!sim_controller_obj[i])
			continue;
		Vector<vec3> world_points = TransformPointsToWorld(sim_state.hmd_pose_world, sim_ctrl_obs[i].points);
		FillOctree(sim_controller_obj[i]->octree.octree, world_points, -3);
		sim_controller_obj[i]->octree_ptr = 0;
		if (sim_controller_model_obj[i]) {
			GeomObject& obj = *sim_controller_model_obj[i];
			GeomTimeline& tl = obj.GetTimeline();
			tl.keypoints.Clear();
			GeomKeypoint& kp = tl.keypoints.Add(0);
			kp.position = sim_state.controller_poses_world[i].position;
			kp.orientation = sim_state.controller_poses_world[i].orientation;
			kp.has_position = true;
			kp.has_orientation = true;
		}
	}
	GeomCamera& cam = state->GetFocus();
	cam.position = sim_fake_hmd_pose.position;
	cam.orientation = sim_fake_hmd_pose.orientation;
	GeomScene& scene = state->GetActiveScene();
	UpdateCameraObjectRender(scene.GetAddCamera("sim_fake_camera"), sim_fake_hmd_pose, false);
	RefrehRenderers();
}

void Edit3D::DebugClearSynthetic() {
	sim_has_state = false;
	sim_has_obs = false;
	sim_has_ctrl_obs = false;
	sim_state.reference.Clear();
	sim_ctrl_obs.Clear();
	EnsureSimSceneObjects();
	if (sim_pointcloud_obj)
		sim_pointcloud_obj->octree.octree.Initialize(-3, 8);
	if (sim_observation_obj)
		sim_observation_obj->octree.octree.Initialize(-3, 8);
	for (int i = 0; i < 2; i++) {
		if (sim_controller_obj[i])
			sim_controller_obj[i]->octree.octree.Initialize(-3, 8);
	}
	RefrehRenderers();
}

void Edit3D::RunSyntheticSimVisual(bool log_stdout, bool verbose) {
	DebugGeneratePointcloud();
	DebugSimulateObservation();
	String log;
	log << "Synthetic sim (visual)\n";
	log << "reference points=" << sim_state.reference.points.GetCount() << "\n";
	log << "observation points=" << sim_obs.points.GetCount() << "\n";
	int obs_in_frustum = 0;
	int ref_in_range = 0;
	int ref_z_pos = 0;
	int ref_z_neg = 0;
	float min_z = 1e9f;
	float max_z = -1e9f;
	for (int i = 0; i < sim_obs.points.GetCount(); i++) {
		if (VisibleInFrustumSimple(sim_obs.points[i], sim_cfg))
			obs_in_frustum++;
	}
	for (int i = 0; i < sim_state.reference.points.GetCount(); i++) {
		vec3 cam = ApplyInversePoseSimple(sim_fake_hmd_pose, sim_state.reference.points[i]);
		float len = cam.GetLength();
		if (len <= sim_cfg.max_range)
			ref_in_range++;
		min_z = min(min_z, cam[2]);
		max_z = max(max_z, cam[2]);
		if (cam[2] >= 0)
			ref_z_pos++;
		else
			ref_z_neg++;
	}
	log << "observation in-frustum=" << obs_in_frustum << "\n";
	if (verbose) {
		log << "reference in-range=" << ref_in_range << "\n";
		log << "reference z>=0=" << ref_z_pos << " z<0=" << ref_z_neg << "\n";
		log << "reference z min=" << min_z << " max=" << max_z << "\n";
	}
	if (verbose && sim_state.reference.points.GetCount() > 0) {
		vec3 cam0 = ApplyInversePoseSimple(sim_fake_hmd_pose, sim_state.reference.points[0]);
		vec3 ref0 = sim_state.reference.points[0];
		log << "reference0 world=(" << ref0[0] << " " << ref0[1] << " " << ref0[2] << ")\n";
		log << "fake camera pos=(" << sim_fake_hmd_pose.position[0] << " "
			<< sim_fake_hmd_pose.position[1] << " " << sim_fake_hmd_pose.position[2] << ")\n";
		log << "fake camera orient=(" << sim_fake_hmd_pose.orientation[0] << " "
			<< sim_fake_hmd_pose.orientation[1] << " " << sim_fake_hmd_pose.orientation[2]
			<< " " << sim_fake_hmd_pose.orientation[3] << ")\n";
		vec3 rel0 = ref0 - sim_fake_hmd_pose.position;
		log << "reference0 rel=(" << rel0[0] << " " << rel0[1] << " " << rel0[2] << ")\n";
		vec3 cam0_inv = VectorTransform(rel0, sim_fake_hmd_pose.orientation.GetInverse());
		vec3 cam0_fwd = VectorTransform(rel0, sim_fake_hmd_pose.orientation);
		log << "reference0 cam_inv=(" << cam0_inv[0] << " " << cam0_inv[1] << " " << cam0_inv[2] << ")\n";
		log << "reference0 cam_fwd=(" << cam0_fwd[0] << " " << cam0_fwd[1] << " " << cam0_fwd[2] << ")\n";
		log << "reference0 cam=(" << cam0[0] << " " << cam0[1] << " " << cam0[2] << ")\n";
	}
	log << RunLocalizationLog(false);
	DebugSimulateControllerObservations();
	log << "controller observations=" << sim_ctrl_obs.GetCount() << "\n";
	int ctrl_points_total = 0;
	for (int i = 0; i < sim_ctrl_obs.GetCount(); i++)
		ctrl_points_total += sim_ctrl_obs[i].points.GetCount();
	log << "controller points=" << ctrl_points_total << "\n";
	int ctrl_centers_in_frustum = 0;
	for (int i = 0; i < sim_state.controller_poses_world.GetCount(); i++) {
		vec3 center_cam = ApplyInversePoseSimple(sim_fake_hmd_pose, sim_state.controller_poses_world[i].position);
		bool visible = VisibleInFrustumSimple(center_cam, sim_cfg);
		if (verbose) {
			log << "controller[" << i << "] cam=(" << center_cam[0] << " " << center_cam[1] << " " << center_cam[2]
				<< ") visible=" << (int)visible << "\n";
		}
		if (visible)
			ctrl_centers_in_frustum++;
	}
	log << "controller centers in-frustum=" << ctrl_centers_in_frustum << "\n";
	if (sim_state.controller_poses_world.GetCount() >= 2) {
		vec3 delta = sim_state.controller_poses_world[0].position - sim_state.controller_poses_world[1].position;
		if (verbose)
			log << "controller separation=" << delta.GetLength() << "\n";
	}
	if (verbose) {
		vec3 cam_to_origin = sim_fake_hmd_pose.position;
		log << "fake camera dist to origin=" << cam_to_origin.GetLength() << "\n";
	}
	log << RunControllerLocalizationLog(false);
	bool ok = !sim_obs.points.IsEmpty();
	ok = ok && (obs_in_frustum == sim_obs.points.GetCount());
	ok = ok && (ctrl_centers_in_frustum == sim_state.controller_poses_world.GetCount());
	ok = ok && (ctrl_points_total > 0);
	log << "sanity ok=" << (int)ok << "\n";
	if (log_stdout)
		Cout() << log;
}

bool Edit3D::IsScene3DBinaryPath(const String& path) const {
	String ext = ToLower(GetFileExt(path));
	if (ext == ".scene3db")
		return true;
	if (ext == ".bin") {
		String base = GetFileExt(GetFileTitle(path));
		return ToLower(base) == ".scene3d";
	}
	return false;
}

bool Edit3D::IsScene3DJsonPath(const String& path) const {
	return ToLower(GetFileExt(path)) == ".scene3d";
}

String Edit3D::EnsureScene3DExtension(const String& path, bool use_json) const {
	String ext = ToLower(GetFileExt(path));
	if (use_json) {
		if (ext == ".scene3d")
			return path;
		return AppendFileName(GetFileFolder(path), GetFileTitle(path) + ".scene3d");
	}
	if (ext == ".scene3db")
		return path;
	if (ext == ".bin" && ToLower(GetFileExt(GetFileTitle(path))) == ".scene3d")
		return path;
	return AppendFileName(GetFileFolder(path), GetFileTitle(path) + ".scene3db");
}

void Edit3D::OpenScene3D() {
	LoadScene3DWithDialog();
}

void Edit3D::OpenFilePool() {
	file_pool.Data();
	if (dock_pool) {
		ActivateDockableChild(file_pool);
		file_pool.SetFocus();
	}
}

void Edit3D::OpenAssetBrowser() {
	asset_browser.Data();
	if (dock_assets) {
		ActivateDockableChild(asset_browser);
		asset_browser.SetFocus();
	}
}

void Edit3D::OpenTextureEditor() {
	if (dock_texture) {
		ActivateDockableChild(texture_edit);
		texture_edit.SetFocus();
	}
}

void Edit3D::SaveScene3DInteractive() {
	if (scene3d_path.IsEmpty())
		SaveScene3DAs();
	else
		SaveScene3D(scene3d_path, scene3d_use_json, true);
}

void Edit3D::SaveScene3DAs() {
	SaveScene3DWithDialog(scene3d_use_json);
}

void Edit3D::SaveScene3DAsJson() {
	SaveScene3DWithDialog(true);
}

void Edit3D::SaveScene3DAsBinary() {
	SaveScene3DWithDialog(false);
}

bool Edit3D::SaveScene3DWithDialog(bool use_json) {
	FileSel fs;
	fs.Type(t_("Scene3D (JSON)"), "*.scene3d");
	fs.Type(t_("Scene3D (Binary)"), "*.scene3db");
	fs.AllFilesType();
	if (!scene3d_path.IsEmpty())
		fs.Set(scene3d_path);
	if (!fs.ExecuteSaveAs())
		return false;
	String path = EnsureScene3DExtension(~fs, use_json);
	return SaveScene3D(path, use_json, true);
}

bool Edit3D::LoadScene3DWithDialog() {
	FileSel fs;
	fs.Type(t_("Scene3D (JSON)"), "*.scene3d");
	fs.Type(t_("Scene3D (Binary)"), "*.scene3db");
	fs.AllFilesType();
	if (!fs.ExecuteOpen())
		return false;
	return LoadScene3D(~fs);
}

static String Scene3DIsoTime(Time t) {
	String date = Format("%04d-%02d-%02d", t.year, t.month, t.day);
	String clock = Format("%02d:%02d:%02d", t.hour, t.minute, t.second);
	return date + "T" + clock + "Z";
}

void Edit3D::SyncPointcloudDatasetsExternalFiles() {
	GeomScene& scene = GetActiveScene();
	const hash_t ds_hash = TypedStringHasher<GeomPointcloudDataset>("GeomPointcloudDataset");
	Vector<Scene3DExternalFile> kept;
	for (const Scene3DExternalFile& file : scene3d_external_files) {
		if (file.type != "pointcloud.dataset")
			kept.Add(file);
	}
	for (auto& s : scene.val.sub) {
		if (!IsVfsType(s, ds_hash))
			continue;
		GeomPointcloudDataset& ds = s.GetExt<GeomPointcloudDataset>();
		if (ds.source_ref.IsEmpty())
			continue;
		Scene3DExternalFile f;
		f.id = ds.GetId();
		f.type = "pointcloud.dataset";
		f.path = ds.source_ref;
		String abs = IsFullPath(ds.source_ref)
			? ds.source_ref
			: AppendFileName(AppendFileName(project_dir, scene3d_data_dir), ds.source_ref);
		if (FileExists(abs)) {
			f.size = GetFileLength(abs);
			f.modified_utc = Scene3DIsoTime(FileGetTime(abs));
		}
		kept.Add(f);
	}
	scene3d_external_files.Clear();
	for (const Scene3DExternalFile& f : kept)
		scene3d_external_files.Add(f);
}

void Edit3D::SyncTextureExternalFiles() {
	GeomScene& scene = GetActiveScene();
	Vector<Scene3DExternalFile> kept;
	for (const Scene3DExternalFile& file : scene3d_external_files) {
		if (file.type != "texture")
			kept.Add(file);
	}
	GeomObjectCollection objs(scene);
	for (GeomObject& obj : objs) {
		GeomTextureEdit* tex = obj.FindTextureEdit();
		if (!tex || tex->path.IsEmpty())
			continue;
		Scene3DExternalFile f;
		f.id = obj.name.IsEmpty() ? IntStr((int)obj.key) : obj.name;
		f.type = "texture";
		f.path = tex->path;
		String abs = IsFullPath(tex->path)
			? tex->path
			: AppendFileName(AppendFileName(project_dir, scene3d_data_dir), tex->path);
		if (FileExists(abs)) {
			f.size = GetFileLength(abs);
			f.modified_utc = Scene3DIsoTime(FileGetTime(abs));
		}
		kept.Add(f);
	}
	scene3d_external_files.Clear();
	for (const Scene3DExternalFile& f : kept)
		scene3d_external_files.Add(f);
}

String Edit3D::GetAssetRootDir() const {
	String root = scene3d_data_dir.IsEmpty() ? String("data") : scene3d_data_dir;
	if (project_dir.IsEmpty())
		return NormalizePath(AppendFileName(GetCurrentDirectory(), root));
	return NormalizePath(AppendFileName(project_dir, root));
}

void Edit3D::AddRecentAsset(const String& rel) {
	if (rel.IsEmpty())
		return;
	int idx = FindIndex(recent_assets, rel);
	if (idx >= 0)
		recent_assets.Remove(idx);
	recent_assets.Insert(0, rel);
	while (recent_assets.GetCount() > 24)
		recent_assets.Remove(recent_assets.GetCount() - 1);
	UpdateAssetBrowser();
}

void Edit3D::UpdateAssetBrowser() {
	String root = GetAssetRootDir();
	RealizeDirectory(root);
	asset_browser.SetRoot(root);
	asset_browser.SetRecent(recent_assets);
}

void Edit3D::AddAssetFromPath(const String& path) {
	if (!state)
		return;
	String abs = path;
	if (!IsFullPath(abs))
		abs = AppendFileName(project_dir, abs);
	abs = NormalizePath(abs);
	if (!FileExists(abs))
		return;
	String rel = abs;
	if (!project_dir.IsEmpty()) {
		String base = NormalizePath(project_dir);
		if (abs.StartsWith(base)) {
			rel = abs.Mid(base.GetCount());
			if (rel.StartsWith("/") || rel.StartsWith("\\"))
				rel = rel.Mid(1);
		}
	}
	PushUndo("Add asset");
	GeomDirectory* dir = nullptr;
	if (v0.selected_ref && v0.selected_ref->kind == GeomProjectCtrl::TreeNodeRef::K_VFS && v0.selected_ref->vfs)
		dir = GetNodeDirectory(*v0.selected_ref->vfs);
	if (!dir)
		dir = &state->GetActiveScene();
	String base_name = ToVarName(GetFileTitle(rel), '_');
	if (base_name.IsEmpty())
		base_name = "asset";
	String name = base_name;
	int idx = 1;
	while (dir->FindObject(name))
		name = base_name + "_" + IntStr(idx++);
	GeomObject& obj = dir->GetAddModel(name);
	obj.type = GeomObject::O_MODEL;
	obj.asset_ref = rel;
	state->UpdateObjects();
	AddRecentAsset(rel);
	SyncAssetExternalFiles();
	RefreshData();
}

void Edit3D::SyncAssetExternalFiles() {
	Vector<Scene3DExternalFile> kept;
	for (const Scene3DExternalFile& file : scene3d_external_files) {
		if (file.type != "asset")
			kept.Add(file);
	}
	Index<String> assets;
	for (GeomScene& scene : prj->val.Sub<GeomScene>()) {
		GeomObjectCollection objs(scene);
		for (GeomObject& o : objs) {
			if (!o.asset_ref.IsEmpty())
				assets.FindAdd(o.asset_ref);
		}
	}
	for (const String& rel : assets) {
		Scene3DExternalFile f;
		f.type = "asset";
		f.path = rel;
		f.id = GetFileTitle(rel);
		String abs = IsFullPath(rel) ? rel : AppendFileName(project_dir, rel);
		if (FileExists(abs)) {
			f.size = GetFileLength(abs);
			f.modified_utc = Scene3DIsoTime(FileGetTime(abs));
		}
		kept.Add(f);
	}
	scene3d_external_files.Clear();
	for (const Scene3DExternalFile& f : kept)
		scene3d_external_files.Add(f);
}

String Edit3D::EnsureTexturePath(GeomObject& obj, GeomTextureEdit& tex) {
	if (project_dir.IsEmpty())
		SetProjectDir(GetCurrentDirectory());
	if (scene3d_data_dir.IsEmpty())
		scene3d_data_dir = "data";
	String rel = tex.path;
	if (!rel.IsEmpty())
		return rel;
	String base = obj.name.IsEmpty() ? "texture" : obj.name;
	String safe = ToVarName(base, '_');
	if (safe.IsEmpty())
		safe = "texture";
	String dir = AppendFileName(project_dir, scene3d_data_dir);
	RealizeDirectory(dir);
	String name = safe + "_tex.png";
	String rel_try = AppendFileName(scene3d_data_dir, name);
	String abs_try = AppendFileName(project_dir, rel_try);
	int idx = 1;
	while (FileExists(abs_try)) {
		name = safe + "_tex_" + IntStr(idx++) + ".png";
		rel_try = AppendFileName(scene3d_data_dir, name);
		abs_try = AppendFileName(project_dir, rel_try);
	}
	tex.path = rel_try;
	return rel_try;
}

void Edit3D::SaveTextureEdit(GeomObject& obj, GeomTextureEdit& tex, ImageBuffer& ib) {
	String rel = EnsureTexturePath(obj, tex);
	String abs = IsFullPath(rel) ? rel : AppendFileName(project_dir, rel);
	PNGEncoder().SaveFile(abs, Image(ib));
	SyncTextureExternalFiles();
}

void Edit3D::UpdateTextureEditor(GeomObject* obj) {
	texture_obj = obj;
	if (!obj) {
		texture_edit.Clear();
		texture_edit.WhenPaint.Clear();
		return;
	}
	GeomTextureEdit* tex = obj->FindTextureEdit();
	if (!tex) {
		texture_edit.Clear();
		texture_edit.WhenPaint.Clear();
		return;
	}
	Image img;
	if (!tex->path.IsEmpty()) {
		String abs = IsFullPath(tex->path) ? tex->path : AppendFileName(project_dir, tex->path);
		if (FileExists(abs))
			img = StreamRaster::LoadFileAny(abs);
	}
	if (img.IsEmpty()) {
		ImageBuffer ib(tex->width, tex->height);
		for (int y = 0; y < ib.GetHeight(); y++) {
			RGBA* line = ib[y];
			for (int x = 0; x < ib.GetWidth(); x++)
				line[x] = White();
		}
		img = Image(ib);
	}
	texture_edit.SetImage(img);
	texture_edit.WhenPaint = [=] {
		if (texture_obj && tex)
			SaveTextureEdit(*texture_obj, *tex, texture_edit.img);
	};
}

bool Edit3D::LoadScene3D(const String& path) {
	Scene3DDocument doc;
	bool use_json = !IsScene3DBinaryPath(path);
	if (use_json) {
		if (!LoadScene3DJson(path, doc))
			return false;
	}
	else if (!LoadScene3DBin(path, doc)) {
		return false;
	}
	for (GeomScene& scene : doc.project->val.Sub<GeomScene>()) {
		GeomObjectCollection objects(scene);
		for (GeomObject& obj : objects) {
			if (!obj.IsModel())
				continue;
			if (obj.mdl || !obj.asset_ref.IsEmpty())
				continue;
			ModelBuilder mb;
			mb.AddBox(0, 1, 1);
			obj.mdl = mb.Detach();
		}
	}
	VisitCopy(*doc.project, *prj);
	state->prj = prj;
	state->active_scene = doc.active_scene;
	VisitCopy(*doc.focus, state->GetFocus());
	VisitCopy(*doc.program, state->GetProgram());
	scene3d_path = path;
	SetProjectDir(GetFileFolder(path));
	scene3d_use_json = use_json;
	scene3d_created = doc.created_utc;
	scene3d_modified = doc.modified_utc;
	scene3d_data_dir = doc.data_dir;
	scene3d_external_files = pick(doc.external_files);
	scene3d_meta = pick(doc.meta);
	if (scene3d_data_dir.IsEmpty())
		scene3d_data_dir = "data";
	UpdateAssetBrowser();
	script_instances.Clear();
	state->UpdateObjects();
	anim->Reset();
	Data();
	v0.TimelineData();
	v0.tree.OpenDeep(v0.tree_scenes);
	UpdateWindowTitle();
	return true;
}

bool Edit3D::SaveScene3D(const String& path, bool use_json, bool pretty) {
	SyncPointcloudDatasetsExternalFiles();
	SyncAssetExternalFiles();
	SyncTextureExternalFiles();
	Scene3DDocument doc;
	doc.version = SCENE3D_VERSION;
	doc.name = "ModelerApp";
	doc.project = prj;
	doc.active_scene = state->active_scene;
	doc.focus = &state->GetFocus();
	doc.program = &state->GetProgram();
	if (scene3d_created.IsEmpty())
		scene3d_created = Scene3DIsoTime(GetUtcTime());
	scene3d_modified = Scene3DIsoTime(GetUtcTime());
	if (scene3d_data_dir.IsEmpty())
		scene3d_data_dir = "data";
	doc.created_utc = scene3d_created;
	doc.modified_utc = scene3d_modified;
	doc.data_dir = scene3d_data_dir;
	doc.external_files.Clear();
	for (const Scene3DExternalFile& file : scene3d_external_files)
		doc.external_files.Add(file);
	doc.meta.Clear();
	for (const Scene3DMetaEntry& entry : scene3d_meta)
		doc.meta.Add(entry);
	bool ok = use_json ? SaveScene3DJson(path, doc, pretty) : SaveScene3DBin(path, doc);
	if (ok) {
		scene3d_path = path;
		SetProjectDir(GetFileFolder(path));
		scene3d_use_json = use_json;
		UpdateWindowTitle();
	}
	return ok;
}

#if 0
void Edit3D::LoadRemote(EditClientService* svc, bool debug) {
	this->svc = svc;
	this->debug_remote = debug;
	
	if (svc) {
		svc->sync.SetTarget(*prj, *state, *anim, video);
	}
	
	if (debug_remote) {
		svc->SetDebuggingMode();
		
		LoadEmptyProject();
		SetView(VIEW_REMOTE_DEBUG);
	}
	else
		LoadTestProject(0);
	
	svc->edit = this;
	svc->sync.SetRequireAllSync();
	svc->SetReady();
}
#endif

void Edit3D::OnDebugMetadata() {
	
	TODO
	
}



END_UPP_NAMESPACE

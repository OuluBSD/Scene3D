#include "Exec.h"
#include <Geometry/Geometry.h>
#include <Core/TextParsing/Tokenizer.h>

void RegisterGeometry(Upp::PyVM& vm);

NAMESPACE_UPP

void ExecutionFileMapping::Jsonize(JsonIO& jio) {
	jio("type", type)
	   ("source", source)
	   ("exported", exported);
}

void ExecutionManifest::Jsonize(JsonIO& jio) {
	jio("version", version)
	   ("mode", mode)
	   ("exported_utc", exported_utc)
	   ("project_name", project_name)
	   ("scene3d", scene3d)
	   ("project_dir", project_dir)
	   ("data_dir", data_dir);
	jio.Array("files", files, [](JsonIO& io, ExecutionFileMapping& item) { item.Jsonize(io); });
}

bool LoadExecutionManifest(const String& path, ExecutionManifest& out) {
	if (path.IsEmpty())
		return false;
	return LoadFromJsonFile(out, path);
}

bool SaveExecutionManifest(const String& path, ExecutionManifest& manifest, bool pretty) {
	if (path.IsEmpty())
		return false;
	StoreAsJsonFile(manifest, ~path, pretty);
	return true;
}

void ExecInputState::BeginFrame() {
	mouse_delta = Point(0, 0);
	wheel_delta = 0;
	keys_pressed.Clear();
	keys_released.Clear();
}

void ExecInputState::SetMouse(Point p, dword flags) {
	mouse_delta += p - mouse_pos;
	mouse_pos = p;
	mouse_flags = flags;
}

void ExecInputState::SetMouseDown(Point p, dword flags) {
	mouse_down = true;
	SetMouse(p, flags);
}

void ExecInputState::SetMouseUp(Point p, dword flags) {
	mouse_down = false;
	SetMouse(p, flags);
}

void ExecInputState::AddWheel(int delta, dword flags) {
	wheel_delta += delta;
	mouse_flags = flags;
}

void ExecInputState::SetKey(int key, bool down) {
	if (down) {
		if (keys_down.Find(key) < 0)
			keys_down.Add(key);
		if (keys_pressed.Find(key) < 0)
			keys_pressed.Add(key);
	}
	else {
		int idx = keys_down.Find(key);
		if (idx >= 0)
			keys_down.Remove(idx);
		if (keys_released.Find(key) < 0)
			keys_released.Add(key);
	}
}

static hash_t HashGeomObject() {
	static bool init = (TypedStringHasher<GeomObject>("GeomObject"), true);
	return AsTypeHash<GeomObject>();
}

static hash_t HashGeomScene() {
	static bool init = (TypedStringHasher<GeomScene>("GeomScene"), true);
	return AsTypeHash<GeomScene>();
}

static hash_t HashGeomDirectory() {
	static bool init = (TypedStringHasher<GeomDirectory>("GeomDirectory"), true);
	return AsTypeHash<GeomDirectory>();
}

static hash_t HashGeomDataset() {
	static bool init = (TypedStringHasher<GeomPointcloudDataset>("GeomPointcloudDataset"), true);
	return AsTypeHash<GeomPointcloudDataset>();
}

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

static GeomDynamicProperties* FindDynamicProps(VfsValue& n) {
	static bool init = (TypedStringHasher<GeomDynamicProperties>("GeomDynamicProperties"), true);
	for (auto& sub : n.sub) {
		if (IsVfsType(sub, AsTypeHash<GeomDynamicProperties>()) && sub.id == "props")
			return &sub.GetExt<GeomDynamicProperties>();
	}
	return nullptr;
}

static bool NodeTypeMatches(const VfsValue& node, const String& type) {
	String t = ToLower(type);
	if (IsVfsType(node, HashGeomObject())) {
		const GeomObject& obj = const_cast<VfsValue&>(node).GetExt<GeomObject>();
		String ui = ToLower(obj.ui_type);
		if (ui.IsEmpty()) {
			if (GeomDynamicProperties* props = FindDynamicProps(const_cast<VfsValue&>(node))) {
				int pi = props->props.Find("type");
				if (pi >= 0)
					ui = ToLower(AsString(props->props[pi]));
			}
		}
		if (!ui.IsEmpty()) {
			if (t == ui)
				return true;
			if (t == "light" && (ui == "light" || ui == "light.point" || ui == "light.directional"))
				return true;
			if (t == "overlay" && (ui == "overlay2d" || ui == "touch2d"))
				return true;
			if (t == "pathnode" && ui == "path_node")
				return true;
		}
	}
	if (t == "object" && IsVfsType(node, HashGeomObject()))
		return true;
	if (t == "model" && IsVfsType(node, HashGeomObject()) && const_cast<VfsValue&>(node).GetExt<GeomObject>().IsModel())
		return true;
	if (t == "camera" && IsVfsType(node, HashGeomObject()) && const_cast<VfsValue&>(node).GetExt<GeomObject>().IsCamera())
		return true;
	if ((t == "directory" || t == "dir") && IsVfsType(node, HashGeomDirectory()))
		return true;
	if (t == "scene" && IsVfsType(node, HashGeomScene()))
		return true;
	if ((t == "dataset" || t == "pointcloud_dataset") && IsVfsType(node, HashGeomDataset()))
		return true;
	return false;
}

static VfsValue* FindChildByName(VfsValue& node, const String& name) {
	for (auto& sub : node.sub) {
		if (GetNodeName(sub) == name || sub.id == name)
			return &sub;
	}
	return nullptr;
}

static VfsValue* FindChildByNameRecursive(VfsValue& node, const String& name) {
	if (VfsValue* found = FindChildByName(node, name))
		return found;
	for (auto& sub : node.sub) {
		if (VfsValue* child = FindChildByNameRecursive(sub, name))
			return child;
	}
	return nullptr;
}

static int FindChildIndex(VfsValue& parent, VfsValue* child) {
	if (!child)
		return -1;
	for (int i = 0; i < parent.sub.GetCount(); i++)
		if (&parent.sub[i] == child)
			return i;
	return -1;
}

static PyValue ValueToPyValue(const Value& v) {
	return PyValue::FromValue(v);
}

static Value PyValueToValue(const PyValue& v) {
	return v.ToValue();
}

static bool PyValueToVec3(const PyValue& v, vec3& out) {
	if ((v.GetType() == PY_LIST || v.GetType() == PY_TUPLE) && v.GetCount() >= 3) {
		out = vec3((float)v.GetItem(0).AsDouble(), (float)v.GetItem(1).AsDouble(), (float)v.GetItem(2).AsDouble());
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

struct DisplayObjectProxy : PyUserData {
	ExecScriptRuntime* runtime = nullptr;
	VfsValue* node = nullptr;
	PyVM* vm = nullptr;

	DisplayObjectProxy(ExecScriptRuntime* r, VfsValue* n, PyVM* v) : runtime(r), node(n), vm(v) {}
	String GetTypeName() const override { return "DisplayObject"; }
	PyValue GetAttr(const String& name) override;
	bool SetAttr(const String& name, const PyValue& v) override;
};

static PyValue MakeDisplayObject(ExecScriptRuntime* rt, VfsValue* node, PyVM* vm) {
	if (!rt || !node || !vm)
		return PyValue::None();
	return PyValue(new DisplayObjectProxy(rt, node, vm));
}

static PyValue DisplayObject_AddChild(const Vector<PyValue>& args, void* user_data) {
	if (args.GetCount() < 2 || !args[0].IsUserData())
		return PyValue::None();
	DisplayObjectProxy& self = (DisplayObjectProxy&)args[0].GetUserData();
	String type = args[1].ToString();
	String name = args.GetCount() > 2 ? args[2].ToString() : String();
	if (!self.runtime || !self.node)
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
	if (self.runtime->state)
		self.runtime->state->UpdateObjects();
	if (self.runtime->WhenChanged)
		self.runtime->WhenChanged();
	return MakeDisplayObject(self.runtime, out, self.vm);
}

static PyValue DisplayObject_Find(const Vector<PyValue>& args, void* user_data) {
	if (args.GetCount() < 2 || !args[0].IsUserData())
		return PyValue::None();
	DisplayObjectProxy& self = (DisplayObjectProxy&)args[0].GetUserData();
	if (!self.runtime || !self.node)
		return PyValue::None();
	String name = args[1].ToString();
	VfsValue* found = FindChildByName(*self.node, name);
	return MakeDisplayObject(self.runtime, found, self.vm);
}

static PyValue DisplayObject_Children(const Vector<PyValue>& args, void* user_data) {
	if (args.GetCount() < 1 || !args[0].IsUserData())
		return PyValue::None();
	DisplayObjectProxy& self = (DisplayObjectProxy&)args[0].GetUserData();
	if (!self.runtime || !self.node)
		return PyValue::None();
	PyValue list = PyValue::List();
	for (auto& sub : self.node->sub)
		list.Add(MakeDisplayObject(self.runtime, &sub, self.vm));
	return list;
}

static PyValue DisplayObject_GetChildByName(const Vector<PyValue>& args, void* user_data) {
	if (args.GetCount() < 2 || !args[0].IsUserData())
		return PyValue::None();
	DisplayObjectProxy& self = (DisplayObjectProxy&)args[0].GetUserData();
	if (!self.runtime || !self.node)
		return PyValue::None();
	String name = args[1].ToString();
	VfsValue* found = FindChildByName(*self.node, name);
	return MakeDisplayObject(self.runtime, found, self.vm);
}

static PyValue DisplayObject_GetChildrenByType(const Vector<PyValue>& args, void* user_data) {
	if (args.GetCount() < 2 || !args[0].IsUserData())
		return PyValue::None();
	DisplayObjectProxy& self = (DisplayObjectProxy&)args[0].GetUserData();
	if (!self.runtime || !self.node)
		return PyValue::None();
	String type = args[1].ToString();
	PyValue list = PyValue::List();
	for (auto& sub : self.node->sub) {
		if (NodeTypeMatches(sub, type))
			list.Add(MakeDisplayObject(self.runtime, &sub, self.vm));
	}
	return list;
}

static PyValue DisplayObject_On(const Vector<PyValue>& args, void* user_data) {
	if (args.GetCount() < 3 || !args[0].IsUserData())
		return PyValue::None();
	DisplayObjectProxy& self = (DisplayObjectProxy&)args[0].GetUserData();
	if (!self.runtime || !self.node)
		return PyValue::None();
	String event = args[1].ToString();
	PyValue fn = args[2];
	self.runtime->AddScriptEventHandler(event, self.vm, self.node, fn);
	return PyValue::True();
}

static PyValue DisplayObject_OnFrame(const Vector<PyValue>& args, void* user_data) {
	if (args.GetCount() < 2 || !args[0].IsUserData())
		return PyValue::None();
	DisplayObjectProxy& self = (DisplayObjectProxy&)args[0].GetUserData();
	if (!self.runtime || !self.node)
		return PyValue::None();
	PyValue fn = args[1];
	self.runtime->AddScriptEventHandler("enterFrame", self.vm, self.node, fn);
	return PyValue::True();
}

static bool SetTimelinePosition(ExecScriptRuntime* rt, int frame, bool play) {
	if (!rt || !rt->state || !rt->anim)
		return false;
	if (!rt->state->HasActiveScene())
		return false;
	GeomScene& scene = rt->state->GetActiveScene();
	int target = min(max(frame, 0), max(scene.length - 1, 0));
	double time = target / (double)rt->state->prj->kps;
	rt->anim->ApplyAtPosition(target, time);
	rt->anim->position = target;
	rt->anim->time = time;
	rt->anim->is_playing = play;
	if (rt->WhenChanged)
		rt->WhenChanged();
	return true;
}

static PyValue DisplayObject_GotoAndPlay(const Vector<PyValue>& args, void* user_data) {
	if (args.GetCount() < 2 || !args[0].IsUserData())
		return PyValue::None();
	DisplayObjectProxy& self = (DisplayObjectProxy&)args[0].GetUserData();
	int frame = args[1].AsInt();
	return PyValue(SetTimelinePosition(self.runtime, frame, true));
}

static PyValue DisplayObject_GotoAndStop(const Vector<PyValue>& args, void* user_data) {
	if (args.GetCount() < 2 || !args[0].IsUserData())
		return PyValue::None();
	DisplayObjectProxy& self = (DisplayObjectProxy&)args[0].GetUserData();
	int frame = args[1].AsInt();
	return PyValue(SetTimelinePosition(self.runtime, frame, false));
}

PyValue DisplayObjectProxy::GetAttr(const String& name) {
	if (!node)
		return PyValue::None();
	GeomTransform* tr = GetNodeTransform(*node);
	if (name == "name")
		return PyValue(GetNodeName(*node));
	if (name == "type") {
		if (GeomObject* obj = GetNodeObject(*node)) {
			if (!obj->ui_type.IsEmpty())
				return PyValue(obj->ui_type);
		}
	}
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
	if (name == "find")
		return PyValue::BoundMethod(PyValue::Function("find", DisplayObject_Find, nullptr), PyValue(this));
	if (name == "children")
		return PyValue::BoundMethod(PyValue::Function("children", DisplayObject_Children, nullptr), PyValue(this));
	if (name == "getChildByName")
		return PyValue::BoundMethod(PyValue::Function("getChildByName", DisplayObject_GetChildByName, nullptr), PyValue(this));
	if (name == "getChildrenByType")
		return PyValue::BoundMethod(PyValue::Function("getChildrenByType", DisplayObject_GetChildrenByType, nullptr), PyValue(this));
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
		if (name == "x") { tr->position[0] = v.AsDouble(); if (runtime && runtime->state) runtime->state->UpdateObjects(); if (runtime && runtime->WhenChanged) runtime->WhenChanged(); return true; }
		if (name == "y") { tr->position[1] = v.AsDouble(); if (runtime && runtime->state) runtime->state->UpdateObjects(); if (runtime && runtime->WhenChanged) runtime->WhenChanged(); return true; }
		if (name == "z") { tr->position[2] = v.AsDouble(); if (runtime && runtime->state) runtime->state->UpdateObjects(); if (runtime && runtime->WhenChanged) runtime->WhenChanged(); return true; }
		if (name == "rotation") {
			vec3 axes;
			if (PyValueToVec3(v, axes)) {
				tr->orientation = AxesQuat(axes);
				if (runtime && runtime->state) runtime->state->UpdateObjects();
				if (runtime && runtime->WhenChanged) runtime->WhenChanged();
				return true;
			}
		}
		if (name == "scale") {
			vec3 scale;
			if (PyValueToVec3(v, scale)) {
				tr->scale = scale;
				if (runtime && runtime->state) runtime->state->UpdateObjects();
				if (runtime && runtime->WhenChanged) runtime->WhenChanged();
				return true;
			}
		}
	}
	if (name == "visible") {
		if (GeomObject* obj = GetNodeObject(*node)) {
			obj->is_visible = v.IsTrue();
			if (runtime && runtime->WhenChanged)
				runtime->WhenChanged();
			return true;
		}
	}
	if (name == "onFrame" || name == "onEnterFrame") {
		if (runtime)
			runtime->AddScriptEventHandler("enterFrame", vm, node, v);
		return true;
	}
	if (GeomDynamicProperties* props = FindDynamicProps(*node)) {
		props->props.GetAdd(name) = PyValueToValue(v);
		if (runtime && runtime->WhenChanged)
			runtime->WhenChanged();
		return true;
	}
	return false;
}

struct StageProxy : PyUserData {
	ExecScriptRuntime* runtime = nullptr;
	PyVM* vm = nullptr;

	StageProxy(ExecScriptRuntime* r, PyVM* v) : runtime(r), vm(v) {}
	String GetTypeName() const override { return "Stage"; }
	PyValue GetAttr(const String& name) override;
	bool SetAttr(const String& name, const PyValue& v) override;
};

static PyValue Stage_Find(const Vector<PyValue>& args, void* user_data) {
	if (args.GetCount() < 2 || !args[0].IsUserData())
		return PyValue::None();
	StageProxy& self = (StageProxy&)args[0].GetUserData();
	if (!self.runtime || !self.runtime->state || !self.runtime->state->HasActiveScene())
		return PyValue::None();
	String name = args[1].ToString();
	VfsValue* found = FindChildByNameRecursive(self.runtime->state->GetActiveScene().val, name);
	return MakeDisplayObject(self.runtime, found, self.vm);
}

static PyValue Stage_Children(const Vector<PyValue>& args, void* user_data) {
	if (args.GetCount() < 1 || !args[0].IsUserData())
		return PyValue::None();
	StageProxy& self = (StageProxy&)args[0].GetUserData();
	if (!self.runtime || !self.runtime->state || !self.runtime->state->HasActiveScene())
		return PyValue::None();
	PyValue list = PyValue::List();
	for (auto& sub : self.runtime->state->GetActiveScene().val.sub)
		list.Add(MakeDisplayObject(self.runtime, &sub, self.vm));
	return list;
}

static PyValue Stage_Create(const Vector<PyValue>& args, void* user_data) {
	if (args.GetCount() < 2 || !args[0].IsUserData())
		return PyValue::None();
	StageProxy& self = (StageProxy&)args[0].GetUserData();
	if (!self.runtime || !self.runtime->state || !self.runtime->state->HasActiveScene())
		return PyValue::None();
	String type = args[1].ToString();
	String name = args.GetCount() > 2 ? args[2].ToString() : String();
	GeomScene& scene = self.runtime->state->GetActiveScene();
	if (name.IsEmpty())
		name = type.IsEmpty() ? "node" : type;
	VfsValue* out = nullptr;
	String t = ToLower(type);
	if (t == "directory" || t == "dir")
		out = &scene.GetAddDirectory(name).val;
	else if (t == "model")
		out = &scene.GetAddModel(name).val;
	else if (t == "camera")
		out = &scene.GetAddCamera(name).val;
	else if (t == "pointcloud" || t == "octree")
		out = &scene.GetAddOctree(name).val;
	if (!out)
		return PyValue::None();
	self.runtime->state->UpdateObjects();
	if (self.runtime->WhenChanged)
		self.runtime->WhenChanged();
	return MakeDisplayObject(self.runtime, out, self.vm);
}

static PyValue Stage_Goto(const Vector<PyValue>& args, void* user_data) {
	if (args.GetCount() < 2 || !args[0].IsUserData())
		return PyValue::None();
	StageProxy& self = (StageProxy&)args[0].GetUserData();
	int frame = args[1].AsInt();
	return PyValue(SetTimelinePosition(self.runtime, frame, self.runtime && self.runtime->anim ? self.runtime->anim->is_playing : false));
}

static PyValue Stage_GotoAndPlay(const Vector<PyValue>& args, void* user_data) {
	if (args.GetCount() < 2 || !args[0].IsUserData())
		return PyValue::None();
	StageProxy& self = (StageProxy&)args[0].GetUserData();
	int frame = args[1].AsInt();
	return PyValue(SetTimelinePosition(self.runtime, frame, true));
}

static PyValue Stage_GotoAndStop(const Vector<PyValue>& args, void* user_data) {
	if (args.GetCount() < 2 || !args[0].IsUserData())
		return PyValue::None();
	StageProxy& self = (StageProxy&)args[0].GetUserData();
	int frame = args[1].AsInt();
	return PyValue(SetTimelinePosition(self.runtime, frame, false));
}

static PyValue Stage_On(const Vector<PyValue>& args, void* user_data) {
	if (args.GetCount() < 3 || !args[0].IsUserData())
		return PyValue::None();
	StageProxy& self = (StageProxy&)args[0].GetUserData();
	String event = args[1].ToString();
	PyValue fn = args[2];
	if (self.runtime)
		self.runtime->AddScriptEventHandler(event, self.vm, nullptr, fn);
	return PyValue::True();
}

static PyValue Stage_OnFrame(const Vector<PyValue>& args, void* user_data) {
	if (args.GetCount() < 2 || !args[0].IsUserData())
		return PyValue::None();
	StageProxy& self = (StageProxy&)args[0].GetUserData();
	PyValue fn = args[1];
	if (self.runtime)
		self.runtime->AddScriptEventHandler("enterFrame", self.vm, nullptr, fn);
	return PyValue::True();
}

PyValue StageProxy::GetAttr(const String& name) {
	if (!runtime || !runtime->state || !runtime->anim)
		return PyValue::None();
	if (name == "time")
		return PyValue(runtime->anim->time);
	if (name == "frame")
		return PyValue(runtime->anim->position);
	if (name == "fps")
		return PyValue(runtime->state->prj ? runtime->state->prj->fps : 0);
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
	return PyValue::None();
}

bool StageProxy::SetAttr(const String& name, const PyValue& v) {
	if (!runtime)
		return false;
	if (name == "time") {
		runtime->anim->time = v.AsDouble();
		return true;
	}
	if (name == "frame") {
		runtime->anim->position = v.AsInt();
		return true;
	}
	return false;
}

struct InputProxy : PyUserData {
	ExecScriptRuntime* runtime = nullptr;
	InputProxy(ExecScriptRuntime* r) : runtime(r) {}
	String GetTypeName() const override { return "Input"; }
	PyValue GetAttr(const String& name) override;
	bool SetAttr(const String& name, const PyValue& v) override;
};

struct CameraProxy : PyUserData {
	ExecScriptRuntime* runtime = nullptr;
	CameraProxy(ExecScriptRuntime* r) : runtime(r) {}
	String GetTypeName() const override { return "Camera"; }
	PyValue GetAttr(const String& name) override;
	bool SetAttr(const String& name, const PyValue& v) override;
};

struct PhysicsWorldProxy : PyUserData {
	ExecScriptRuntime* runtime = nullptr;
	SoftPhys::World* world = nullptr;
	PhysicsWorldProxy(ExecScriptRuntime* r, SoftPhys::World* w) : runtime(r), world(w) {}
	String GetTypeName() const override { return "SoftPhysWorld"; }
	PyValue GetAttr(const String&) override { return PyValue::None(); }
	bool SetAttr(const String&, const PyValue&) override { return false; }
};

struct PhysicsSpaceProxy : PyUserData {
	ExecScriptRuntime* runtime = nullptr;
	SoftPhys::Space* space = nullptr;
	PhysicsSpaceProxy(ExecScriptRuntime* r, SoftPhys::Space* s) : runtime(r), space(s) {}
	String GetTypeName() const override { return "SoftPhysSpace"; }
	PyValue GetAttr(const String&) override { return PyValue::None(); }
	bool SetAttr(const String&, const PyValue&) override { return false; }
};

struct PhysicsBodyProxy : PyUserData {
	ExecScriptRuntime* runtime = nullptr;
	SoftPhys::RigidbodyVolume* body = nullptr;
	PhysicsBodyProxy(ExecScriptRuntime* r, SoftPhys::RigidbodyVolume* b) : runtime(r), body(b) {}
	String GetTypeName() const override { return "SoftPhysBody"; }
	PyValue GetAttr(const String& name) override;
	bool SetAttr(const String& name, const PyValue& v) override;
};

static SoftPhys::World* GetWorldFromPy(const PyValue& v) {
	if (!v.IsUserData())
		return nullptr;
	PyUserData& ud = v.GetUserData();
	if (ud.GetTypeName() != "SoftPhysWorld")
		return nullptr;
	return ((PhysicsWorldProxy&)ud).world;
}

static SoftPhys::Space* GetSpaceFromPy(const PyValue& v) {
	if (!v.IsUserData())
		return nullptr;
	PyUserData& ud = v.GetUserData();
	if (ud.GetTypeName() != "SoftPhysSpace")
		return nullptr;
	return ((PhysicsSpaceProxy&)ud).space;
}

static SoftPhys::RigidbodyVolume* GetBodyFromPy(const PyValue& v) {
	if (!v.IsUserData())
		return nullptr;
	PyUserData& ud = v.GetUserData();
	if (ud.GetTypeName() != "SoftPhysBody")
		return nullptr;
	return ((PhysicsBodyProxy&)ud).body;
}

void ExecScriptRuntime::DriverAI::Init() {
	ap.Clear();
	ap.SetSize(0, 0);
	wrapper.Create(ap);
	atom_target_left = wrapper->GetAtomIndex("target_left");
	atom_target_right = wrapper->GetAtomIndex("target_right");
	atom_target_ahead = wrapper->GetAtomIndex("target_ahead");
	atom_at_target = wrapper->GetAtomIndex("at_target");

	act_turn_left = wrapper->GetEventIndex("turn_left");
	act_turn_right = wrapper->GetEventIndex("turn_right");
	act_go_straight = wrapper->GetEventIndex("go_straight");
	act_stop = wrapper->GetEventIndex("stop");

	wrapper->SetPreCondition("turn_left", "target_left", true);
	wrapper->SetPreCondition("turn_right", "target_right", true);
	wrapper->SetPreCondition("go_straight", "target_ahead", true);
	wrapper->SetPreCondition("stop", "at_target", true);

	wrapper->SetCost("turn_left", 2);
	wrapper->SetCost("turn_right", 2);
	wrapper->SetCost("go_straight", 1);
	wrapper->SetCost("stop", 0);

	inited = true;
}

int ExecScriptRuntime::DriverAI::PlanAction(const vec3& car_pos, float heading, const vec3& target_pos, float arrive_radius, float angle_thresh) {
	if (!inited)
		Init();
	vec3 to = target_pos - car_pos;
	float dist = sqrt(Dot(to, to));
	bool at_target = dist <= arrive_radius;
	bool target_left = false;
	bool target_right = false;
	bool target_ahead = false;
	if (!at_target && dist > 0.0001f) {
		vec3 dir = to / dist;
		vec3 fwd = vec3(-sin(heading), 0.0f, cos(heading));
		float dotv = Clamp(Dot(fwd, dir), -1.0f, 1.0f);
		float ang = acos(dotv);
		float cross = fwd[0] * dir[2] - fwd[2] * dir[0];
		if (ang <= angle_thresh)
			target_ahead = true;
		else if (cross > 0.0f)
			target_left = true;
		else
			target_right = true;
	}
	BinaryWorldState ws;
	ws.mask = wrapper->GetMask();
	ws.SetAtomIndex(atom_target_left, target_left);
	ws.SetAtomIndex(atom_target_right, target_right);
	ws.SetAtomIndex(atom_target_ahead, target_ahead);
	ws.SetAtomIndex(atom_at_target, at_target);

	Array<BinaryWorldState*> dest;
	Vector<int> act_ids;
	Vector<double> costs;
	ap.GetPossibleStateTransition(ws, dest, act_ids, costs);
	if (act_ids.IsEmpty()) {
		if (at_target)
			return 2;
		if (target_left)
			return -1;
		if (target_right)
			return 1;
		return 0;
	}
	int best_i = 0;
	double best_cost = costs[0];
	for (int i = 1; i < costs.GetCount(); i++) {
		if (costs[i] < best_cost) {
			best_cost = costs[i];
			best_i = i;
		}
	}
	int act = act_ids[best_i];
	if (act == act_turn_left)
		return -1;
	if (act == act_turn_right)
		return 1;
	if (act == act_stop)
		return 2;
	return 0;
}

Vector<int> ExecScriptRuntime::DriverAI::ComputeRoute(const Vector<vec3>& points, int start_idx, int goal_idx, double max_edge) {
	Vector<int> out;
	int n = points.GetCount();
	if (n == 0 || start_idx < 0 || start_idx >= n || goal_idx < 0 || goal_idx >= n)
		return out;
	Vector<double> dist;
	Vector<int> prev;
	Vector<bool> used;
	dist.SetCount(n, DBL_MAX);
	prev.SetCount(n, -1);
	used.SetCount(n, false);
	dist[start_idx] = 0.0;
	for (int i = 0; i < n; i++) {
		int v = -1;
		double best = DBL_MAX;
		for (int j = 0; j < n; j++) {
			if (!used[j] && dist[j] < best) {
				best = dist[j];
				v = j;
			}
		}
		if (v < 0)
			break;
		used[v] = true;
		if (v == goal_idx)
			break;
		for (int u = 0; u < n; u++) {
			if (u == v)
				continue;
			vec3 diff = points[u] - points[v];
			double d = sqrt(Dot(diff, diff));
			if (max_edge > 0 && d > max_edge)
				continue;
			double nd = dist[v] + d;
			if (nd < dist[u]) {
				dist[u] = nd;
				prev[u] = v;
			}
		}
	}
	int cur = goal_idx;
	if (cur == start_idx) {
		out.Add(cur);
		return out;
	}
	if (prev[cur] == -1)
		return out;
	Vector<int> rev;
	while (cur >= 0) {
		rev.Add(cur);
		if (cur == start_idx)
			break;
		cur = prev[cur];
	}
	for (int i = rev.GetCount() - 1; i >= 0; i--)
		out.Add(rev[i]);
	return out;
}

static PyValue Input_IsKeyDown(const Vector<PyValue>& args, void* user_data) {
	if (args.GetCount() < 1)
		return PyValue::False();
	InputProxy* self = (InputProxy*)user_data;
	if (!self || !self->runtime)
		return PyValue::False();
	int key = args[0].AsInt();
	bool down = self->runtime->input.IsKeyDown(key);
	return PyValue(down);
}

static PyValue Input_WasKeyPressed(const Vector<PyValue>& args, void* user_data) {
	if (args.GetCount() < 1)
		return PyValue::False();
	InputProxy* self = (InputProxy*)user_data;
	if (!self || !self->runtime)
		return PyValue::False();
	return PyValue(self->runtime->input.WasKeyPressed(args[0].AsInt()));
}

static PyValue Input_WasKeyReleased(const Vector<PyValue>& args, void* user_data) {
	if (args.GetCount() < 1)
		return PyValue::False();
	InputProxy* self = (InputProxy*)user_data;
	if (!self || !self->runtime)
		return PyValue::False();
	return PyValue(self->runtime->input.WasKeyReleased(args[0].AsInt()));
}

static PyValue Input_IsMouseDown(const Vector<PyValue>& args, void* user_data) {
	InputProxy* self = (InputProxy*)user_data;
	if (!self || !self->runtime)
		return PyValue::False();
	return PyValue(self->runtime->input.mouse_down);
}

PyValue InputProxy::GetAttr(const String& name) {
	if (!runtime)
		return PyValue::None();
	if (name == "mouseX")
		return PyValue(runtime->input.mouse_pos.x);
	if (name == "mouseY")
		return PyValue(runtime->input.mouse_pos.y);
	if (name == "mouseDX")
		return PyValue(runtime->input.mouse_delta.x);
	if (name == "mouseDY")
		return PyValue(runtime->input.mouse_delta.y);
	if (name == "mouseDown")
		return PyValue(runtime->input.mouse_down);
	if (name == "wheel")
		return PyValue(runtime->input.wheel_delta);
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

bool InputProxy::SetAttr(const String& name, const PyValue& v) {
	return false;
}

PyValue CameraProxy::GetAttr(const String& name) {
	if (!runtime || !runtime->state)
		return PyValue::None();
	GeomCamera& cam = runtime->state->GetProgram();
	if (name == "x")
		return PyValue(cam.position[0]);
	if (name == "y")
		return PyValue(cam.position[1]);
	if (name == "z")
		return PyValue(cam.position[2]);
	if (name == "rotation")
		return MakeVec3Value(GetQuatAxes(cam.orientation));
	if (name == "fov")
		return PyValue(cam.fov);
	if (name == "scale")
		return PyValue(cam.scale);
	return PyValue::None();
}

bool CameraProxy::SetAttr(const String& name, const PyValue& v) {
	if (!runtime || !runtime->state)
		return false;
	GeomCamera& cam = runtime->state->GetProgram();
	if (name == "x") { cam.position[0] = v.AsDouble(); return true; }
	if (name == "y") { cam.position[1] = v.AsDouble(); return true; }
	if (name == "z") { cam.position[2] = v.AsDouble(); return true; }
	if (name == "rotation") {
		vec3 axes;
		if (PyValueToVec3(v, axes)) {
			cam.orientation = AxesQuat(axes);
			return true;
		}
	}
	if (name == "fov") { cam.fov = v.AsDouble(); return true; }
	if (name == "scale") { cam.scale = v.AsDouble(); return true; }
	return false;
}

PyValue PhysicsBodyProxy::GetAttr(const String& name) {
	if (!body)
		return PyValue::None();
	if (name == "position")
		return MakeVec3Value(body->position);
	if (name == "velocity")
		return MakeVec3Value(body->velocity);
	if (name == "rotation")
		return MakeVec3Value(body->orientation);
	if (name == "mass")
		return PyValue(body->mass);
	return PyValue::None();
}

bool PhysicsBodyProxy::SetAttr(const String& name, const PyValue& v) {
	if (!body)
		return false;
	if (name == "position") {
		vec3 pos;
		if (PyValueToVec3(v, pos)) {
			body->position = pos;
			body->SynchCollisionVolumes();
			return true;
		}
	}
	if (name == "velocity") {
		vec3 vel;
		if (PyValueToVec3(v, vel)) {
			body->velocity = vel;
			return true;
		}
	}
	if (name == "rotation") {
		vec3 rot;
		if (PyValueToVec3(v, rot)) {
			body->orientation = rot;
			body->SynchCollisionVolumes();
			return true;
		}
	}
	if (name == "mass") {
		body->mass = (float)v.AsDouble();
		return true;
	}
	return false;
}

static void SyncBindingsToScene(ExecScriptRuntime* rt) {
	if (!rt || !rt->state)
		return;
	for (const ExecScriptRuntime::PhysicsBinding& bind : rt->physics.bindings) {
		if (!bind.node || !bind.body)
			continue;
		GeomTransform* tr = GetNodeTransform(*bind.node);
		if (!tr)
			continue;
		tr->position = bind.body->position;
		if (bind.sync_rotation)
			tr->orientation = AxesQuat(bind.body->orientation);
	}
	rt->state->UpdateObjects();
	if (rt->WhenChanged)
		rt->WhenChanged();
}

static PyValue Physics_CreateWorld(const Vector<PyValue>& args, void* user_data) {
	ExecScriptRuntime* rt = (ExecScriptRuntime*)user_data;
	if (!rt)
		return PyValue::None();
	SoftPhys::World& world = rt->physics.worlds.Add();
	world.SetGravity(GRAVITY_CONST);
	return PyValue(new PhysicsWorldProxy(rt, &world));
}

static PyValue Physics_CreateSpace(const Vector<PyValue>& args, void* user_data) {
	if (args.GetCount() < 1 || !args[0].IsUserData())
		return PyValue::None();
	ExecScriptRuntime* rt = (ExecScriptRuntime*)user_data;
	if (!rt)
		return PyValue::None();
	SoftPhys::World* world = GetWorldFromPy(args[0]);
	if (!world)
		return PyValue::None();
	SoftPhys::Space& space = rt->physics.spaces.Add();
	space.SetWorld(*world);
	return PyValue(new PhysicsSpaceProxy(rt, &space));
}

static SoftPhys::RigidbodyVolume* CreateRigidBody(ExecScriptRuntime* rt, SoftPhys::Space& space, int type, double mass, const vec3& size, const vec3& pos) {
	SoftPhys::RigidbodyVolume& body = rt->physics.bodies.Add();
	body.type = type;
	body.mass = (float)mass;
	body.position = pos;
	body.velocity = vec3(0, 0, 0);
	body.orientation = vec3(0, 0, 0);
	if (type == RIGIDBODY_TYPE_BOX) {
		body.box.size = size * 0.5f;
	}
	if (type == RIGIDBODY_TYPE_SPHERE) {
		body.sphere.radius = size[0];
	}
	body.SynchCollisionVolumes();
	space.Add(body);
	return &body;
}

static PyValue Physics_CreateBox(const Vector<PyValue>& args, void* user_data) {
	if (args.GetCount() < 2 || !args[0].IsUserData())
		return PyValue::None();
	ExecScriptRuntime* rt = (ExecScriptRuntime*)user_data;
	if (!rt)
		return PyValue::None();
	SoftPhys::Space* space = GetSpaceFromPy(args[0]);
	if (!space)
		return PyValue::None();
	vec3 size(1, 1, 1);
	PyValueToVec3(args[1], size);
	double mass = args.GetCount() > 2 ? args[2].AsDouble() : 1.0;
	vec3 pos(0, 0, 0);
	if (args.GetCount() > 3)
		PyValueToVec3(args[3], pos);
	SoftPhys::RigidbodyVolume* body = CreateRigidBody(rt, *space, RIGIDBODY_TYPE_BOX, mass, size, pos);
	return PyValue(new PhysicsBodyProxy(rt, body));
}

static PyValue Physics_CreateSphere(const Vector<PyValue>& args, void* user_data) {
	if (args.GetCount() < 2 || !args[0].IsUserData())
		return PyValue::None();
	ExecScriptRuntime* rt = (ExecScriptRuntime*)user_data;
	if (!rt)
		return PyValue::None();
	SoftPhys::Space* space = GetSpaceFromPy(args[0]);
	if (!space)
		return PyValue::None();
	double radius = args[1].AsDouble();
	double mass = args.GetCount() > 2 ? args[2].AsDouble() : 1.0;
	vec3 pos(0, 0, 0);
	if (args.GetCount() > 3)
		PyValueToVec3(args[3], pos);
	vec3 size((float)radius, 0, 0);
	SoftPhys::RigidbodyVolume* body = CreateRigidBody(rt, *space, RIGIDBODY_TYPE_SPHERE, mass, size, pos);
	return PyValue(new PhysicsBodyProxy(rt, body));
}

static PyValue Physics_Bind(const Vector<PyValue>& args, void* user_data) {
	if (args.GetCount() < 2 || !args[0].IsUserData())
		return PyValue::False();
	ExecScriptRuntime* rt = (ExecScriptRuntime*)user_data;
	if (!rt)
		return PyValue::False();
	DisplayObjectProxy& obj = (DisplayObjectProxy&)args[0].GetUserData();
	SoftPhys::RigidbodyVolume* body = GetBodyFromPy(args[1]);
	if (!obj.node || !body)
		return PyValue::False();
	bool sync_rotation = args.GetCount() > 2 ? args[2].IsTrue() : true;
	ExecScriptRuntime::PhysicsBinding& bind = rt->physics.bindings.Add();
	bind.node = obj.node;
	bind.body = body;
	bind.sync_rotation = sync_rotation;
	GeomTransform* tr = GetNodeTransform(*obj.node);
	if (tr) {
		body->position = tr->position;
		body->orientation = GetQuatAxes(tr->orientation);
		body->SynchCollisionVolumes();
	}
	return PyValue::True();
}

static PyValue Physics_Step(const Vector<PyValue>& args, void* user_data) {
	if (args.GetCount() < 2 || !args[0].IsUserData())
		return PyValue::None();
	ExecScriptRuntime* rt = (ExecScriptRuntime*)user_data;
	if (!rt)
		return PyValue::None();
	SoftPhys::World* world = GetWorldFromPy(args[0]);
	if (!world)
		return PyValue::None();
	double dt = args[1].AsDouble();
	world->Step((float)dt);
	SyncBindingsToScene(rt);
	return PyValue::None();
}

static bool PyValueToVec3List(const PyValue& v, Vector<vec3>& out) {
	out.Clear();
	if (v.GetType() != PY_LIST && v.GetType() != PY_TUPLE)
		return false;
	int count = v.GetCount();
	for (int i = 0; i < count; i++) {
		vec3 pt(0, 0, 0);
		if (!PyValueToVec3(v.GetItem(i), pt))
			return false;
		out.Add(pt);
	}
	return true;
}

static PyValue DriverAI_ComputeRoute(const Vector<PyValue>& args, void* user_data) {
	if (args.GetCount() < 3)
		return PyValue::None();
	ExecScriptRuntime* rt = (ExecScriptRuntime*)user_data;
	if (!rt)
		return PyValue::None();
	Vector<vec3> pts;
	if (!PyValueToVec3List(args[0], pts))
		return PyValue::None();
	int start_idx = args[1].AsInt();
	int goal_idx = args[2].AsInt();
	double max_edge = args.GetCount() > 3 ? args[3].AsDouble() : 0.0;
	Vector<int> route = rt->driver_ai.ComputeRoute(pts, start_idx, goal_idx, max_edge);
	PyValue list = PyValue::List();
	for (int idx : route)
		list.Add(PyValue(idx));
	return list;
}

static PyValue DriverAI_PlanAction(const Vector<PyValue>& args, void* user_data) {
	if (args.GetCount() < 3)
		return PyValue::None();
	ExecScriptRuntime* rt = (ExecScriptRuntime*)user_data;
	if (!rt)
		return PyValue::None();
	vec3 car_pos(0, 0, 0);
	vec3 target_pos(0, 0, 0);
	if (!PyValueToVec3(args[0], car_pos))
		return PyValue::None();
	float heading = (float)args[1].AsDouble();
	if (!PyValueToVec3(args[2], target_pos))
		return PyValue::None();
	float arrive = args.GetCount() > 3 ? (float)args[3].AsDouble() : 1.5f;
	float ang_thresh = args.GetCount() > 4 ? (float)args[4].AsDouble() : 0.3f;
	int action = rt->driver_ai.PlanAction(car_pos, heading, target_pos, arrive, ang_thresh);
	return PyValue(action);
}

static PyValue ExecTrace(const Vector<PyValue>& args, void* user_data) {
	String out;
	for (int i = 0; i < args.GetCount(); i++) {
		if (i)
			out << " ";
		out << args[i].ToString();
	}
	Cout() << out << "\n";
	return PyValue::None();
}

static PyValue ExecGetTimer(const Vector<PyValue>& args, void* user_data) {
	ExecScriptRuntime* rt = (ExecScriptRuntime*)user_data;
	if (!rt || !rt->anim)
		return PyValue(0);
	return PyValue((int64)(rt->anim->time * 1000.0));
}

static PyValue ExecRandom(const Vector<PyValue>& args, void* user_data) {
	if (args.GetCount() == 0)
		return PyValue(Randomf());
	if (args.GetCount() == 1)
		return PyValue(Randomf() * args[0].AsDouble());
	if (args.GetCount() >= 2) {
		double minv = args[0].AsDouble();
		double maxv = args[1].AsDouble();
		return PyValue(minv + (maxv - minv) * Randomf());
	}
	return PyValue(Randomf());
}

static PyValue ExecExit(const Vector<PyValue>& args, void* user_data) {
	ExecScriptRuntime* rt = (ExecScriptRuntime*)user_data;
	if (rt)
		rt->RequestExit();
	return PyValue::None();
}

static bool CompilePySource(const String& code, const String& filename, Vector<PyIR>& out_ir, String& err) {
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

static bool RunPyIR(PyVM& vm, const Vector<PyIR>& ir, String& err) {
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

void ExecScriptRuntime::Init(GeomWorldState* ws, GeomAnim* a) {
	state = ws;
	anim = a;
}

void ExecScriptRuntime::SetProjectDir(const String& dir) {
	project_dir = NormalizePath(dir);
}

void ExecScriptRuntime::SetExportDir(const String& dir) {
	export_dir = NormalizePath(dir);
}

void ExecScriptRuntime::SetDataDir(const String& dir) {
	data_dir = dir;
}

void ExecScriptRuntime::SetManifest(const ExecutionManifest& manifest, const String& base_dir) {
	SetProjectDir(manifest.project_dir);
	SetExportDir(base_dir);
	SetDataDir(manifest.data_dir);
	file_map.Clear();
	for (const ExecutionFileMapping& item : manifest.files) {
		ExecutionFileMapping& map = file_map.Add();
		map.type = item.type;
		map.source = item.source;
		map.exported = item.exported;
	}
}

String ExecScriptRuntime::ResolvePath(const String& path) const {
	if (path.IsEmpty())
		return String();
	if (IsFullPath(path) && FileExists(path))
		return NormalizePath(path);
	if (!project_dir.IsEmpty()) {
		String abs = NormalizePath(AppendFileName(project_dir, path));
		if (FileExists(abs))
			return abs;
	}
	if (!export_dir.IsEmpty()) {
		String abs = NormalizePath(AppendFileName(export_dir, path));
		if (FileExists(abs))
			return abs;
	}
	String rel = path;
	for (const ExecutionFileMapping& map : file_map) {
		if (!map.source.IsEmpty()) {
			String norm_source = IsFullPath(map.source) ? NormalizePath(map.source) : map.source;
			if (norm_source == rel || norm_source == NormalizePath(rel))
				return NormalizePath(AppendFileName(export_dir, map.exported));
		}
		if (!map.exported.IsEmpty() && map.exported == rel)
			return NormalizePath(AppendFileName(export_dir, map.exported));
	}
	return path;
}

String ExecScriptRuntime::ResolveScriptPath(const String& rel) const {
	return ResolvePath(rel);
}

static void CollectScriptsFromNode(VfsValue& node, Vector<GeomScript*>& out) {
	static bool init = (TypedStringHasher<GeomScript>("GeomScript"), true);
	hash_t script_hash = AsTypeHash<GeomScript>();
	for (auto& sub : node.sub) {
		if (IsVfsType(sub, script_hash))
			out.Add(&sub.GetExt<GeomScript>());
	}
}

void ExecScriptRuntime::RegisterScriptVM(PyVM& vm) {
	::RegisterGeometry(vm);
	{
		PY_MODULE(exec, vm)
		PY_MODULE_FUNC(trace, ExecTrace, this);
		PY_MODULE_FUNC(get_timer, ExecGetTimer, this);
		PY_MODULE_FUNC(random, ExecRandom, this);
		PY_MODULE_FUNC(exit, ExecExit, this);
	}
	PyValue stage_obj = PyValue(new StageProxy(this, &vm));
	vm.GetGlobalsRW().GetDictRW().GetAdd(PyValue("stage")) = stage_obj;
	vm.GetGlobalsRW().GetDictRW().GetAdd(PyValue("trace")) = PyValue::Function("trace", ExecTrace, this);
	vm.GetGlobalsRW().GetDictRW().GetAdd(PyValue("getTimer")) = PyValue::Function("getTimer", ExecGetTimer, this);
	vm.GetGlobalsRW().GetDictRW().GetAdd(PyValue("random")) = PyValue::Function("random", ExecRandom, this);
	vm.GetGlobalsRW().GetDictRW().GetAdd(PyValue("input")) = PyValue(new InputProxy(this));
	vm.GetGlobalsRW().GetDictRW().GetAdd(PyValue("camera")) = PyValue(new CameraProxy(this));
	{
		PY_MODULE(physics, vm)
		PY_MODULE_FUNC(create_world, Physics_CreateWorld, this);
		PY_MODULE_FUNC(create_space, Physics_CreateSpace, this);
		PY_MODULE_FUNC(create_box, Physics_CreateBox, this);
		PY_MODULE_FUNC(create_sphere, Physics_CreateSphere, this);
		PY_MODULE_FUNC(bind, Physics_Bind, this);
		PY_MODULE_FUNC(step, Physics_Step, this);
	}
	{
		PY_MODULE(driver_ai, vm)
		PY_MODULE_FUNC(compute_route, DriverAI_ComputeRoute, this);
		PY_MODULE_FUNC(plan_action, DriverAI_PlanAction, this);
	}
	PyValue root_obj = PyValue::None();
	if (state && state->HasActiveScene())
		root_obj = MakeDisplayObject(this, &state->GetActiveScene().val, &vm);
	vm.GetGlobalsRW().GetDictRW().GetAdd(PyValue("root")) = root_obj;
	vm.GetGlobalsRW().GetDictRW().GetAdd(PyValue("this")) = root_obj;
}

void ExecScriptRuntime::EnsureScriptInstances() {
	if (!state || !state->prj || !state->HasActiveScene())
		return;
	Vector<GeomScript*> scripts;
	GeomScene& scene = state->GetActiveScene();
	CollectScriptsFromNode(scene.val, scripts);
	for (auto& sub : scene.val.sub) {
		if (IsVfsType(sub, HashGeomDirectory()))
			CollectScriptsFromNode(sub, scripts);
	}
	for (GeomObject& obj : GeomObjectCollection(scene))
		CollectScriptsFromNode(obj.val, scripts);
	for (int i = script_instances.GetCount() - 1; i >= 0; i--) {
		bool found = false;
		for (GeomScript* script : scripts) {
			if (script == script_instances[i].script) {
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

void ExecScriptRuntime::UpdateScriptInstance(ScriptInstance& inst, bool force_reload) {
	if (!inst.script)
		return;
	GeomScript& script = *inst.script;
	if (!script.enabled)
		return;
	String abs = ResolveScriptPath(script.file);
	if (abs.IsEmpty() || !FileExists(abs))
		return;
	Time mod = FileGetTime(abs);
	if (inst.compile_failed && !force_reload && mod == inst.file_time)
		return;
	bool needs_reload = force_reload || !inst.loaded || mod != inst.file_time;
	if (!needs_reload)
		return;
	RemoveScriptEventHandlers(&inst.vm);
	inst.file_time = mod;
	inst.loaded = false;
	inst.compile_failed = false;
	inst.has_load = false;
	inst.has_start = false;
	inst.has_frame = false;
	String code = LoadFile(abs);
	String err;
	Vector<PyIR> ir;
	if (!CompilePySource(code, abs, ir, err)) {
		String msg = "Exec script compile failed: " + abs + " | " + err;
		LOG(msg);
		Cout() << msg << "\n";
		inst.compile_failed = true;
		return;
	}
	inst.vm.Clear();
	RegisterScriptVM(inst.vm);
	if (state && state->HasActiveScene()) {
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
		String msg = "Exec script run failed: " + abs + " | " + err;
		LOG(msg);
		Cout() << msg << "\n";
		inst.compile_failed = true;
		return;
	}
	inst.loaded = true;
	Cout() << "Exec script loaded: " << abs << "\n";
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

void ExecScriptRuntime::RunScriptOnLoad(ScriptInstance& inst, bool force) {
	if (!inst.script || !inst.loaded)
		return;
	if (!force && !inst.script->run_on_load)
		return;
	if (!inst.has_load)
		return;
	String err;
	if (!RunPyIR(inst.vm, inst.load_ir, err))
		LOG("Exec on_load failed: " + err);
}

void ExecScriptRuntime::RunScriptOnStart(ScriptInstance& inst, bool force) {
	if (!inst.script || !inst.loaded)
		return;
	if (!force && !inst.script->run_on_load)
		return;
	if (!inst.has_start)
		return;
	String err;
	if (!RunPyIR(inst.vm, inst.start_ir, err))
		LOG("Exec on_start failed: " + err);
}

void ExecScriptRuntime::RunScriptFrame(ScriptInstance& inst, double dt) {
	if (!inst.script || !inst.script->enabled || !inst.script->run_every_frame)
		return;
	if (!inst.loaded || !inst.has_frame)
		return;
	inst.vm.GetGlobalsRW().GetDictRW().GetAdd(PyValue("__dt__")) = PyValue(dt);
	String err;
	if (!RunPyIR(inst.vm, inst.frame_ir, err))
		LOG("Exec on_frame failed: " + err);
}

void ExecScriptRuntime::AddScriptEventHandler(const String& event, PyVM* vm, VfsValue* node, const PyValue& func) {
	if (!vm || (!func.IsFunction() && !func.IsBoundMethod()))
		return;
	ScriptEventHandler& h = script_event_handlers.Add();
	h.event = ToLower(event);
	h.func = func;
	h.vm = vm;
	h.node = node;
}

void ExecScriptRuntime::RemoveScriptEventHandlers(PyVM* vm) {
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

void ExecScriptRuntime::DispatchScriptEvent(const String& event, VfsValue* node, const PyValue& payload) {
	String key = ToLower(event);
	for (const ScriptEventHandler& h : script_event_handlers) {
		if (h.event != key)
			continue;
		if (node && h.node && h.node != node)
			continue;
		if (!node && h.node) {
		}
		if (!h.vm)
			continue;
		Vector<PyValue> args;
		args.Add(payload);
		String err;
		if (!RunPyCallback(*h.vm, h.func, args, err))
			LOG("Exec event failed: " + err);
	}
}

void ExecScriptRuntime::DispatchInputEvent(const String& type, const Point& p, dword flags, int key, int view_i) {
	if (type.StartsWith("mouse")) {
		input.SetMouse(p, flags);
		if (type == "mouseDown")
			input.SetMouseDown(p, flags);
		else if (type == "mouseUp")
			input.SetMouseUp(p, flags);
		else if (type == "mouseWheel")
			input.AddWheel(key, flags);
	}
	else if (type == "keyDown") {
		int k = key & 0xFFFF;
		input.SetKey(k, true);
		if (k >= 'a' && k <= 'z')
			input.SetKey(k - 'a' + 'A', true);
		if (k >= 'A' && k <= 'Z')
			input.SetKey(k - 'A' + 'a', true);
	}
	else if (type == "keyUp") {
		int k = key & 0xFFFF;
		input.SetKey(k, false);
		if (k >= 'a' && k <= 'z')
			input.SetKey(k - 'a' + 'A', false);
		if (k >= 'A' && k <= 'Z')
			input.SetKey(k - 'A' + 'a', false);
	}
	PyValue payload = PyValue::Dict();
	payload.SetItem(PyValue("type"), PyValue(type));
	payload.SetItem(PyValue("x"), PyValue(p.x));
	payload.SetItem(PyValue("y"), PyValue(p.y));
	payload.SetItem(PyValue("flags"), PyValue((int64)flags));
	payload.SetItem(PyValue("key"), PyValue(key));
	payload.SetItem(PyValue("view"), PyValue(view_i));
	payload.SetItem(PyValue("dx"), PyValue((int64)input.mouse_delta.x));
	payload.SetItem(PyValue("dy"), PyValue((int64)input.mouse_delta.y));
	if (anim) {
		payload.SetItem(PyValue("time"), PyValue(anim->time));
		payload.SetItem(PyValue("frame"), PyValue(anim->position));
	}
	DispatchScriptEvent(type, nullptr, payload);
}

void ExecScriptRuntime::DispatchFrameEvents(double dt) {
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
			LOG("Exec enterFrame failed: " + err);
	}
}

void ExecScriptRuntime::Update(double dt) {
	EnsureScriptInstances();
	for (auto& inst : script_instances)
		UpdateScriptInstance(inst, false);
	for (auto& inst : script_instances)
		RunScriptFrame(inst, dt);
	DispatchFrameEvents(dt);
	input.BeginFrame();
}

void ExecScriptRuntime::ReloadScripts(bool force_reload) {
	EnsureScriptInstances();
	for (auto& inst : script_instances)
		UpdateScriptInstance(inst, force_reload);
}

END_UPP_NAMESPACE

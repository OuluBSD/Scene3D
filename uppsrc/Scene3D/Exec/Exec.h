#ifndef _Scene3D_Exec_Exec_h_
#define _Scene3D_Exec_Exec_h_

#include <Scene3D/Core/Core.h>
#include <Scene3D/IO/IO.h>
#include <ByteVM/ByteVM.h>
#include <ByteVM/PyBindings.h>
#include <SoftPhys/SoftPhys.h>
#include <Vfs/Core/WorldState.h>
#include <Vfs/Core/VfsValueExt.h>
#include <AI/Core/Base/Base.h>

NAMESPACE_UPP

struct ExecutionFileMapping : Moveable<ExecutionFileMapping> {
	String type;
	String source;
	String exported;

	void Jsonize(JsonIO& jio);
};

struct ExecutionManifest : Moveable<ExecutionManifest> {
	String version;
	String mode;
	String exported_utc;
	String project_name;
	String scene3d;
	String project_dir;
	String data_dir;
	Vector<ExecutionFileMapping> files;

	void Jsonize(JsonIO& jio);
};

bool LoadExecutionManifest(const String& path, ExecutionManifest& out);
bool SaveExecutionManifest(const String& path, ExecutionManifest& manifest, bool pretty = true);

struct ExecInputState {
	Point mouse_pos = Point(0, 0);
	Point mouse_delta = Point(0, 0);
	int wheel_delta = 0;
	bool mouse_down = false;
	dword mouse_flags = 0;
	Index<int> keys_down;
	Index<int> keys_pressed;
	Index<int> keys_released;

	void BeginFrame();
	void SetMouse(Point p, dword flags);
	void SetMouseDown(Point p, dword flags);
	void SetMouseUp(Point p, dword flags);
	void AddWheel(int delta, dword flags);
	void SetKey(int key, bool down);
	bool IsKeyDown(int key) const { return keys_down.Find(key) >= 0; }
	bool WasKeyPressed(int key) const { return keys_pressed.Find(key) >= 0; }
	bool WasKeyReleased(int key) const { return keys_released.Find(key) >= 0; }
};

struct ExecScriptRuntime {
	GeomWorldState* state = nullptr;
	GeomAnim* anim = nullptr;
	String project_dir;
	String export_dir;
	String data_dir;
	ExecInputState input;
	bool exit_requested = false;

	struct PhysicsBinding : Moveable<PhysicsBinding> {
		VfsValue* node = nullptr;
		SoftPhys::RigidbodyVolume* body = nullptr;
		bool sync_rotation = true;
	};

	struct PhysicsState {
		Array<SoftPhys::World> worlds;
		Array<SoftPhys::Space> spaces;
		Array<SoftPhys::RigidbodyVolume> bodies;
		Vector<PhysicsBinding> bindings;
	};

	PhysicsState physics;

	struct DriverAI {
		ActionPlanner ap;
		One<ActionPlannerWrapper> wrapper;
		int atom_target_left = -1;
		int atom_target_right = -1;
		int atom_target_ahead = -1;
		int atom_at_target = -1;
		int act_turn_left = -1;
		int act_turn_right = -1;
		int act_go_straight = -1;
		int act_stop = -1;
		bool inited = false;

		void Init();
		int PlanAction(const vec3& car_pos, float heading, const vec3& target_pos, float arrive_radius, float angle_thresh);
		Vector<int> ComputeRoute(const Vector<vec3>& points, int start_idx, int goal_idx, double max_edge);
	};

	DriverAI driver_ai;

	struct ScriptInstance {
		GeomScript* script = nullptr;
		VfsValue* owner = nullptr;
		String abs_path;
		Time file_time;
		PyVM vm;
		bool loaded = false;
		bool compile_failed = false;
		bool has_load = false;
		bool has_start = false;
		bool has_frame = false;
		Vector<PyIR> main_ir;
		Vector<PyIR> load_ir;
		Vector<PyIR> start_ir;
		Vector<PyIR> frame_ir;
	};

	struct ScriptEventHandler : Moveable<ScriptEventHandler> {
		String event;
		PyValue func;
		PyVM* vm = nullptr;
		VfsValue* node = nullptr;
	};

	Vector<ExecutionFileMapping> file_map;
	Array<ScriptInstance> script_instances;
	Vector<ScriptEventHandler> script_event_handlers;

	Callback WhenChanged;

	void Init(GeomWorldState* ws, GeomAnim* a);
	void SetProjectDir(const String& dir);
	void SetExportDir(const String& dir);
	void SetDataDir(const String& dir);
	void SetManifest(const ExecutionManifest& manifest, const String& base_dir);
	String ResolvePath(const String& path) const;
	String ResolveScriptPath(const String& rel) const;

	void RegisterScriptVM(PyVM& vm);
	void EnsureScriptInstances();
	void UpdateScriptInstance(ScriptInstance& inst, bool force_reload);
	void RunScriptOnLoad(ScriptInstance& inst, bool force);
	void RunScriptOnStart(ScriptInstance& inst, bool force);
	void RunScriptFrame(ScriptInstance& inst, double dt);
	void AddScriptEventHandler(const String& event, PyVM* vm, VfsValue* node, const PyValue& func);
	void RemoveScriptEventHandlers(PyVM* vm);
	void DispatchScriptEvent(const String& event, VfsValue* node, const PyValue& payload);
	void DispatchInputEvent(const String& type, const Point& p, dword flags, int key, int view_i);
	void DispatchFrameEvents(double dt);
	void Update(double dt);
	void ReloadScripts(bool force_reload);
	void RequestExit() { exit_requested = true; }
};

END_UPP_NAMESPACE

#endif

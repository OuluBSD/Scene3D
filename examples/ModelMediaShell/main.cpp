#include <CtrlLib/CtrlLib.h>
#include <Scene3D/Core/Core.h>
#include <Scene3D/IO/IO.h>
#include <Scene3D/Render/Render.h>
#include <Scene3D/Exec/Exec.h>

#ifdef flagGUI
using namespace Upp;

namespace {

void FixCamera(GeomCamera& cam);
void PrintModelBounds(const String& name, const String& path, const Model& mdl);

bool LoadExecutionProjectCommon(const String& manifest_path,
                                Scene3DDocument& doc,
                                ExecutionManifest& manifest,
                                GeomWorldState& state,
                                GeomAnim& anim,
                                ExecScriptRuntime& runtime,
                                String& base_dir) {
	if (!LoadExecutionManifest(manifest_path, manifest)) {
		Cout() << "Failed to load execution manifest.\n";
		return false;
	}
	base_dir = GetFileFolder(manifest_path);
	String scene_path = manifest.scene3d;
	if (!IsFullPath(scene_path))
		scene_path = AppendFileName(base_dir, scene_path);
	bool loaded_scene = LoadScene3DJson(scene_path, doc);
	if (!loaded_scene)
		loaded_scene = LoadScene3DBin(scene_path, doc);
	if (!loaded_scene) {
		Cout() << "Failed to load scene3d.\n";
		return false;
	}
	if (!doc.project) {
		Cout() << "Scene3D document missing project.\n";
		return false;
	}
	state.prj = doc.project;
	state.active_scene = doc.active_scene;
	if (state.active_scene < 0 && state.prj->GetSceneCount() > 0)
		state.active_scene = 0;
	if (doc.focus)
		VisitCopy(*doc.focus, state.GetFocus());
	if (doc.program)
		VisitCopy(*doc.program, state.GetProgram());
	FixCamera(state.GetFocus());
	FixCamera(state.GetProgram());
	{
		GeomCamera& program = state.GetProgram();
		if (program.position.GetLength() < 0.001f) {
			program.position = vec3(0, 5, 14);
			mat4 view = LookAt(program.position, vec3(0, 0, 5), vec3(0, 1, 0));
			mat4 world = view.GetInverse();
			program.orientation = MatQuat(world);
		}
	}
	state.UpdateObjects();
	anim.Reset();
	anim.Play();
	runtime.Init(&state, &anim);
	runtime.SetManifest(manifest, base_dir);
	runtime.ReloadScripts(true);
	return true;
}

void FixCamera(GeomCamera& cam) {
	auto fixf = [&](float& v, float def) {
		if (IsNull(v))
			v = def;
	};
	auto fixv3 = [&](vec3& v, const vec3& def) {
		for (int i = 0; i < 3; i++) {
			if (IsNull(v[i]))
				v[i] = def[i];
		}
	};
	auto fixq = [&](quat& q) {
		bool bad = false;
		for (int i = 0; i < 4; i++)
			if (IsNull(q[i]))
				bad = true;
		if (bad)
			q = Identity<quat>();
	};
	fixv3(cam.position, vec3(0));
	fixq(cam.orientation);
	fixf(cam.scale, 1.0f);
	fixf(cam.fov, 120.0f);
	if (cam.fov <= 0.1f)
		cam.fov = 120.0f;
	if (cam.scale == 0)
		cam.scale = 1.0f;
	bool is_identity = fabs(cam.orientation[0]) < 1e-4f &&
	                   fabs(cam.orientation[1]) < 1e-4f &&
	                   fabs(cam.orientation[2]) < 1e-4f &&
	                   fabs(cam.orientation[3] - 1.0f) < 1e-4f;
	if (is_identity && cam.position.GetLength() > 0.1f) {
		mat4 view = LookAt(cam.position, vec3(0, 0, 0), vec3(0, 1, 0));
		mat4 world = view.GetInverse();
		cam.orientation = MatQuat(world);
	}
}

void PrintModelBounds(const String& name, const String& path, const Model& mdl) {
	bool has = false;
	vec3 mn, mx;
	for (const Mesh& mesh : mdl.meshes) {
		vec3 tmin, tmax;
		mesh.GetMinMax(tmin, tmax);
		if (!has) {
			mn = tmin;
			mx = tmax;
			has = true;
		}
		else {
			mn[0] = min(mn[0], tmin[0]);
			mn[1] = min(mn[1], tmin[1]);
			mn[2] = min(mn[2], tmin[2]);
			mx[0] = max(mx[0], tmax[0]);
			mx[1] = max(mx[1], tmax[1]);
			mx[2] = max(mx[2], tmax[2]);
		}
	}
	if (!has) {
		Cout() << "ModelBounds: " << name << " path=" << path << " meshes=0\n";
		return;
	}
	vec3 dims = mx - mn;
	Cout() << "ModelBounds: " << name << " path=" << path
	       << " min=(" << mn[0] << "," << mn[1] << "," << mn[2] << ")"
	       << " max=(" << mx[0] << "," << mx[1] << "," << mx[2] << ")"
	       << " size=(" << dims[0] << "," << dims[1] << "," << dims[2] << ")"
	       << " meshes=" << mdl.meshes.GetCount() << "\n";
}

void LoadModels(GeomWorldState& state, Scene3DDocument& doc, ExecScriptRuntime& runtime) {
	for (GeomScene& scene : doc.project->val.Sub<GeomScene>()) {
		GeomObjectCollection objects(scene);
		for (GeomObject& obj : objects) {
			if (!obj.IsModel())
				continue;
			if (obj.mdl)
				continue;
			if (obj.asset_ref.IsEmpty()) {
				ModelBuilder mb;
				mb.AddBox(0, 1, 1);
				obj.mdl = mb.Detach();
				PrintModelBounds(obj.name, "<box>", *obj.mdl);
				continue;
			}
			String resolved = runtime.ResolvePath(obj.asset_ref);
			ModelLoader loader;
			if (!resolved.IsEmpty() && loader.LoadModel(resolved)) {
				obj.mdl = pick(loader.model);
				PrintModelBounds(obj.name, resolved, *obj.mdl);
			}
			else {
				ModelBuilder mb;
				mb.AddBox(0, 1, 1);
				obj.mdl = mb.Detach();
				PrintModelBounds(obj.name, "<box>", *obj.mdl);
			}
		}
	}
	state.UpdateObjects();
}

bool ParseSizeArg(const String& value, Size& out) {
	Vector<String> parts = Split(value, 'x');
	if (parts.GetCount() != 2)
		parts = Split(value, 'X');
	if (parts.GetCount() != 2)
		parts = Split(value, ',');
	if (parts.GetCount() != 2)
		return false;
	int w = StrInt(parts[0]);
	int h = StrInt(parts[1]);
	if (w <= 0 || h <= 0)
		return false;
	out = Size(w, h);
	return true;
}

int HeadlessPrompt(Event<const String&>, const char* title, const Image&, const char* qtf, bool,
                   const char* button1, const char*, const char*,
                   int, Image, Image, Image) {
	Cout() << "Prompt: " << (title ? title : "") << " | "
	       << (qtf ? qtf : "") << " | default=" << (button1 ? button1 : "OK") << "\n";
	return 1;
}

static GeomObject* FindObjectRecursive(GeomScene& scene, const String& name) {
	for (GeomObject& obj : GeomObjectCollection(scene)) {
		if (obj.name == name)
			return &obj;
	}
	return nullptr;
}

}

enum RendererKind {
	RENDER_V1,
	RENDER_V2,
	RENDER_V2_OGL
};

class ModelMediaShell : public TopWindow {
	typedef ModelMediaShell CLASSNAME;
	One<EditRendererBase> renderer;
	EditRendererBase* renderer_ctrl = nullptr;
	Scene3DRenderConfig conf;
	Scene3DRenderContext ctx;
	VfsValue state_val;
	GeomWorldState* state = nullptr;
	VfsValue anim_val;
	GeomAnim* anim = nullptr;
	GeomVideo video;
	Scene3DDocument doc;
	ExecutionManifest manifest;
	ExecScriptRuntime runtime;
	TimeStop frame_ts;
	TimeStop fps_ts;
	double fps_accum = 0.0;
	int fps_frames = 0;
	bool loaded = false;
	bool mouse_captured = false;
	bool capture_mouse_enabled = true;
	bool relative_mouse = false;
	bool relative_mouse_center = false;
	bool mouse_pos_valid = false;
	Point last_mouse_pos;
	Point rel_mouse_pos = Point(0, 0);

	void RefreshFrame();
	void OnChanged();
	bool Key(dword key, int count) override;
	void MouseMove(Point p, dword keyflags) override;
	void LeftDown(Point p, dword keyflags) override;
	void LeftUp(Point p, dword keyflags) override;
	void RightDown(Point p, dword keyflags) override;
	void RightUp(Point p, dword keyflags) override;
	void MouseWheel(Point p, int zdelta, dword keyflags) override;
	void MiddleDown(Point p, dword keyflags) override;
	void MiddleUp(Point p, dword keyflags) override;

public:
	ModelMediaShell(RendererKind kind);
	bool LoadExecutionProject(const String& manifest_path);
	void SetCaptureMouseEnabled(bool b) { capture_mouse_enabled = b; }
	void SetRelativeMouse(bool b) { relative_mouse = b; mouse_pos_valid = false; rel_mouse_pos = Point(0, 0); }
	void SetRelativeMouseCenter(bool b) { relative_mouse_center = b; mouse_pos_valid = false; rel_mouse_pos = Point(0, 0); }
};

ModelMediaShell::ModelMediaShell(RendererKind kind) {
	Title("ModelMediaShell");
	Sizeable();
	switch (kind) {
	case RENDER_V1:
		renderer.Create<EditRendererV1>();
		break;
	case RENDER_V2:
		renderer.Create<EditRendererV2>();
		break;
	default:
		renderer.Create<EditRendererV2_Ogl>();
		break;
	}
	renderer_ctrl = renderer.Get();
	Add(renderer_ctrl->SizePos());
	state = &state_val.CreateExt<GeomWorldState>();
	anim = &anim_val.CreateExt<GeomAnim>();
	ctx.conf = &conf;
	ctx.state = state;
	ctx.anim = anim;
	ctx.video = &video;
	ctx.show_hud = false;
	ctx.show_hud_help = false;
	ctx.selection_gizmo_enabled = false;
	renderer_ctrl->ctx = &ctx;
	renderer_ctrl->SetViewMode(VIEWMODE_PERSPECTIVE);
	renderer_ctrl->SetCameraSource(CAMSRC_PROGRAM);
	renderer_ctrl->SetCameraInputEnabled(false);
	renderer_ctrl->WhenInput = [this](const String& type, const Point& p, dword flags, int key) {
		runtime.DispatchInputEvent(type, p, flags, key, 0);
	};
	anim->state = state;
	runtime.WhenChanged = THISBACK(OnChanged);
}

void ModelMediaShell::OnChanged() {
	if (renderer_ctrl)
		renderer_ctrl->Refresh();
}

bool ModelMediaShell::LoadExecutionProject(const String& manifest_path) {
	String base_dir;
	if (!LoadExecutionProjectCommon(manifest_path, doc, manifest, *state, *anim, runtime, base_dir)) {
		PromptOK("Failed to load execution manifest or scene.");
		return false;
	}
	LoadModels(*state, doc, runtime);
	frame_ts.Reset();
	fps_ts.Reset();
	if (anim)
		anim->Play();
	SetTimeCallback(-1000/60, THISBACK(RefreshFrame));
	PostCallback(THISBACK(RefreshFrame));
	if (renderer_ctrl)
		renderer_ctrl->SetFocus();
	loaded = true;
	return true;
}

bool ModelMediaShell::Key(dword key, int count) {
	if (loaded) {
		if ((key & K_KEYUP) == 0 && (key & 0xFFFF) == K_ESCAPE && mouse_captured) {
			ReleaseCapture();
			mouse_captured = false;
		}
		String desc = GetKeyDesc(key);
		Cout() << "ModelMediaShell Key: key=" << key << " desc=" << desc
		       << " up=" << ((key & K_KEYUP) ? 1 : 0) << "\n";
		Cout().Flush();
		runtime.DispatchInputEvent((key & K_KEYUP) ? "keyUp" : "keyDown", Point(0, 0), 0, (int)key, 0);
	}
	return TopWindow::Key(key, count);
}

void ModelMediaShell::MouseMove(Point p, dword keyflags) {
	if (loaded) {
		if (relative_mouse && capture_mouse_enabled) {
			Point center = GetSize() / 2;
			if (relative_mouse_center) {
				Point delta = p - center;
				rel_mouse_pos += delta;
				last_mouse_pos = center;
				mouse_pos_valid = true;
				runtime.DispatchInputEvent("mouseMove", rel_mouse_pos, keyflags, 0, 0);
			} else {
				if (mouse_pos_valid) {
					Point delta = p - last_mouse_pos;
					rel_mouse_pos += delta;
					runtime.DispatchInputEvent("mouseMove", rel_mouse_pos, keyflags, 0, 0);
				}
				last_mouse_pos = p;
				mouse_pos_valid = true;
			}
		} else {
			runtime.DispatchInputEvent("mouseMove", p, keyflags, 0, 0);
		}
	}
	TopWindow::MouseMove(p, keyflags);
}

void ModelMediaShell::LeftDown(Point p, dword keyflags) {
	if (loaded)
		runtime.DispatchInputEvent("mouseDown", p, keyflags, 0, 0);
	if (capture_mouse_enabled && !mouse_captured) {
		SetCapture();
		mouse_captured = true;
	}
	TopWindow::LeftDown(p, keyflags);
}

void ModelMediaShell::LeftUp(Point p, dword keyflags) {
	if (loaded)
		runtime.DispatchInputEvent("mouseUp", p, keyflags, 0, 0);
	if (mouse_captured) {
		ReleaseCapture();
		mouse_captured = false;
	}
	TopWindow::LeftUp(p, keyflags);
}

void ModelMediaShell::RightDown(Point p, dword keyflags) {
	if (loaded)
		runtime.DispatchInputEvent("mouseDown", p, keyflags, 1, 0);
	if (capture_mouse_enabled && !mouse_captured) {
		SetCapture();
		mouse_captured = true;
	}
	TopWindow::RightDown(p, keyflags);
}

void ModelMediaShell::RightUp(Point p, dword keyflags) {
	if (loaded)
		runtime.DispatchInputEvent("mouseUp", p, keyflags, 1, 0);
	if (mouse_captured) {
		ReleaseCapture();
		mouse_captured = false;
	}
	TopWindow::RightUp(p, keyflags);
}

void ModelMediaShell::MouseWheel(Point p, int zdelta, dword keyflags) {
	if (loaded)
		runtime.DispatchInputEvent("mouseWheel", p, keyflags, zdelta, 0);
	TopWindow::MouseWheel(p, zdelta, keyflags);
}

void ModelMediaShell::MiddleDown(Point p, dword keyflags) {
	if (loaded)
		runtime.DispatchInputEvent("mouseDown", p, keyflags, 2, 0);
	if (capture_mouse_enabled && !mouse_captured) {
		SetCapture();
		mouse_captured = true;
	}
	TopWindow::MiddleDown(p, keyflags);
}

void ModelMediaShell::MiddleUp(Point p, dword keyflags) {
	if (loaded)
		runtime.DispatchInputEvent("mouseUp", p, keyflags, 2, 0);
	if (mouse_captured) {
		ReleaseCapture();
		mouse_captured = false;
	}
	TopWindow::MiddleUp(p, keyflags);
}

void ModelMediaShell::RefreshFrame() {
	if (!loaded)
		return;
	double dt = frame_ts.Elapsed();
	frame_ts.Reset();
	dt /= 1000000.0;
	if (dt <= 0)
		dt = 1.0 / 60.0;
	if (anim)
		anim->Update(dt);
	runtime.Update(dt);
	fps_accum += dt;
	fps_frames++;
	if (fps_ts.Seconds() >= 3.0) {
		double fps = fps_accum > 0 ? fps_frames / fps_accum : 0.0;
		Cout() << "FPS: " << Format("%.2f", fps) << " (" << fps_frames << " frames, "
		       << Format("%.3f", fps_accum) << " s)" << "\n";
		Cout().Flush();
		fps_ts.Reset();
		fps_accum = 0.0;
		fps_frames = 0;
	}
	if (runtime.exit_requested) {
		Close();
		return;
	}
	if (renderer_ctrl)
		renderer_ctrl->Refresh();
}

GUI_APP_MAIN {
	CommandLineArguments cmd;
	cmd.AddArg('p', "Execution manifest path", true, "path");
	cmd.AddArg("project", 0, "Execution manifest path", true, "path");
	cmd.AddArg("exec", 0, "Execution manifest path", true, "path");
	cmd.AddArg("headless", 0, "Render one frame headless and print stats", false);
	cmd.AddArg("headless-debug", 0, "Dump first triangle clip-space details when headless render fails", false);
	cmd.AddArg("dump-first-tri", 0, "Dump first triangle clip-space details when headless render fails", false);
	cmd.AddArg("headless-sweep", 0, "Sweep camera around target and validate frustum + render stats", false);
	cmd.AddArg("headless-frustum-dump", 0, "Dump frustum planes and per-object AABB hit status", false);
	cmd.AddArg("frustum-dump-frame", 0, "Dump frustum only for specific sweep frame", true, "index");
	cmd.AddArg("frustum-dump-out", 0, "Dump frustum output to file", true, "path");
	cmd.AddArg("headless-mouse-test", 0, "Run headless mouse delta test", false);
	cmd.AddArg("headless-drive-test", 0, "Run headless drive test (W key)", false);
	cmd.AddArg("headless-drive-ai-test", 0, "Run headless drive test (AI driving)", false);
	cmd.AddArg("drive-test-frames", 0, "Drive test frame count", true, "count");
	cmd.AddArg("drive-ai-frames", 0, "Drive AI test frame count", true, "count");
	cmd.AddArg('s', "Headless render size WxH", true, "size");
	cmd.AddArg("size", 0, "Headless render size WxH", true, "size");
	cmd.AddArg("sweep-frames", 0, "Headless sweep frame count", true, "count");
	cmd.AddArg("sweep-radius", 0, "Headless sweep radius", true, "radius");
	cmd.AddArg("sweep-height", 0, "Headless sweep height", true, "height");
	cmd.AddArg("sweep-target", 0, "Headless sweep target x,y,z", true, "vec3");
	cmd.AddArg("sweep-angles", 0, "Headless sweep angles start,end (degrees)", true, "range");
	cmd.AddArg("headless-snapshot", 0, "Save a headless RGBA snapshot to path", true, "path");
	cmd.AddArg("capture-mouse", 0, "Enable mouse capture during GUI run", false);
	cmd.AddArg("no-capture-mouse", 0, "Disable mouse capture during GUI run", false);
	cmd.AddArg("relative-mouse", 0, "Enable relative mouse deltas during GUI run", false);
	cmd.AddArg("relative-mouse-center", 0, "Center relative mouse deltas around window", false);
	cmd.AddArg("renderer", 0, "Renderer backend (v1|v2|v2_ogl|gfx_sw|gfx_ogl)", true, "name");
	if (!cmd.Parse(CommandLine())) {
		cmd.PrintHelp();
		return;
	}
	String exec_path;
	if (cmd.IsArg('p'))
		exec_path = cmd.GetArg('p');
	if (cmd.IsArg("project"))
		exec_path = cmd.GetArg("project");
	if (cmd.IsArg("exec"))
		exec_path = cmd.GetArg("exec");
	if (exec_path.IsEmpty()) {
		PromptOK("Provide --project <path> to project.exec.json.");
		return;
	}
	if (DirectoryExists(exec_path))
		exec_path = AppendFileName(exec_path, "project.exec.json");
	if (!FileExists(exec_path)) {
		String share_path = ShareDirFile(exec_path);
		if (!share_path.IsEmpty() && FileExists(share_path))
			exec_path = share_path;
	}
	bool headless = cmd.IsArg("headless");
	if (headless) {
		Upp::MemoryIgnoreLeaksBegin();
		RedirectPrompts(HeadlessPrompt);
		Scene3DRenderConfig conf;
		Scene3DRenderContext ctx;
		VfsValue state_val;
		VfsValue anim_val;
		GeomWorldState& state = state_val.CreateExt<GeomWorldState>();
		GeomAnim& anim = anim_val.CreateExt<GeomAnim>();
		ExecutionManifest manifest;
		Scene3DDocument doc;
		GeomVideo video;
		ExecScriptRuntime runtime;
		ctx.conf = &conf;
		ctx.state = &state;
		ctx.anim = &anim;
		ctx.video = &video;
		anim.state = &state;
		String base_dir;
		if (!LoadExecutionProjectCommon(exec_path, doc, manifest, state, anim, runtime, base_dir))
			return;
		LoadModels(state, doc, runtime);
		double dt = 1.0 / 60.0;
		anim.Update(dt);
		runtime.Update(dt);
		Size sz(1280, 720);
		if (cmd.IsArg('s'))
			ParseSizeArg(cmd.GetArg('s'), sz);
		if (cmd.IsArg("size"))
			ParseSizeArg(cmd.GetArg("size"), sz);
		Scene3DRenderStats stats;
		String debug_dump;
		bool dump_first_tri = cmd.IsArg("headless-debug") || cmd.IsArg("dump-first-tri");
		bool sweep = cmd.IsArg("headless-sweep");
		bool frustum_dump = cmd.IsArg("headless-frustum-dump");
		bool mouse_test = cmd.IsArg("headless-mouse-test");
		bool drive_test = cmd.IsArg("headless-drive-test");
		bool drive_ai_test = cmd.IsArg("headless-drive-ai-test");
		bool snapshot = cmd.IsArg("headless-snapshot");
		String snapshot_path;
		if (snapshot)
			snapshot_path = cmd.GetArg("headless-snapshot");
		int frustum_dump_frame = -1;
		if (cmd.IsArg("frustum-dump-frame"))
			frustum_dump_frame = StrInt(cmd.GetArg("frustum-dump-frame"));
		String frustum_dump_out;
		if (cmd.IsArg("frustum-dump-out"))
			frustum_dump_out = cmd.GetArg("frustum-dump-out");
		int sweep_frames = 24;
		double sweep_radius = 12.0;
		double sweep_height = 3.0;
		if (cmd.IsArg("sweep-frames"))
			sweep_frames = max(4, StrInt(cmd.GetArg("sweep-frames")));
		if (cmd.IsArg("sweep-radius"))
			sweep_radius = StrDbl(cmd.GetArg("sweep-radius"));
		if (cmd.IsArg("sweep-height"))
			sweep_height = StrDbl(cmd.GetArg("sweep-height"));
		if (!sweep) {
			if (mouse_test) {
				runtime.input.BeginFrame();
				runtime.DispatchInputEvent("mouseMove", Point(10, 10), 0, 0, 0);
				runtime.DispatchInputEvent("mouseMove", Point(25, 14), 0, 0, 0);
				int dx = runtime.input.mouse_delta.x;
				int dy = runtime.input.mouse_delta.y;
				Cout() << "MouseTest: dx=" << dx << " dy=" << dy << "\n";
				if (dx == 0 && dy == 0) {
					SetExitCode(2);
					return;
				}
			}
			if (drive_ai_test || drive_test) {
				int frames = 120;
				if (cmd.IsArg("drive-test-frames"))
					frames = max(1, StrInt(cmd.GetArg("drive-test-frames")));
				if (drive_ai_test && cmd.IsArg("drive-ai-frames"))
					frames = max(1, StrInt(cmd.GetArg("drive-ai-frames")));
				if (drive_ai_test && !cmd.IsArg("drive-ai-frames"))
					frames = 900;
				if (!state.HasActiveScene()) {
					Cout() << "DriveTest: missing active scene\n";
					SetExitCode(2);
					return;
				}
				GeomScene& scene = state.GetActiveScene();
				GeomObject* car = FindObjectRecursive(scene, "car_body");
				if (!car) {
					Cout() << "DriveTest: missing car_body\n";
					SetExitCode(2);
					return;
				}
				GeomTransform* tr = car->FindTransform();
				if (!tr) {
					Cout() << "DriveTest: missing car_body transform\n";
					SetExitCode(2);
					return;
				}
				double start_x = tr->position[0];
				double start_z = tr->position[2];
				if (drive_ai_test) {
					vec3 checkpoints[3] = {
						vec3(0.0f, 0.0f, 18.0f),
						vec3(10.0f, 0.0f, 0.0f),
						vec3(0.0f, 0.0f, -18.0f),
					};
					bool visited[3] = {false, false, false};
					const float visit_radius = 8.0f;
					for (int i = 0; i < frames; i++) {
						runtime.input.BeginFrame();
						anim.Update(dt);
						runtime.Update(dt);
						vec3 pos = tr->position;
						for (int c = 0; c < 3; c++) {
							if (visited[c])
								continue;
							vec3 d = pos - checkpoints[c];
							float dist = sqrt(Dot(d, d));
							if (dist < visit_radius)
								visited[c] = true;
						}
					}
					double end_z = tr->position[2];
					Cout() << "DriveAITest: start_z=" << start_z << " end_z=" << end_z << "\n";
					int visited_count = 0;
					for (int c = 0; c < 3; c++)
						if (visited[c])
							visited_count++;
					Cout() << "DriveAITest: visited=" << visited_count << "\n";
					if (visited_count < 2) {
						SetExitCode(2);
						return;
					}
				}
				if (drive_test) {
					runtime.input.SetKey(87, true);
					for (int i = 0; i < frames; i++) {
						runtime.input.BeginFrame();
						anim.Update(dt);
						runtime.Update(dt);
					}
					runtime.input.SetKey(87, false);
					double mid_x = tr->position[0];
					double mid_z = tr->position[2];
					Cout() << "DriveTest: start_z=" << start_z << " mid_z=" << mid_z << "\n";
					if (mid_z <= start_z + 0.5) {
						SetExitCode(2);
						return;
					}
					runtime.input.SetKey(87, true);
					runtime.input.SetKey(65, true);
					for (int i = 0; i < frames; i++) {
						runtime.input.BeginFrame();
						anim.Update(dt);
						runtime.Update(dt);
					}
					runtime.input.SetKey(87, false);
					runtime.input.SetKey(65, false);
					double end_x = tr->position[0];
					double end_z = tr->position[2];
					Cout() << "DriveTest: mid_x=" << mid_x << " end_x=" << end_x
					       << " mid_z=" << mid_z << " end_z=" << end_z << "\n";
					double dx = end_x - mid_x;
					double dz = end_z - mid_z;
					double dist = sqrt(dx * dx + dz * dz);
					if (fabs(dx) < 0.2 || dist < 0.5) {
						SetExitCode(2);
						return;
					}
				}
			}
			SetExitCode(0);
			Image out_img;
			if (!RenderSceneV2Headless(ctx, sz, &stats, nullptr, &debug_dump, dump_first_tri)) {
				Cout() << "RenderStatsV2: failed\n";
				SetExitCode(2);
				return;
			}
			Cout() << "RenderStatsV2: models=" << stats.models
			       << " triangles=" << stats.triangles
			       << " pixels=" << stats.pixels << "\n";
			Cout() << "RenderStatsV2: rendered=" << (stats.rendered ? 1 : 0) << "\n";
			if (!stats.rendered && dump_first_tri && !debug_dump.IsEmpty())
				Cout() << debug_dump;
			if (snapshot) {
				if (RenderSceneV2Headless(ctx, sz, &stats, &out_img, nullptr, false)) {
					PNGEncoder enc;
					String png = enc.SaveString(out_img);
					if (!snapshot_path.IsEmpty())
						SaveFile(snapshot_path, png);
				}
			}
			if (frustum_dump) {
				One<FileOut> out;
				Stream* stream = &Cout();
				if (!frustum_dump_out.IsEmpty()) {
					out.Create(frustum_dump_out);
					if (out->IsOpen())
						stream = out.operator->();
				}
				GeomCamera& program = state.GetProgram();
				Camera cam;
				program.LoadCamera(VIEWMODE_PERSPECTIVE, cam, sz);
				Frustum frustum = cam.GetFrustum();
				DumpFrustum(frustum, program, cam, *stream);
				int total = 0;
				int hits = 0;
				GeomObjectCollection iter(state.GetActiveScene());
				for (GeomObject& go : iter) {
					if (!go.is_visible || !go.IsModel() || !go.mdl)
						continue;
					total++;
					const Model& mdl = *go.mdl;
					AABB aabb = ComputeModelAABB(mdl);
					GeomTransform* tr = go.FindTransform();
					mat4 world_m = Identity<mat4>();
					if (tr)
						world_m = Translate(tr->position) * QuatMat(tr->orientation) * Scale(tr->scale);
					AABB world_aabb = TransformAABB(aabb, world_m);
					bool hit = frustum.Intersects(world_aabb);
					if (hit)
						hits++;
					*stream << "FrustumHit: name=" << go.name
					        << " hit=" << (hit ? 1 : 0)
					        << " center=" << world_aabb.position[0] << "," << world_aabb.position[1] << "," << world_aabb.position[2]
					        << " size=" << world_aabb.size[0] << "," << world_aabb.size[1] << "," << world_aabb.size[2] << "\n";
				}
				*stream << "FrustumHitSummary: " << hits << "/" << total << "\n";
			}
			SetExitCode(stats.rendered ? 0 : 1);
			return;
		}
		bool all_ok = true;
		vec3 target(0, 0, 0);
		if (cmd.IsArg("sweep-target")) {
			vec3 parsed;
			if (ParseVec3Arg(cmd.GetArg("sweep-target"), parsed))
				target = parsed;
		}
		double ang_start = 0.0;
		double ang_end = 2.0 * M_PI;
		if (cmd.IsArg("sweep-angles")) {
			Vector<String> parts = Split(cmd.GetArg("sweep-angles"), ',');
			if (parts.GetCount() == 2) {
				ang_start = DEG2RADf((float)StrDbl(parts[0]));
				ang_end = DEG2RADf((float)StrDbl(parts[1]));
			}
		}
		for (int i = 0; i < sweep_frames; i++) {
			double t = (double)i / (double)sweep_frames;
			double ang = ang_start + (ang_end - ang_start) * t;
			vec3 pos((float)(cos(ang) * sweep_radius), (float)sweep_height, (float)(sin(ang) * sweep_radius));
			GeomCamera& program = state.GetProgram();
			program.position = pos;
			mat4 view = LookAt(program.position, target, vec3(0, 1, 0));
			mat4 world = view.GetInverse();
			program.orientation = MatQuat(world);
			Scene3DRenderStats sweep_stats;
			String sweep_debug;
			Image sweep_img;
			bool ok = RenderSceneV2Headless(ctx, sz, &sweep_stats, snapshot ? &sweep_img : nullptr, &sweep_debug, dump_first_tri);
			Camera cam;
			program.LoadCamera(VIEWMODE_PERSPECTIVE, cam, sz);
			Frustum frustum = cam.GetFrustum();
			if (snapshot && !snapshot_path.IsEmpty() && !sweep_img.IsEmpty()) {
				String base = snapshot_path;
				String out_path = Format("%s_%d.png", base, i);
				PNGEncoder enc;
				SaveFile(out_path, enc.SaveString(sweep_img));
			}
			if (frustum_dump && (frustum_dump_frame < 0 || frustum_dump_frame == i)) {
				One<FileOut> out;
				Stream* stream = &Cout();
				if (!frustum_dump_out.IsEmpty()) {
					out.Create(frustum_dump_out);
					if (out->IsOpen())
						stream = out.operator->();
				}
				DumpFrustum(frustum, program, cam, *stream);
			}
			int total = 0;
			int hits = 0;
			GeomObjectCollection iter(state.GetActiveScene());
			for (GeomObject& go : iter) {
				if (!go.is_visible || !go.IsModel() || !go.mdl)
					continue;
				total++;
				const Model& mdl = *go.mdl;
				AABB aabb = ComputeModelAABB(mdl);
				GeomTransform* tr = go.FindTransform();
				mat4 world_m = Identity<mat4>();
				if (tr)
					world_m = Translate(tr->position) * QuatMat(tr->orientation) * Scale(tr->scale);
				AABB world_aabb = TransformAABB(aabb, world_m);
				if (frustum.Intersects(world_aabb))
					hits++;
			}
			Cout() << "HeadlessSweep: frame=" << i
			       << " rendered=" << (ok && sweep_stats.rendered ? 1 : 0)
			       << " pixels=" << sweep_stats.pixels
			       << " frustum_hits=" << hits
			       << " total=" << total << "\n";
			if (!ok || !sweep_stats.rendered || hits == 0)
				all_ok = false;
		}
		SetExitCode(all_ok ? 0 : 1);
		return;
	}
	RendererKind kind = RENDER_V2_OGL;
	if (cmd.IsArg("renderer")) {
		String name = ToLower(cmd.GetArg("renderer"));
		if (name == "v1")
			kind = RENDER_V1;
		else if (name == "v2")
			kind = RENDER_V2;
		else if (name == "v2_ogl" || name == "ogl" || name == "opengl" || name == "gfx_ogl")
			kind = RENDER_V2_OGL;
		else if (name == "gfx_sw" || name == "gfx")
			kind = RENDER_V2;
	}
	ModelMediaShell app(kind);
	if (cmd.IsArg("no-capture-mouse"))
		app.SetCaptureMouseEnabled(false);
	if (cmd.IsArg("capture-mouse"))
		app.SetCaptureMouseEnabled(true);
	if (cmd.IsArg("relative-mouse"))
		app.SetRelativeMouse(true);
	if (cmd.IsArg("relative-mouse-center")) {
		app.SetRelativeMouse(true);
		app.SetRelativeMouseCenter(true);
	}
	if (!app.LoadExecutionProject(exec_path))
		return;
	app.Run();
}

#else

CONSOLE_APP_MAIN {
	
}

#endif

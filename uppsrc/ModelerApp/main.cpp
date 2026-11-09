#ifdef flagGUI
#include "ModelerApp.h"
#include <Ctrl/Automation/Automation.h>
#include <AI/LogicGui/LogicGui.h>

#ifdef flagMAIN

namespace {

static bool LooksLikeFormula(const String& s) {
	int l = s.Find('(');
	int r = s.ReverseFind(')');
	if (l <= 0 || r <= l)
		return false;
	for (int i = 0; i < l; i++) {
		byte c = (byte)s[i];
		if (!IsAlpha(c) && !IsDigit(c) && c != '_')
			return false;
	}
	return true;
}

static void AddConstraintFormula(const String& raw) {
	String s = TrimBoth(raw);
	if (s.IsEmpty() || s.StartsWith("#"))
		return;
	if (s.StartsWith("-")) {
		int q0 = s.Find('"');
		int q1 = s.ReverseFind('"');
		if (q0 >= 0 && q1 > q0) {
			String quoted = s.Mid(q0 + 1, q1 - q0 - 1);
			if (LooksLikeFormula(quoted))
				Ctrl::constraints.Add(quoted);
			return;
		}
	}
	if (LooksLikeFormula(s))
		Ctrl::constraints.Add(s);
}

static void LoadMaestroConstraints() {
	String root = GetCurrentDirectory();
	while(!root.IsEmpty()) {
		if(DirectoryExists(AppendFileName(root, "docs/maestro")))
			break;
		String next = GetFileFolder(root);
		if(next == root) {
			root.Clear();
			break;
		}
		root = next;
	}
	if(root.IsEmpty())
		return;

	String constr_dir = AppendFileName(root, "docs/maestro/plans/constraints");
	if(!DirectoryExists(constr_dir))
		return;

	FindFile ff(AppendFileName(constr_dir, "*.ugui"));
	while(ff) {
		String path = ff.GetPath();
		String content = LoadFile(path);
		if(!content.IsEmpty()) {
			Vector<String> lines = Split(content, '\n');
			for(const String& l : lines)
				AddConstraintFormula(l);
		}
		ff.Next();
	}
}

static void RunUGuiRibbonChecks(Edit3D& app)
{
	app.Open();
	for(int i = 0; i < 3; i++) {
		Ctrl::ProcessEvents();
		Ctrl::GuiSleep(20);
	}

	Vector<String> tabs;
	tabs.Add("Main tools");
	tabs.Add("Create");
	tabs.Add("Polygon Editing");
	tabs.Add("Scenes");
	tabs.Add("Publish");

	int checks = 0;
	for(const String& tab : tabs) {
		if(app.ribbon.SelectTab(tab)) {
			for(int i = 0; i < 3; i++) {
				Ctrl::ProcessEvents();
				Ctrl::GuiSleep(20);
			}
			Ctrl::CheckConstraints();
			checks++;
		}
	}
	if(checks == 0)
		Ctrl::CheckConstraints();
}

}


	GUI_APP_MAIN {
		using namespace UPP;
		ChGraySkin();
	
	SetCoutLog();
	LinkLogicGui();
	LoadMaestroConstraints();
	
	//DaemonBase::Register<EditClientService>("EditClient");
	
		CommandLineArguments cmd;
		DaemonBase daemon;
		String test_script;
	
	cmd.AddArg('v', "Verbose", false);
	cmd.AddArg('t', "Load test project", false);
	cmd.AddArg('b', "Builtin test index", true, "index");
	cmd.AddArg('s', "Run synthetic sim (visual)", false);
	cmd.AddArg('d', "Dump test .scene3d and exit", false);
	cmd.AddArg('n', "Pointcloud directory", true, "directory");
	cmd.AddArg('r', "Project directory", true, "directory");
	cmd.AddArg('c', "Connect to a server", true, "address");
		cmd.AddArg('p', "Port", true, "integer");
		cmd.AddArg('g', "Debug-mode", false);
		cmd.AddArg('m', "Render math test (headless)", false);
		cmd.AddArg('G', "Dump grid math to file", true, "path");
		cmd.AddArg("test", 0, "Run UI automation script", true, "script.py");
		const Vector<String>& raw_args = CommandLine();
		Vector<String> filtered_args;
		filtered_args.Reserve(raw_args.GetCount());
		bool verbose = false;
		bool verbose_debug = false;
		bool ugui_test = false;
		for (int i = 0; i < raw_args.GetCount(); i++) {
			const String& arg = raw_args[i];
			if (arg == "--ugui-test") {
				ugui_test = true;
				continue;
			}
			if (arg == "--ugui-verbosity" || arg == "-uv") {
				i++;
				continue;
			}
			if (arg.StartsWith("--ugui-verbosity=") || arg.StartsWith("-uv="))
				continue;
			if (arg == "-vd" || arg == "--verbosity-debug") {
				verbose = true;
				verbose_debug = true;
				continue;
			}
		if (arg == "--verbose") {
			verbose = true;
			continue;
		}
		filtered_args.Add(arg);
	}
		if (!cmd.Parse(filtered_args)) {
			cmd.PrintHelp();
			return;
		}
		if (cmd.IsArg('v'))
			verbose = true;
		if (cmd.IsArg("test"))
			test_script = cmd.GetArg("test");
	
	enum {
		EMPTY,
		POINTCLOUD,
		PROJECT0,
		SYNTHETIC,
		REMOTE,
		REMOTE_DEBUG,
	};
	int mode = EMPTY;
	int builtin_index = 0;
	
	String pointcloud_dir;
	String test_scene_path = "share/animation/tests/01.scene3d";
	
	#if 0
	if (cmd.IsArg('c')) {
		daemon.Add("EditClient");
		daemon.Add("EnetClient");
		EnetServiceClient::addr_arg = cmd.GetArg('c');
		if (cmd.IsArg('p')) {
			EnetServiceClient::port_arg = StrInt(cmd.GetArg('p'));
			LOG("Connecting to port " << (int)EnetServiceClient::port_arg);
		}
		EnetServiceClient::SetVerbose(cmd.IsArg('v'));
		if (cmd.IsArg('g'))
			mode = REMOTE_DEBUG;
		else
			mode = REMOTE;
		
		if (!daemon.Init())
			return;
	}
	else
	#endif
	if (cmd.IsArg('n')) {
		pointcloud_dir = cmd.GetArg('n');
		mode = POINTCLOUD;
	}
	else if (cmd.IsArg('t')) {
		mode = PROJECT0;
	}
	else if (cmd.IsArg('s')) {
		mode = SYNTHETIC;
	}
		if (cmd.IsArg('b')) {
			builtin_index = StrInt(cmd.GetArg('b'));
			if (builtin_index < 0)
				builtin_index = 0;
			mode = PROJECT0;
		}
		
		
		if(!ugui_test)
			daemon.RunInThread();
		
		Edit3D app;
		app.verbose = verbose || verbose_debug;
		app.verbose_debug = verbose_debug;
		if (cmd.IsArg('r'))
			app.SetProjectDir(cmd.GetArg('r'));
	if (cmd.IsArg('G'))
		app.conf.dump_grid_path = cmd.GetArg('G');
		if (cmd.IsArg('G')) {
			for (int i = 0; i < 4; i++)
				app.v0.SetRendererVersion(i, 2);
		}
		if (ugui_test) {
			RunUGuiRibbonChecks(app);
			return;
		}

		if (cmd.IsArg('d') && !verbose_debug) {
			app.LoadTestProject(builtin_index);
			RealizeDirectory(GetFileFolder(test_scene_path));
			if (!app.SaveScene3D(test_scene_path, true))
				Cout() << "Failed to write " << test_scene_path << "\n";
			return;
		}
		
		if (!test_script.IsEmpty()) {
			app.Open();
			SetAutomationHooks(nullptr, nullptr);
			
			PyVM vm;
			RegisterAutomationBindings(vm);
			
			try {
				String source = LoadFile(test_script);
				if (source.IsEmpty())
					throw Exc("Could not load script file");
				
				Vector<ProcMsg> errors;
				Tokenizer tokenizer;
				tokenizer.WhenMessage << [&](const ProcMsg& m) {
					if (m.severity == PROCMSG_ERROR)
						errors.Add(m);
				};
				tokenizer.SkipPythonComments(true);
				if (!tokenizer.Process(source, test_script)) {
					Cout() << "Tokenization failed:\n";
					for (const auto& e : errors)
						Cout() << e.line << ":" << e.col << ": " << e.msg << "\n";
					_exit(1);
				}
				tokenizer.CombineTokens();
				
				PyCompiler compiler(tokenizer.GetTokens());
				Vector<PyIR> ir;
				compiler.Compile(ir);
				
				vm.SetIR(ir);
				vm.Run();
			}
			catch (const Exc& e) {
				if (e.Find("EXIT:0") >= 0)
					_exit(0);
				Cout() << "Test Error: " << e << "\n";
				_exit(1);
			}
			_exit(0);
			return;
		}
	if (cmd.IsArg('m')) {
		auto print_vec3 = [&](const String& label, const vec3& v) {
			Cout() << label << ": (" << v[0] << ", " << v[1] << ", " << v[2] << ")\n";
		};
		auto print_mat4 = [&](const String& label, const mat4& m) {
			Cout() << label << ":\n";
			for (int r = 0; r < 4; r++) {
				for (int c = 0; c < 4; c++) {
					Cout() << Format("%9.4f ", (double)m[r][c]);
				}
				Cout() << "\n";
			}
		};
		auto clip_line_ndc = [&](vec2& a, vec2& b) -> bool {
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
		};
		
		Cout() << "Render math test\n";
		Cout() << "SCALAR_FWD_Z=" << SCALAR_FWD_Z << "\n";
		VfsValue cam_node;
		GeomCamera& gc = cam_node.CreateExt<GeomCamera>();
		gc.position = vec3(1, 2, 3);
		gc.orientation = MatQuat(YRotation(0.5) * XRotation(-0.3));
		gc.scale = 1.0f;
		gc.fov = 60.0f;
		Camera cam;
		Size sz(640, 480);
		gc.LoadCamera(VIEWMODE_PERSPECTIVE, cam, sz);
		mat4 world = cam.GetWorldMatrix();
		mat4 proj = cam.GetProjectionMatrix();
		mat4 view = cam.GetViewMatrix();
		print_vec3("cam.pos", gc.position);
		print_vec3("cam.fwd", VectorTransform(VEC_FWD, gc.orientation));
		print_mat4("world", world);
		print_mat4("proj", proj);
		print_mat4("view", view);
		
		Vector<vec3> pts;
		pts.Add(vec3(-1, 0, -1));
		pts.Add(vec3(1, 0, -1));
		pts.Add(vec3(-1, 0, 1));
		pts.Add(vec3(1, 0, 1));
		pts.Add(vec3(0, 0, 5));
		pts.Add(vec3(0, 0, -5));
		for (int i = 0; i < pts.GetCount(); i++) {
			vec3 p = pts[i];
			vec3 cam_p = VecMul(world, p);
			vec3 ndc = VecMul(view, p);
			Cout() << "P" << i << " world ";
			print_vec3("", p);
			Cout() << "  cam  (" << cam_p[0] << ", " << cam_p[1] << ", " << cam_p[2] << ")\n";
			Cout() << "  ndc  (" << ndc[0] << ", " << ndc[1] << ", " << ndc[2] << ")\n";
		}
		vec3 a = vec3(-2, 0, -2);
		vec3 b = vec3(2, 0, 2);
		vec3 a_cam = VecMul(world, a);
		vec3 b_cam = VecMul(world, b);
		Cout() << "Line cam z: a=" << a_cam[2] << " b=" << b_cam[2] << "\n";
		vec3 a_ndc = VecMul(view, a);
		vec3 b_ndc = VecMul(view, b);
		vec2 a2(a_ndc[0], a_ndc[1]);
		vec2 b2(b_ndc[0], b_ndc[1]);
		bool clipped = clip_line_ndc(a2, b2);
		Cout() << "Line clip ndc: " << (clipped ? "true" : "false")
		       << " a=(" << a2[0] << "," << a2[1] << ")"
		       << " b=(" << b2[0] << "," << b2[1] << ")\n";
		
		Cout() << "Grid line samples:\n";
		Vector<float> grid_vals;
		grid_vals.Add(-2.0f);
		grid_vals.Add(0.0f);
		grid_vals.Add(2.0f);
		for (int i = 0; i < grid_vals.GetCount(); i++) {
			float x = grid_vals[i];
			vec3 p0(x, 0, -2);
			vec3 p1(x, 0, 2);
			vec3 p0c = VecMul(world, p0);
			vec3 p1c = VecMul(world, p1);
			vec3 p0n = VecMul(view, p0);
			vec3 p1n = VecMul(view, p1);
			vec2 a2(p0n[0], p0n[1]);
			vec2 b2(p1n[0], p1n[1]);
			bool ok = clip_line_ndc(a2, b2);
			Cout() << "  x=" << x
			       << " camz=(" << p0c[2] << "," << p1c[2] << ")"
			       << " ndc0=(" << p0n[0] << "," << p0n[1] << "," << p0n[2] << ")"
			       << " ndc1=(" << p1n[0] << "," << p1n[1] << "," << p1n[2] << ")"
			       << " clip=" << (ok ? "1" : "0")
			       << " a=(" << a2[0] << "," << a2[1] << ")"
			       << " b=(" << b2[0] << "," << b2[1] << ")\n";
		}
		return;
	}
	
	#if 0
	if (mode == REMOTE_DEBUG)
		app.LoadRemote(daemon.FindServiceT<EditClientService>(), true);
	else if (mode == REMOTE)
		app.LoadRemote(daemon.FindServiceT<EditClientService>(), false);
	else
	#endif
	if (mode == POINTCLOUD)
		app.LoadWmrStereoPointcloud(pointcloud_dir);
	else if (mode == PROJECT0) {
		//if (!app.LoadScene3D(test_scene_path)) {
			app.LoadTestProject(builtin_index);
			RealizeDirectory(GetFileFolder(test_scene_path));
			app.SaveScene3D(test_scene_path, true);
		//}
		app.SetScene3DFormat(true);
		app.ToggleRepeatPlayback();
		app.Play();
		if (builtin_index == 2)
			app.ScheduleBuiltinDump(builtin_index);
	}
	else if (mode == SYNTHETIC) {
		app.LoadEmptyProject();
		app.RunSyntheticSimVisual(true, cmd.IsArg('v'));
	}
	
	app.Run();
	
	daemon.Stop();
	daemon.Deinit();
}

#endif
#endif // flagGUI

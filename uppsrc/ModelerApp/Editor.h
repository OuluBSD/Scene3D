#ifndef _ModelerApp_Editor_h_
#define _ModelerApp_Editor_h_

struct Edit3D;

struct GeomProjectCtrl : Ctrl {
	Edit3D* e;
	
	TreeArrayCtrl tree;
	TreeArrayCtrl props;
	FixedGridCtrl grid;
	TimelineCtrl time;
	EditRendererBase* rends[4] = {0};
	EditRendererV1 rends_v1[4];
	EditRendererV2 rends_v2[4];
	EditRendererV2_Ogl rends_v2_ogl[4];
	int rend_version[4] = {3, 3, 3, 3};
	
	int tree_scenes = -1;
	int tree_col_visible = -1;
	int tree_col_locked = -1;
	int tree_col_read = -1;
	int tree_col_write = -1;
	Index<hash_t> warned_tree_types;
	int props_col_value = -1;
	int props_col_keyframe = -1;
	Vector<hash_t> timeline_row_keys;

	struct TreeNodeRef {
		enum Kind {
			K_VFS,
			K_PROGRAM,
			K_FOCUS,
		};
		Kind kind = K_VFS;
		VfsValue* vfs = 0;
	};

	Vector<TreeNodeRef> tree_nodes;
	bool use_ecs_tree = true;
	VfsValue ecs_root_val;
	VectorMap<const VfsValue*, VfsValue*> ecs_to_geom;
	VectorMap<const VfsValue*, VfsValue*> geom_to_ecs;
	bool program_read = true;
	bool program_write = false;
	bool focus_read = true;
	bool focus_write = false;
	int focus_tree_id = -1;
	GeomObject* selected_obj = 0;
	TreeNodeRef* selected_ref = 0;
	GeomPointcloudDataset* selected_dataset = 0;
	bool props_refreshing = false;
	int last_props_frame = -1;
	int last_props_scene_frame = -1;
	struct TimelineRowInfo {
		enum Kind {
			R_SCENE,
			R_OBJECT,
			R_TRANSFORM,
			R_POSITION,
			R_ORIENTATION,
			R_MESH,
			R_2D,
		};
		Kind kind = R_OBJECT;
		hash_t object_key = 0;
		int indent = 0;
		bool has_children = false;
		bool expanded = true;
		bool active = false;
		bool muted = false;
		bool solo = false;
		Color tag_color = Null;
	};
	Vector<TimelineRowInfo> timeline_rows;
	Index<hash_t> timeline_expanded;
	VectorMap<hash_t, bool> timeline_muted;
	VectorMap<hash_t, bool> timeline_solo;
	VectorMap<hash_t, Color> timeline_row_color;
	bool timeline_has_solo = false;
	int timeline_menu_row = -1;
	struct TimelineClipboard {
		struct Item {
			TimelineRowInfo::Kind kind = TimelineRowInfo::R_OBJECT;
			hash_t object_key = 0;
			int frame_offset = 0;
			GeomKeypoint keypoint;
			GeomMeshKeyframe mesh_kf;
			Geom2DKeyframe a2d_kf;
		};
		Array<Item> items;
		int base_frame = 0;
		bool has_range = false;
		void Clear() { items.Clear(); base_frame = 0; has_range = false; }
	};
	TimelineClipboard timeline_clipboard;

	struct PropRef {
		enum Kind {
			P_ROOT,
			P_TRANSFORM,
			P_POSITION,
			P_ORIENTATION,
			P_EFFECTS,
			P_EFFECT_POSITION,
			P_EFFECT_ORIENTATION,
			P_MATERIALS,
			P_MATERIAL,
			P_MAT_BASE_COLOR,
			P_MAT_BASE_ALPHA,
			P_MAT_METALLIC,
			P_MAT_ROUGHNESS,
			P_MAT_EMISSIVE,
			P_MAT_NORMAL_SCALE,
			P_MAT_OCCLUSION,
			P_MAT_STROKE_CAP,
			P_MAT_STROKE_JOIN,
			P_SELECTION,
			P_SELECTION_CENTER,
			P_SELECTION_OFFSET,
			P_MESH_MATERIAL,
			P_COMPONENTS,
			P_SCRIPT,
			P_SCRIPT_FILE,
			P_SCRIPT_ENABLED,
			P_SCRIPT_RUN_ON_LOAD,
			P_SCRIPT_RUN_EACH_FRAME,
			P_SCRIPT_EDIT,
			P_SCRIPT_RUN,
			P_POINTCLOUD,
			P_DATASET,
			P_SCENE_TIMELINE,
			P_SCENE_TIMELINE_LENGTH,
			P_SCENE_TIMELINE_POSITION,
			P_SCENE_TIMELINE_PLAY,
			P_SCENE_TIMELINE_REPEAT,
			P_SCENE_TIMELINE_SPEED,
		};
		Kind kind = P_ROOT;
		GeomScript* script = 0;
		int material_id = -1;
		int mesh_index = -1;
	};

	Vector<PropRef> props_nodes;
	Vector<One<Ctrl>> props_ctrls;
	struct PropsCursorState : Moveable<PropsCursorState> {
		struct PropPathToken : Moveable<PropPathToken> {
			int kind = -1;
			int material_id = -1;
			int mesh_index = -1;
			String label;
			void Serialize(Stream& s) { s % kind % material_id % mesh_index % label; }
		};
		Vector<PropPathToken> tokens;
		Vector<int> index_path;
		int line = -1;
		int scroll = 0;
		void Serialize(Stream& s) { s % tokens % index_path % line % scroll; }
	};
	Ctrl* props_transform_pos_ctrl = 0;
	Ctrl* props_transform_ori_ctrl = 0;
	Ctrl* props_selection_center_ctrl = 0;
	Ctrl* props_selection_offset_ctrl = 0;
	int props_transform_pos_id = -1;
	int props_transform_ori_id = -1;
	int props_selection_center_id = -1;
	int props_selection_offset_id = -1;
	VectorMap<String, PropsCursorState> props_cursor_by_tree;
	String current_tree_path;
	Vector<String> tree_open_paths;
	
	
	typedef GeomProjectCtrl CLASSNAME;
	GeomProjectCtrl(Edit3D* e);
	void Update(double dt);
	void Data();
	void TimelineData();
	void TimelineRowMenu(Bar& bar, int row);
	void TimelineRowSelect(int row);
	void TimelineRowToggle(int row);
	void TimelineToggleKeyframe(int row, int frame);
	void TimelineRemoveKeyframe(int row, int frame);
	void TimelineMoveKeyframe(int row, int from_frame, int to_frame);
	void TimelineToggleAutoKey();
	void TimelineCopySelection();
	void TimelinePasteSelection(int frame);
	void TimelineToggleMuteRow(int row);
	void TimelineToggleSoloRow(int row);
	void TimelineSetRowColor(int row, Color c);
	void TimelineRowColorMenu(Bar& bar);
	void TreeSelect();
	void TreeMenu(Bar& bar);
	void PropsMenu(Bar& bar);
	void UpdateTreeFocus(int new_id);
	void PropsData();
	void SyncPropsValues();
	void PropsApply();
	TreeNodeRef* GetNodeRef(const Value& v);
	VfsValue* ResolveGeomValue(const Value& v);
	void BuildEcsMirror();
	void TreeValueEcs(int id, VfsValue& node);
	GeomObject* GetNodeObject(const Value& v);
	GeomPointcloudDataset* GetNodeDataset(const Value& v);
	String GetTreePathForValue(const Value& v, int id) const;
	String GetTreePathFromId(int id) const;
	String GetPropsPathForId(int id) const;
	int FindPropsIdByPath(const String& path, bool open);
	Vector<int> GetPropsIndexPathForId(int id) const;
	int FindPropsIdByIndexPath(const Vector<int>& path, bool open);
	Vector<PropsCursorState::PropPathToken> GetPropsTokensForId(int id) const;
	int FindPropsIdByTokens(const Vector<PropsCursorState::PropPathToken>& tokens, bool open);
	void StorePropsCursor(const String& tree_path);
	void RestorePropsCursor(const String& tree_path);
	void StoreTreeOpenState();
	void RestoreTreeOpenState();
	void OnCursor(int kp_i);
	void TreeValue(int id, VfsValue& node);
	void RefreshRenderer(int i);
	void RefreshAll();
	void BuildViewMenu(Bar& bar, int i);
	void SetRendererVersion(int i, int version);
	void RebuildGrid();
	
};

struct FilePoolCtrl : ParentCtrl {
	Edit3D* owner = 0;
	ArrayCtrl files;

	typedef FilePoolCtrl CLASSNAME;
	FilePoolCtrl(Edit3D* e);
	void Data();
};

struct AssetBrowserCtrl : ParentCtrl {
	Edit3D* owner = 0;
	Splitter split;
	TreeCtrl tree;
	ArrayCtrl files;
	String root_dir;
	String current_dir;
	Vector<String> recent_assets;

	typedef AssetBrowserCtrl CLASSNAME;
	AssetBrowserCtrl(Edit3D* e);
	void SetRoot(const String& dir);
	void SetRecent(const Vector<String>& list);
	void Data();
	void BuildTree();
	void UpdateFiles();
	void OnTreeCursor();
	void OnFileDouble();
	void StartDrag();
	String GetCursorPath() const;
	Image MakePreview(const String& path, int size) const;
};

struct ScriptEditorCtrl : ParentCtrl {
	Edit3D* owner = 0;
	CodeEditor editor;
	ToolBar tool;
	ArrayCtrl errors;
	Splitter split;
	String path;
	bool dirty = false;
	GeomScript* script = 0;
	VectorMap<String, Vector<int>> breakpoints;
	
	typedef ScriptEditorCtrl CLASSNAME;
	ScriptEditorCtrl(Edit3D* e);
	void OpenFile(const String& p);
	void OpenScript(GeomScript& s);
	void Save();
	void SaveAs(const String& p);
	void OnChange();
	void RunFile();
	void RunSelection();
	void StopScript();
	void ClearErrors();
	void AddError(int line, const String& msg);
	void ApplyBreakpoints();
	void ToggleBreakpoint(int line);
};

struct TextureEditCtrl : Ctrl {
	ImageBuffer img;
	Image cached;
	bool has_img = false;
	bool painting = false;
	int brush = 12;
	Color color = Color(200, 60, 60);
	Rect img_rect;

	Event<> WhenPaint;

	typedef TextureEditCtrl CLASSNAME;
	TextureEditCtrl();
	void SetImage(const Image& image);
	void Clear();
	Image GetImage() const;
	void Paint(Draw& d) override;
	void LeftDown(Point p, dword keyflags) override;
	void LeftDrag(Point p, dword keyflags) override;
	void LeftUp(Point p, dword keyflags) override;
	void MouseWheel(Point p, int zdelta, dword keyflags) override;

private:
	void PaintAt(Point p);
	void UpdateCached();
};

#if defined(flagPOSIX) && defined(flagSOFTHMD)
struct HmdCapture {
	HMD::System sys;
	One<StereoSource> source;
	HMD::SoftHmdFusion fusion;
	Image bright;
	Image dark;
	int64 bright_serial = -1;
	int64 dark_serial = -1;
	bool running = false;
	bool recording = false;

	typedef HmdCapture CLASSNAME;
	bool Start();
	void Stop();
	void ResetTracking();
	void Poll();
	bool IsRunning() const { return running; }
	bool IsRecording() const { return recording; }
	const Octree* GetPointcloud(bool bright) const;
};
#else
struct HmdCapture {
	bool recording = false;
	bool Start() { return false; }
	void Stop() {}
	void ResetTracking() {}
	void Poll() {}
	bool IsRunning() const { return false; }
	bool IsRecording() const { return false; }
	const Octree* GetPointcloud(bool bright) const { return nullptr; }
};
#endif

struct ToolPanel : Ctrl {
	Edit3D* owner = nullptr;
	Option view;
	Option obj_select;
	Option obj_move;
	Option obj_rotate;
	Option mesh_select;
	Option mesh_vertex;
	Option mesh_edge;
	Option mesh_face;
	Option draw_point;
	Option draw_line;
	Option draw_face;
	Button extrude;
	Button inset;
	Button spin;
	Button screw;
	DropList cam_view;
	Option cam_focus;
	Option cam_program;
	Option cam_selected;
	Vector<Ctrl*> layout_items;

	void Init(Edit3D* e);
	void Sync();
	virtual void Layout() override;
};

struct Edit3D : DockWindow {
	typedef enum {
		VIEW_NONE,
		VIEW_GEOMPROJECT,
		VIEW_VIDEOIMPORT,
	} ViewType;
	
	ViewType view = VIEW_NONE;
	
	GeomProjectCtrl v0;
	VideoImportCtrl v1;
	FilePoolCtrl file_pool;
	AssetBrowserCtrl asset_browser;
	TextureEditCtrl texture_edit;
	//RemoteDebugCtrl v_rdbg;
	ToolPanel tool_panel;
	ModelerAppRibbon ribbon;
	ToolBar tool;
	int ribbon_display_mode = RibbonBar::RIBBON_ALWAYS;
	int ribbon_qat_pos = RibbonBar::QAT_TOP;
	DockableCtrl* dock_tree = 0;
	DockableCtrl* dock_props = 0;
	DockableCtrl* dock_time = 0;
	DockableCtrl* dock_video = 0;
	DockableCtrl* dock_pool = 0;
	DockableCtrl* dock_assets = 0;
	DockableCtrl* dock_texture = 0;
	DockableCtrl* dock_script = 0;
	DockableCtrl* dock_tools = 0;
	
	struct ExecutionWindow : TopWindow {
		typedef ExecutionWindow CLASSNAME;
		Edit3D* owner = nullptr;
		EditRendererV2_Ogl renderer;
		Scene3DRenderConfig conf;
		Scene3DRenderContext ctx;
		TimeCallback tc;
		
		ExecutionWindow();
		void Init(Edit3D* o);
		void RefreshFrame();
		void CloseWindow();
	};
	
	Scene3DRenderConfig conf;
	Scene3DRenderContext render_ctx;
	Vector<EditRendererBase*> external_renderers;
	One<ExecutionWindow> exec_win;
	
	
	VfsValue prj_val;
	GeomProject* prj = 0;
	VfsValue state_val;
	GeomWorldState* state = 0;
	VfsValue anim_val;
	GeomAnim* anim = 0;
	//GeomVideo video;
	GeomStagedVideo video;
	TimeCallback tc;
	TimeCallback debug_tc;
	TimeStop ts;
	HmdCapture hmd;
	GeomObject* texture_obj = 0;
	SyntheticPointcloudConfig sim_cfg;
	SyntheticPointcloudState sim_state;
	PointcloudObservation sim_obs;
	Vector<ControllerObservation> sim_ctrl_obs;
	PointcloudPose sim_fake_hmd_pose;
	PointcloudPose sim_localized_pose;
	bool sim_has_state = false;
	bool sim_has_obs = false;
	bool sim_has_ctrl_obs = false;
	bool sim_observation_effect_visible = true;
	bool sim_observation_effect_locked = false;
	GeomObject* sim_pointcloud_obj = 0;
	GeomObject* sim_observation_obj = 0;
	GeomObject* sim_controller_obj[2] = {0, 0};
	GeomObject* sim_controller_model_obj[2] = {0, 0};
	GeomObject* sim_hmd_pointcloud_obj = 0;
	One<Octree> sim_obs_octree;
	String scene3d_path;
	String scene3d_created;
	String scene3d_modified;
	String scene3d_data_dir;
	Array<Scene3DExternalFile> scene3d_external_files;
	Array<Scene3DMetaEntry> scene3d_meta;
	bool scene3d_use_json = true;
	Vector<String> recent_assets;
	struct UndoEntry {
		String json;
		String note;
	};
	Array<UndoEntry> undo_stack;
	Array<UndoEntry> redo_stack;
	int undo_limit = 50;
	bool undo_guard = false;
	bool repeat_playback = false;
	bool record_pointcloud = false;
	GeomObject* hmd_pointcloud = 0;
	String project_dir;
	bool verbose = false;
	bool verbose_debug = false;
	TimeStop script_timer;
	VfsValue* selected_bone = 0;

	enum EditTool {
		TOOL_VIEW,
		TOOL_OBJ_SELECT,
		TOOL_OBJ_MOVE,
		TOOL_OBJ_ROTATE,
		TOOL_MESH_SELECT,
		TOOL_POINT,
		TOOL_LINE,
		TOOL_FACE,
		TOOL_ERASE,
		TOOL_JOIN,
		TOOL_SPLIT,
		TOOL_2D_SELECT,
		TOOL_2D_LINE,
		TOOL_2D_RECT,
		TOOL_2D_CIRCLE,
		TOOL_2D_POLY,
		TOOL_2D_ERASE,
	};
	enum EditPlaneMode {
		PLANE_VIEW,
		PLANE_XY,
		PLANE_XZ,
		PLANE_YZ,
		PLANE_LOCAL,
	};
	EditTool edit_tool = TOOL_OBJ_SELECT;
	EditPlaneMode edit_plane = PLANE_VIEW;
	int edit_line_start = -1;
	int edit_join_start = -1;
	Vector<int> edit_face_points;
	double edit_pick_radius_px = 10.0;
	double edit_line_pick_radius_px = 8.0;
	bool edit_snap_enable = false;
	double edit_snap_step = 0.1;
	bool edit_snap_local = true;
	bool transform_snap_enable = false;
	double transform_snap_step = 0.1;
	bool transform_use_local = true;
	bool transform_angle_snap = false;
	double transform_angle_step = M_PIf / 12.0;
	double transform_nudge_small = 0.01;
	double transform_nudge_large = 0.1;
	double transform_rot_small = M_PIf / 180.0;
	double transform_rot_large = M_PIf / 12.0;
	bool sculpt_mode = false;
	double sculpt_radius = 0.5;
	double sculpt_strength = 0.2;
	bool sculpt_add = true;
	bool weight_paint_mode = false;
	double weight_radius = 0.5;
	double weight_strength = 0.2;
	bool weight_add = true;
	bool auto_key = false;
	bool auto_key_position = true;
	bool auto_key_orientation = true;
	bool auto_key_mesh = false;
	bool auto_key_2d = false;
	bool show_hud = true;
	bool show_hud_help = false;
	bool show_hud_status = true;
	bool show_hud_debug = false;
	enum MeshSelectMode {
		MESHSEL_VERTEX,
		MESHSEL_EDGE,
		MESHSEL_FACE
	};
	MeshSelectMode mesh_select_mode = MESHSEL_VERTEX;
	Vector<int> mesh_sel_points;
	Vector<int> mesh_sel_lines;
	Vector<int> mesh_sel_faces;
	Vector<int> select_2d_shapes;
	int select_2d_primary = -1;
	int select_2d_hover = -1;
	vec3 mesh_sel_offset = vec3(0);
	vec2 sel2d_offset = vec2(0);
	int active_view = 3;
	bool selection_dragging = false;
	vec3 selection_drag_start_world = vec3(0);
	vec3 selection_drag_applied_local = vec3(0);
	vec3 selection_drag_plane_normal = vec3(0, 0, 1);
	int selection_drag_view = -1;
	int selection_drag_mode = 0;
	vec3 selection_drag_start_pos = vec3(0);
	quat selection_drag_start_ori = Identity<quat>();
	Point selection_drag_start_mouse = Point(0, 0);
	enum TimelineScopeKind {
		TS_SCENE,
		TS_OBJECT,
		TS_COMPONENT
	};
	enum TimelineComponent {
		TC_NONE,
		TC_TRANSFORM,
		TC_MESH,
		TC_2D
	};
	enum TimelineTransformField {
		TT_NONE,
		TT_POSITION,
		TT_ORIENTATION
	};
	TimelineScopeKind timeline_scope = TS_SCENE;
	TimelineComponent timeline_component = TC_NONE;
	TimelineTransformField timeline_transform_field = TT_NONE;
	hash_t timeline_object_key = 0;
	bool draw2d_active = false;
	vec2 draw2d_start = vec2(0);
	vec2 draw2d_last = vec2(0);
	Vector<vec2> draw2d_poly;
	int draw2d_view = -1;
	GeomObject* draw2d_obj = 0;
	
	struct ScriptInstance {
		GeomScript* script = 0;
		VfsValue* owner = 0;
		String abs_path;
		Time file_time;
		PyVM vm;
		bool loaded = false;
		bool has_load = false;
		bool has_start = false;
		bool has_frame = false;
		Vector<PyIR> main_ir;
		Vector<PyIR> load_ir;
		Vector<PyIR> start_ir;
		Vector<PyIR> frame_ir;
	};
	Array<ScriptInstance> script_instances;
	ScriptEditorCtrl script_editor;
	
	struct ScriptEventHandler : Moveable<ScriptEventHandler> {
		String event;
		PyValue func;
		PyVM* vm = nullptr;
		VfsValue* node = nullptr;
	};
	Vector<ScriptEventHandler> script_event_handlers;
	
	void CreateDefaultInit();
	void CreateDefaultPostInit();
	void LoadTestCirclingCube();
	void LoadTestOctree();
	void LoadTestTimelineSphere();
	void LoadTestHmdPointcloud();
	void EnsureHmdSceneObjects();
	void EnsureSimSceneObjects();
	void TogglePointcloudRecording();
	void StartPointcloudRecording();
	void StopPointcloudRecording();
	void UpdateHmdCameraPose();
	void RunSyntheticPointcloudSimDialog();
	String RunLocalizationLog(bool show_dialog);
	String RunControllerLocalizationLog(bool show_dialog);
	void RefreshSimObservation();
	void DebugGeneratePointcloud();
	void DebugSimulateObservation();
	void DebugRunLocalization();
	void DebugRunControllerLocalization();
	void DebugSimulateControllerObservations();
	void DebugClearSynthetic();
	void GenerateSyntheticPointcloudFor(GeomObject& obj);
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
	void RegisterScriptVM(PyVM& vm);
	void SetEditTool(EditTool tool);
	void CreateEditableMeshObject();
	void Create2DLayerObject();
	GeomObject* GetFocusedMeshObject();
	GeomObject* GetFocused2DObject();
	void Clear2DSelection();
	void Select2DShape(int idx, bool add, bool toggle);
	void Toggle2DShape(int idx);
	int Pick2DShape(const Geom2DLayer& layer, const vec2& local, double radius) const;
	Rectf Get2DShapeBounds(const Geom2DShape& shape) const;
	void Translate2DShape(Geom2DShape& shape, const vec2& delta);
	void Align2DSelection(int mode);
	void Distribute2DSelection(bool horizontal);
	void Union2DSelection();
	void Intersect2DSelection();
	void Subtract2DSelection();
	bool GetMeshSelectionCenter(vec3& out);
	bool Get2DSelectionCenter(vec3& out);
	bool GetSelectionCenterWorld(vec3& out, GeomObject*& obj, bool& is2d);
	void ApplyMeshSelectionDelta(const vec3& delta);
	void ApplyMeshSelectionRotate(float angle_rad, const vec3& axis_local);
	void ApplyMeshSelectionScale(float factor);
	void Apply2DSelectionDelta(const vec2& delta);
	void ClearMeshSelection();
	void ToggleMeshPoint(int idx);
	void ToggleMeshLine(int idx);
	void ToggleMeshFace(int idx);
	void SelectMeshLoop();
	void SelectMeshRing();
	void ExpandMeshSelection();
	void ContractMeshSelection();
	void ExtrudeMeshSelection(double amount);
	void InsetMeshSelection(double amount);
	void SpinMeshSelection();
	void ScrewMeshSelection();
	void AddMeshKeyframeAtCursor();
	void ClearMeshKeyframes();
	void Add2DKeyframeAtCursor();
	void Clear2DKeyframes();
	void AutoKeyMeshEdit(GeomObject* obj);
	void AutoKey2DEdit(GeomObject* obj);
	bool ApplyTransformDelta(GeomObject* obj, const vec3& delta, bool local_axes);
	bool ApplyTransformRotation(GeomObject* obj, int axis, double angle_rad, bool local_axes);
	void MaybeAutoKeyTransform(GeomObject* obj, bool pos, bool ori);
	void PushUndo(const String& note);
	void Undo();
	void Redo();
	String SerializeScene3DState() const;
	bool RestoreScene3DState(const String& json);
	bool ScreenToPlaneWorldPoint(int view_i, const Point& p, const vec3& origin, const vec3& normal, vec3& out) const;
	bool ScreenToRay(int view_i, const Point& p, vec3& out_origin, vec3& out_dir) const;
	void CreateSkeletonForSelected();
	void AddBoneToSelectedSkeleton();
	void RemoveSelectedBone();
	void SetWeightPaintMode(bool enable);
	int PickNearestPoint(const GeomEditableMesh& mesh, int view_i, const Point& p, double radius_px) const;
	int PickNearestLine(const GeomEditableMesh& mesh, int view_i, const Point& p, double radius_px) const;
	int PickNearestFace(const GeomEditableMesh& mesh, int view_i, const Point& p) const;
	void RemoveEditablePoint(GeomEditableMesh& mesh, int idx);
	bool HasLine(const GeomEditableMesh& mesh, int a, int b) const;
	void OpenScriptEditor(GeomScript& script);
	void RunScriptOnce(GeomScript& script);
	void OpenExecutionWindow();
	bool ExportExecutionProject(bool full_assets);
	void RegisterExternalRenderer(EditRendererBase* renderer);
	void UnregisterExternalRenderer(EditRendererBase* renderer);
	GeomScript& AddScriptComponent(GeomObject& obj);
	GeomScript& AddScriptComponent(GeomDirectory& dir);
	GeomScript& AddScriptComponent(GeomScene& scene);
	void GetScriptsFromNode(VfsValue& node, Vector<GeomScript*>& out);
	String EnsureScriptFile(GeomScript& script, String base_name);
	String GetScriptAbsPath(const String& rel) const;
	void SetProjectDir(String dir);
	const String& GetProjectDir() const { return project_dir; }
	void SyncPointcloudDatasetsExternalFiles();
	String GetAssetRootDir() const;
	void AddRecentAsset(const String& rel);
	void UpdateAssetBrowser();
	void AddAssetFromPath(const String& path);
	void SyncAssetExternalFiles();
	void UpdateTextureEditor(GeomObject* obj);
	void SaveTextureEdit(GeomObject& obj, GeomTextureEdit& tex, ImageBuffer& ib);
	String EnsureTexturePath(GeomObject& obj, GeomTextureEdit& tex);
	void SyncTextureExternalFiles();
	void UpdateHud();
	void ApplyToolPanelCameraSource(CameraSource src);
	void ApplyRendererCameraSource(int view_i, CameraSource src, hash_t object_key = 0);
	void RequestExecutionExit();
	void UpdateRibbonContext();
	void FocusSelectedNode();
	void SyncRibbonLightmapControls();
	void StoreRibbonLightmapSettings(bool bake);
	bool BakeSceneLightmaps(String& summary);
	void CreatePrimitiveCube(double size);
	void CreatePrimitiveSphere(double radius, int slices, int stacks);
	void CreatePrimitiveCylinder(double radius, double length, int slices);
	void CreatePrimitiveCone(double radius, double length, int slices);
	GeomObject* CreatePrimitivePlane(int tiles, double width, double length, int repeat);
	
public:
	typedef Edit3D CLASSNAME;
	Edit3D();

	bool Key(dword key, int count) override;
	void DragAndDrop(Point p, PasteClip& d) override;
	void SetView(ViewType view);
	virtual void DockInit() override;
	void Update();
	void Data();
	void Exit();
	void RefreshData();
	void Stop();
	void Pause();
	void Play();
	void RefrehRenderers();
	void RefrehToolbar();
	void OnSceneEnd();
	bool HandleRibbonAction(const String& id);
	void ScheduleBuiltinDump(int builtin_index, int delay_ms = 2000);
	void DumpBuiltinState(int builtin_index);
	void OnDebugMetadata();
	void Toolbar(Bar& bar);
	void UpdateWindowTitle();
	void SetScene3DFormat(bool use_json);
	void ToggleRepeatPlayback();
	void Serialize(Stream& s) override;
	void ResetLayout();
	void ResetPropsCursor();
	void OpenAllDocksForTest();
	void OpenScene3D();
	void OpenTextureEditor();
	void OpenFilePool();
	void OpenAssetBrowser();
	void SaveScene3DInteractive();
	void SaveScene3DAs();
	void SaveScene3DAsJson();
	void SaveScene3DAsBinary();
	bool SaveScene3DWithDialog(bool use_json);
	bool LoadScene3DWithDialog();
	bool IsScene3DBinaryPath(const String& path) const;
	bool IsScene3DJsonPath(const String& path) const;
	String EnsureScene3DExtension(const String& path, bool use_json) const;
	
	void LoadEmptyProject();
	void LoadTestProject(int test_i);
	void LoadWmrStereoPointcloud(String directory);
	void RunSyntheticSimVisual(bool log_stdout, bool verbose);
	bool LoadScene3D(const String& path);
	bool SaveScene3D(const String& path, bool use_json, bool pretty = true);
	
	GeomScene& GetActiveScene();

	ExecInputState input_state;
	bool exec_exit_requested = false;
	
};


#endif

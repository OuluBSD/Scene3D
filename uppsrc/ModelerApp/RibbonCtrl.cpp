#include "ModelerApp.h"

NAMESPACE_UPP

namespace {

class LightmapRowCtrl : public ParentCtrl {
public:
	struct Item : Moveable<Item> {
		int label_idx = -1;
		int ctrl_idx = -1;
	};

	Array<Label> labels;
	Array<Ctrl> ctrls;
	Vector<Item> items;
	int gap = DPI(8);

	void SetGap(int g)
	{
		gap = g;
		RefreshParentLayout();
	}

	template <class T>
	T& AddCtrl(const String& label)
	{
		Item& it = items.Add();
		if (!label.IsEmpty()) {
			Label& l = labels.Add();
			l.SetText(label);
			Add(l);
			it.label_idx = labels.GetCount() - 1;
		}
		T& c = static_cast<T&>(ctrls.Create<T>());
		Add(c);
		it.ctrl_idx = ctrls.GetCount() - 1;
		return c;
	}

	Size GetMinSize() const override
	{
		int cx = 0;
		int cy = 0;
		for (int i = 0; i < items.GetCount(); i++) {
			const Item& it = items[i];
			int w = 0;
			int h = 0;
			if (it.label_idx >= 0) {
				Size ls = labels[it.label_idx].GetMinSize();
				w += ls.cx;
				h = max(h, ls.cy);
			}
			if (it.ctrl_idx >= 0) {
				Size cs = ctrls[it.ctrl_idx].GetMinSize();
				if (it.label_idx >= 0)
					w += gap;
				w += cs.cx;
				h = max(h, cs.cy);
			}
			cx += w;
			if (i + 1 < items.GetCount())
				cx += gap;
			cy = max(cy, h);
		}
		return Size(cx, cy);
	}

	void Layout() override
	{
		int x = 0;
		int h = GetSize().cy;
		for (int i = 0; i < items.GetCount(); i++) {
			const Item& it = items[i];
			if (it.label_idx >= 0) {
				Size ls = labels[it.label_idx].GetMinSize();
				labels[it.label_idx].SetRect(x, 0, ls.cx, h);
				x += ls.cx + gap;
			}
			if (it.ctrl_idx >= 0) {
				Size cs = ctrls[it.ctrl_idx].GetMinSize();
				ctrls[it.ctrl_idx].SetRect(x, 0, cs.cx, h);
				x += cs.cx;
			}
			x += gap;
		}
	}
};

class FormPanel : public ParentCtrl {
public:
	struct Row : Moveable<Row> {
		int label_idx = -1;
		int ctrl_idx = -1;
		int height = 0;
	};

	Array<Label> labels;
	Array<Ctrl> ctrls;
	Vector<Row> rows;
	int row_cy = 22;
	int gap = 4;
	int label_cx = 120;

	void SetMetrics(int _row_cy, int _gap, int _label_cx)
	{
		row_cy = _row_cy;
		gap = _gap;
		label_cx = _label_cx;
		RefreshParentLayout();
	}

	void AddLabelOnly(const String& text)
	{
		Row& r = rows.Add();
		Label& l = labels.Add();
		l.SetText(text);
		Add(l);
		r.label_idx = labels.GetCount() - 1;
		r.ctrl_idx = -1;
		r.height = row_cy;
	}

	template <class T>
	T& AddRowCtrl(const String& label)
	{
		Row& r = rows.Add();
		if (!label.IsEmpty()) {
			Label& l = labels.Add();
			l.SetText(label);
			Add(l);
			r.label_idx = labels.GetCount() - 1;
		}
		T& c = static_cast<T&>(ctrls.Create<T>());
		Add(c);
		r.ctrl_idx = ctrls.GetCount() - 1;
		r.height = row_cy;
		return c;
	}

	Size GetMinSize() const override
	{
		int cy = 0;
		int max_cx = 0;
		for (const Row& r : rows) {
			cy += (r.height ? r.height : row_cy);
			if (r.ctrl_idx >= 0 && r.ctrl_idx < ctrls.GetCount()) {
				max_cx = max(max_cx, ctrls[r.ctrl_idx].GetMinSize().cx);
			}
		}
		int cx = label_cx + gap + max_cx;
		return Size(cx, cy);
	}

	void Layout() override
	{
		int y = 0;
		int cx = GetSize().cx;
		int value_cx = max(0, cx - label_cx - gap);
		for (const Row& r : rows) {
			int h = r.height ? r.height : row_cy;
			if (r.label_idx >= 0 && r.label_idx < labels.GetCount()) {
				labels[r.label_idx].SetRect(0, y, label_cx, h);
			}
			if (r.ctrl_idx >= 0 && r.ctrl_idx < ctrls.GetCount()) {
				ctrls[r.ctrl_idx].SetRect(label_cx + gap, y, value_cx, h);
			}
			y += h;
		}
	}
};

static Image GetRibbonIconFallback(const String& key)
{
	if(key.Find("new") >= 0) return CtrlImg::new_doc();
	if(key.Find("open") >= 0) return CtrlImg::open();
	if(key.Find("save") >= 0) return CtrlImg::save();
	if(key.Find("undo") >= 0) return CtrlImg::undo();
	if(key.Find("redo") >= 0) return CtrlImg::redo();
	if(key.Find("play") >= 0 || key.Find("publish") >= 0) return CtrlImg::File();
	if(key.Find("pause") >= 0 || key.Find("stop") >= 0) return CtrlImg::File();
	if(key.Find("plus") >= 0 || key.Find("add") >= 0) return CtrlImg::plus();
	if(key.Find("minus") >= 0 || key.Find("delete") >= 0) return CtrlImg::minus();
	if(key.Find("gear") >= 0 || key.Find("settings") >= 0) return CtrlImg::File();
	return CtrlImg::File();
}

static Image GetRibbonIcon(const String& id, const String& sem)
{
	String key = sem;
	if (key.IsEmpty())
		key = id;
	static const char* kKnown[] = {
		"generic_new_file",
		"generic_open_file",
		"generic_save_file",
		"generic_undo",
		"generic_redo",
		"mouse_cursor",
		"four_arrows_cross",
		"clockwise_arc_arrow",
		"3d_scale_arrows",
		"frame_plus_3d_perspective_block",
		"frame_plus_top_projection_flat_wide_block",
		"frame_plus_front_square_projection",
		"frame_plus_left_tall_narrow_projection",
		"gray_cube_with_black_wireframe",
		"gray_rough_sphere_with_black_wireframe",
		"gray_rough_cylinder_with_black_wireframe",
		"gray_rough_cone_with_black_wireframe",
		"gray_square_plane_in_perspective_with_black_wireframe",
		"classic_film_camera_icon_gray_black",
		"side_profile_terrain_curve_with_filled_ground_polygon",
		"coarse_3d_corridor_shape",
		"3d_cube_like_import_symbol",
		"3d_cube_like_import_symbol_animated",
		"yellow_glowing_lightbulb",
		"sun_icon_with_arrow_left_down",
		"green_2d_fir_tree",
		"blue_plane_in_perspective",
		"gray_background_with_light_B",
		"narrower_taller_variant_of_billboard_icon",
		"small_gray_particle_with_yellow_arrows_up_left_and_up_right",
		"inside_cube_view_blue_sky_walls_gray_ground",
		"speaker_cone",
		"gray_background_with_dark_2D_text",
		"2d_overlay_symbol_with_circular_background",
		"two_points_connected_by_line",
		"path_like_symbol_with_additional_points",
		"triangle_over_axis_background_with_mouse_cursor",
		"small_cursor_with_rectangle_outline",
		"triangle_over_axis_background_plus_four_direction_arrows",
		"triangle_over_axis_background_plus_arc_rotate_arrow",
		"triangle_over_axis_background_plus_perspective_scale_arrows",
		"gray_background_with_black_UV_text",
		"gear_cog",
		"play_icon",
		"pause_icon",
		"stop_icon",
		"plus_icon",
		"minus_icon",
		"scene_metrics_glyph",
		"scene_postfx_glyph",
	};

	bool known = false;
	for (const char* k : kKnown) {
		if (key == k) {
			known = true;
			break;
		}
	}
	if (!known) {
		LOG("Ribbon icon key not registered: " << key);
		return GetRibbonIconFallback(key);
	}

	Vector<String> candidates;
	bool dark = IsDarkTheme();
	const char* theme = dark ? "dark" : "light";

	// New tabler layout.
	candidates.Add(ShareDirFile(AppendFileName("icons/" + String(theme) + "/tabler/outline", key + "_48.png")));
	candidates.Add(ShareDirFile(AppendFileName("icons/" + String(theme) + "/tabler/outline", key + "_24.png")));
	candidates.Add(ShareDirFile(AppendFileName("icons/" + String(theme) + "/tabler/filled", key + "_48.png")));
	candidates.Add(ShareDirFile(AppendFileName("icons/" + String(theme) + "/tabler/filled", key + "_24.png")));
	candidates.Add(ShareDirFile(AppendFileName("icons/" + String(theme) + "/tabler", key + "_48.png")));
	candidates.Add(ShareDirFile(AppendFileName("icons/" + String(theme) + "/tabler", key + "_24.png")));

	// Compatibility tabler locations (without theme split).
	candidates.Add(ShareDirFile(AppendFileName("icons/tabler/outline", key + "_48.png")));
	candidates.Add(ShareDirFile(AppendFileName("icons/tabler/outline", key + "_24.png")));
	candidates.Add(ShareDirFile(AppendFileName("icons/tabler/filled", key + "_48.png")));
	candidates.Add(ShareDirFile(AppendFileName("icons/tabler/filled", key + "_24.png")));
	candidates.Add(ShareDirFile(AppendFileName("icons/tabler", key + "_48.png")));
	candidates.Add(ShareDirFile(AppendFileName("icons/tabler", key + "_24.png")));

	// Legacy locations.
	candidates.Add(ShareDirFile(AppendFileName("icons/large", key + ".png")));
	candidates.Add(ShareDirFile(AppendFileName("icons/small", key + ".png")));
	candidates.Add(ShareDirFile(AppendFileName("icons", key + "_48.png")));
	candidates.Add(ShareDirFile(AppendFileName("icons", key + "_24.png")));
	candidates.Add(ShareDirFile(AppendFileName("icons", key + ".png")));

	for (const String& path : candidates) {
		if (!FileExists(path))
			continue;
		Image img = StreamRaster::LoadFileAny(path);
		if (!img.IsEmpty())
			return Colorize(img, SColorText());
	}

	LOG("Ribbon icon file missing: " << key);
	return GetRibbonIconFallback(key);
}

} // namespace

ModelerAppRibbon::ModelerAppRibbon()
{
	const RibbonStyle& s = RibbonBar::StyleDefault();
	int h = s.full_button.cy + s.label_gap + s.full_font.GetHeight() + DPI(32);
	Height(h);
	WhenTabMenu = [this](Bar& bar) {
		bar.Add(t_("Show Tabs Only"), [this] { SetDisplayMode(RibbonBar::RIBBON_TABS); })
			.Check(GetDisplayMode() == RibbonBar::RIBBON_TABS);
		bar.Add(t_("Always Show Ribbon"), [this] { SetDisplayMode(RibbonBar::RIBBON_ALWAYS); })
			.Check(GetDisplayMode() == RibbonBar::RIBBON_ALWAYS);
		bar.Add(t_("Auto-hide Ribbon"), [this] { SetDisplayMode(RibbonBar::RIBBON_AUTOHIDE); })
			.Check(GetDisplayMode() == RibbonBar::RIBBON_AUTOHIDE);
		bar.Separator();
		bar.Add(t_("QAT Top"), [this] { SetQuickAccessPos(RibbonBar::QAT_TOP); })
			.Check(GetQuickAccessPos() == RibbonBar::QAT_TOP);
		bar.Add(t_("QAT Bottom"), [this] { SetQuickAccessPos(RibbonBar::QAT_BOTTOM); })
			.Check(GetQuickAccessPos() == RibbonBar::QAT_BOTTOM);
	};
}

void ModelerAppRibbon::Init(Edit3D* o)
{
	Clear();
	control_by_id.Clear();
	owned_ctrls.Clear();
	
	owner = o;
	SetupQuickAccess();
	BuildDefaultTabs();
}

void ModelerAppRibbon::BuildDefaultTabs()
{
	// Keep default setup idempotent even if called multiple times.
	ClearTabs();
	control_by_id.Clear();
	owned_ctrls.Clear();

	RibbonPage& page_main = AddTab("Main tools");
	{
		RibbonGroup& g_file = page_main.AddGroup("File");
		g_file.SetLarge([this](Bar& bar) {
			bar.Add("New file", [this] { OnAction("new_file"); }).Image(GetRibbonIcon("new_file", "generic_new_file"));
			bar.Add("Open file", [this] { OnAction("open_file"); }).Image(GetRibbonIcon("open_file", "generic_open_file"));
			bar.Add("Save file", [this] { OnAction("save_file"); }).Image(GetRibbonIcon("save_file", "generic_save_file"));
		});

		RibbonGroup& g_hist = page_main.AddGroup("History");
		g_hist.SetLarge([this](Bar& bar) {
			bar.Add("Undo", [this] { OnAction("undo"); }).Image(GetRibbonIcon("undo", "generic_undo"));
			bar.Add("Redo", [this] { OnAction("redo"); }).Image(GetRibbonIcon("redo", "generic_redo"));
		});
		
		RibbonGroup& g_playback = page_main.AddGroup("Playback");
		g_playback.SetLarge([this](Bar& bar) {
			bar.Add("Play (F5)", [this] { OnAction("play_timeline"); })
				.Image(GetRibbonIcon("play_timeline", "play_icon")).Key(K_F5);
			bar.Add("Pause (F5)", [this] { OnAction("pause_timeline"); })
				.Image(GetRibbonIcon("pause_timeline", "pause_icon")).Key(K_F5);
			bar.Add("Stop (F6)", [this] { OnAction("stop_timeline"); })
				.Image(GetRibbonIcon("stop_timeline", "stop_icon")).Key(K_F6);
			bar.Add("Repeat", [this] { OnAction("repeat_playback"); })
				.Image(GetRibbonIcon("repeat_playback", "clockwise_arc_arrow"));
		});

		RibbonGroup& g_sel = page_main.AddGroup("Selection");
		g_sel.SetLarge([this](Bar& bar) {
			bar.Add("Select objects", [this] { OnAction("select_objects"); }).Image(GetRibbonIcon("select_objects", "mouse_cursor"));
			bar.Add("Move camera (Q)", [this] { OnAction("move_camera"); }).Image(GetRibbonIcon("move_camera", "mouse_cursor")).Key(K_Q);
			bar.Add("Move object (W)", [this] { OnAction("move_object"); }).Image(GetRibbonIcon("move_object", "four_arrows_cross")).Key(K_W);
			bar.Add("Rotate object (E)", [this] { OnAction("rotate_object"); }).Image(GetRibbonIcon("rotate_object", "clockwise_arc_arrow")).Key(K_E);
			bar.Add("Scale object (R)", [this] { OnAction("scale_object"); }).Image(GetRibbonIcon("scale_object", "3d_scale_arrows")).Key(K_R);
		});

		RibbonGroup& g_view = page_main.AddGroup("View");
		g_view.SetLarge([this](Bar& bar) {
			bar.Add("Change current view to perspective (F1)", [this] { OnAction("view_perspective"); }).Image(GetRibbonIcon("view_perspective", "frame_plus_3d_perspective_block")).Key(K_F1);
			bar.Add("Change current view to top (F2)", [this] { OnAction("view_top"); }).Image(GetRibbonIcon("view_top", "frame_plus_top_projection_flat_wide_block")).Key(K_F2);
			bar.Add("Change current view to front (F3)", [this] { OnAction("view_front"); }).Image(GetRibbonIcon("view_front", "frame_plus_front_square_projection")).Key(K_F3);
			bar.Add("Change current view to left (F4)", [this] { OnAction("view_left"); }).Image(GetRibbonIcon("view_left", "frame_plus_left_tall_narrow_projection")).Key(K_F4);
		});
	}

	RibbonPage& page_create = AddTab("Create");
	{
		RibbonGroup& g = page_create.AddGroup("Create");
		g.SetLarge([this](Bar& bar) {
			bar.Add("Create a cube", [this] { OnAction("create_cube"); }).Image(GetRibbonIcon("create_cube", "gray_cube_with_black_wireframe"));
			bar.Add("Create a sphere", [this] { OnAction("create_sphere"); }).Image(GetRibbonIcon("create_sphere", "gray_rough_sphere_with_black_wireframe"));
			bar.Add("Create a cylinder", [this] { OnAction("create_cylinder"); }).Image(GetRibbonIcon("create_cylinder", "gray_rough_cylinder_with_black_wireframe"));
			bar.Add("Create a cone", [this] { OnAction("create_cone"); }).Image(GetRibbonIcon("create_cone", "gray_rough_cone_with_black_wireframe"));
			bar.Add("Create a plane", [this] { OnAction("create_plane"); }).Image(GetRibbonIcon("create_plane", "gray_square_plane_in_perspective_with_black_wireframe"));
			bar.Add("Create a camera", [this] { OnAction("create_camera"); }).Image(GetRibbonIcon("create_camera", "classic_film_camera_icon_gray_black"));
			bar.Add("Create a terrain", [this] { OnAction("create_terrain"); }).Image(GetRibbonIcon("create_terrain", "side_profile_terrain_curve_with_filled_ground_polygon"));
			bar.Add("Create a room mesh from a 2D map", [this] { OnAction("create_room_mesh_from_2d_map"); }).Image(GetRibbonIcon("create_room_mesh_from_2d_map", "coarse_3d_corridor_shape"));
			bar.Add("Import a static mesh", [this] { OnAction("import_static_mesh"); }).Image(GetRibbonIcon("import_static_mesh", "3d_cube_like_import_symbol"));
			bar.Add("Import an animated mesh", [this] { OnAction("import_animated_mesh"); }).Image(GetRibbonIcon("import_animated_mesh", "3d_cube_like_import_symbol_animated"));
			bar.Add("Create a point light", [this] { OnAction("create_point_light"); }).Image(GetRibbonIcon("create_point_light", "yellow_glowing_lightbulb"));
			bar.Add("Create a directional light", [this] { OnAction("create_directional_light"); }).Image(GetRibbonIcon("create_directional_light", "sun_icon_with_arrow_left_down"));
			bar.Add("Create a tree (plant)", [this] { OnAction("create_tree"); }).Image(GetRibbonIcon("create_tree", "green_2d_fir_tree"));
			bar.Add("Create a water surface", [this] { OnAction("create_water_surface"); }).Image(GetRibbonIcon("create_water_surface", "blue_plane_in_perspective"));
			bar.Add("Create a billboard", [this] { OnAction("create_billboard"); }).Image(GetRibbonIcon("create_billboard", "gray_background_with_light_B"));
			bar.Add("Create a vertical billboard", [this] { OnAction("create_vertical_billboard"); }).Image(GetRibbonIcon("create_vertical_billboard", "narrower_taller_variant_of_billboard_icon"));
			bar.Add("Create a particle system", [this] { OnAction("create_particle_system"); }).Image(GetRibbonIcon("create_particle_system", "small_gray_particle_with_yellow_arrows_up_left_and_up_right"));
			bar.Add("Create a skybox", [this] { OnAction("create_skybox"); }).Image(GetRibbonIcon("create_skybox", "inside_cube_view_blue_sky_walls_gray_ground"));
			bar.Add("Create a 3D sound", [this] { OnAction("create_3d_sound"); }).Image(GetRibbonIcon("create_3d_sound", "speaker_cone"));
			bar.Add("Create a 2D overlay item", [this] { OnAction("create_2d_overlay_item"); }).Image(GetRibbonIcon("create_2d_overlay_item", "gray_background_with_dark_2D_text"));
			bar.Add("Create a 2D touchscreen input item", [this] { OnAction("create_2d_touchscreen_input_item"); }).Image(GetRibbonIcon("create_2d_touchscreen_input_item", "2d_overlay_symbol_with_circular_background"));
			bar.Add("Create a path", [this] { OnAction("create_path"); }).Image(GetRibbonIcon("create_path", "two_points_connected_by_line"));
			bar.Add("Create a path node", [this] { OnAction("create_path_node"); }).Image(GetRibbonIcon("create_path_node", "path_like_symbol_with_additional_points"));
		});
	}

	RibbonPage& page_polygon_editing = AddTab("Polygon Editing");
	{
		RibbonGroup& g = page_polygon_editing.AddGroup("Polygon Tools");
		g.SetLarge([this](Bar& bar) {
			bar.Add("Select (Add=Shift, Remove=Alt)", [this] { OnAction("poly_select"); }).Image(GetRibbonIcon("poly_select", "triangle_over_axis_background_with_mouse_cursor"));
			bar.Add("Select by Rectangle (Add=Shift, Remove=Alt)", [this] { OnAction("poly_select_by_rectangle"); }).Image(GetRibbonIcon("poly_select_by_rectangle", "small_cursor_with_rectangle_outline"));
			bar.Add("Move Selected Polygons", [this] { OnAction("poly_move_selected"); }).Image(GetRibbonIcon("poly_move_selected", "triangle_over_axis_background_plus_four_direction_arrows"));
			bar.Add("Rotate Selected Polygons", [this] { OnAction("poly_rotate_selected"); }).Image(GetRibbonIcon("poly_rotate_selected", "triangle_over_axis_background_plus_arc_rotate_arrow"));
			bar.Add("Scale Selected Polygons", [this] { OnAction("poly_scale_selected"); }).Image(GetRibbonIcon("poly_scale_selected", "triangle_over_axis_background_plus_perspective_scale_arrows"));
			bar.Add("Modify Texture Coordinates of Selected Points and Triangles", [this] { OnAction("poly_modify_uv"); }).Image(GetRibbonIcon("poly_modify_uv", "gray_background_with_black_UV_text"));
		});
	}

	RibbonGroup& group_poly_edit_mode = page_polygon_editing.AddGroup("Edit Mode");
	One<Ctrl>& poly_slot = owned_ctrls.Add();
	poly_slot.Create<FormPanel>();
	FormPanel& poly_form = static_cast<FormPanel&>(*poly_slot);
	poly_form.SetMetrics(DPI(22), DPI(6), DPI(120));
	DropList& poly_mode = poly_form.AddRowCtrl<DropList>("Edit Mode");
	poly_mode.LayoutId("poly_edit_mode");
	poly_mode.Add("triangle", "Triangle");
	poly_mode.Add("point", "Point");
	poly_mode.SetIndex(0);
	poly_mode.WhenAction = [this] { OnAction("poly_edit_mode"); };
	Button& poly_tools = poly_form.AddRowCtrl<Button>(String());
	poly_tools.SetLabel("Tools");
	poly_tools.WhenAction = [this] { OnAction("poly_tools"); };
	group_poly_edit_mode.SetListCtrl(poly_form);
	group_poly_edit_mode.SetContentMinSize(Size(DPI(260), DPI(24)));

	RibbonPage& page_lightmapping = AddTab("Lightmapping");
	RibbonGroup& group_lightmapping_controls = page_lightmapping.AddGroup("Bake Controls");
	One<Ctrl>& light_slot = owned_ctrls.Add();
	light_slot.Create<LightmapRowCtrl>();
	LightmapRowCtrl& light_row = static_cast<LightmapRowCtrl&>(*light_slot);
	light_row.SetGap(DPI(10));
	Button& calc = light_row.AddCtrl<Button>(String());
	calc.LayoutId("calculate_lightmap");
	calc.SetLabel("Calculate!");
	calc.WhenAction = [this] { OnAction("calculate_lightmap"); };
	DropList& lm_mode = light_row.AddCtrl<DropList>("Mode");
	lm_mode.LayoutId("lightmap_mode");
	lm_mode.Add("wide", "Wide");
	lm_mode.Add("diffuse", "Diffuse");
	lm_mode.Add("shadows", "Shadows");
	lm_mode.Add("gi", "Global Illumination");
	lm_mode.SetIndex(0);
	lm_mode.WhenAction = [this] { OnAction("lightmap_mode"); };
	DropList& lm_tex = light_row.AddCtrl<DropList>("Texture size");
	lm_tex.LayoutId("lightmap_texture_size");
	lm_tex.Add("64", "64");
	lm_tex.Add("128", "128");
	lm_tex.Add("256", "256");
	lm_tex.Add("512", "512");
	lm_tex.Add("1024", "1024");
	lm_tex.SetIndex(0);
	lm_tex.WhenAction = [this] { OnAction("lightmap_texture_size"); };
	EditIntSpin& lm_res = light_row.AddCtrl<EditIntSpin>("Resolution");
	lm_res.LayoutId("lightmap_resolution");
	lm_res.SetData(1000);
	lm_res.WhenAction = [this] { OnAction("lightmap_resolution"); };
	EditDoubleSpin& lm_shadow = light_row.AddCtrl<EditDoubleSpin>("Shadow Opacity");
	lm_shadow.LayoutId("lightmap_shadow_opacity");
	lm_shadow.SetData(0.8);
	lm_shadow.WhenAction = [this] { OnAction("lightmap_shadow_opacity"); };
	DropList& lm_sub = light_row.AddCtrl<DropList>("Subsampling");
	lm_sub.LayoutId("lightmap_subsampling");
	lm_sub.Add("none", "None");
	lm_sub.Add("4x", "4X");
	lm_sub.Add("9x", "9X");
	lm_sub.Add("16x", "16X");
	lm_sub.Add("25x", "25X");
	lm_sub.SetIndex(0);
	lm_sub.WhenAction = [this] { OnAction("lightmap_subsampling"); };
	Option& ambient = light_row.AddCtrl<Option>("Ambient Light");
	ambient.LayoutId("ambient_light_enabled");
	ambient = true;
	ambient.WhenAction = [this] { OnAction("ambient_light_enabled"); };
	Button& ambient_more = light_row.AddCtrl<Button>(String());
	ambient_more.SetLabel("...");
	ambient_more.WhenAction = [this] { OnAction("ambient_light_more"); };
	Option& smooth = light_row.AddCtrl<Option>("Smooth Normals");
	smooth.LayoutId("smooth_normals_enabled");
	smooth = true;
	smooth.WhenAction = [this] { OnAction("smooth_normals_enabled"); };
	Option& bleed = light_row.AddCtrl<Option>("Color Bleeding");
	bleed.LayoutId("color_bleeding_enabled");
	bleed = true;
	bleed.WhenAction = [this] { OnAction("color_bleeding_enabled"); };
	group_lightmapping_controls.SetListCtrl(light_row);
	group_lightmapping_controls.SetContentMinSize(Size(DPI(900), DPI(24)));

	RibbonPage& page_scenes = AddTab("Scenes");
	{
		RibbonGroup& group_scene_dropdown = page_scenes.AddGroup("Scene");
		One<Ctrl>& scene_slot = owned_ctrls.Add();
		scene_slot.Create<FormPanel>();
		FormPanel& scene_form = static_cast<FormPanel&>(*scene_slot);
		scene_form.SetMetrics(DPI(22), DPI(6), DPI(160));
		DropList& scene_sel = scene_form.AddRowCtrl<DropList>(String());
		scene_sel.LayoutId("scene_selector");
		scene_sel.Add("new_scene", "New 3D Scene");
		scene_sel.SetIndex(0);
		scene_sel.WhenAction = [this] { OnAction("scene_selector"); };
		group_scene_dropdown.SetListCtrl(scene_form);
		group_scene_dropdown.SetContentMinSize(Size(DPI(220), DPI(24)));
	}
	{
		RibbonGroup& group_scene_actions = page_scenes.AddGroup("Scene Actions");
		group_scene_actions.SetLarge([this](Bar& bar) {
			bar.Add("Add a new scene", [this] { OnAction("scene_add"); })
				.Image(GetRibbonIcon("scene_add", "plus_icon"));
			bar.Add("Delete a scene", [this] { OnAction("scene_delete"); })
				.Image(GetRibbonIcon("scene_delete", "minus_icon"));
			bar.Add("Scene Settings", [this] { OnAction("scene_settings"); })
				.Image(GetRibbonIcon("scene_settings", "gear_cog"));
			bar.Add("Scene Metrics", [this] { OnAction("scene_metrics"); })
				.Image(GetRibbonIcon("scene_metrics", "scene_metrics_glyph"));
			bar.Add("Scene Post Effects", [this] { OnAction("scene_post_effects"); })
				.Image(GetRibbonIcon("scene_post_effects", "scene_postfx_glyph"));
		});
	}

	RibbonPage& page_publish = AddTab("Publish");
	{
		RibbonGroup& group_publish_dropdown = page_publish.AddGroup("Target");
		One<Ctrl>& pub_slot = owned_ctrls.Add();
		pub_slot.Create<FormPanel>();
		FormPanel& pub_form = static_cast<FormPanel&>(*pub_slot);
		pub_form.SetMetrics(DPI(22), DPI(6), DPI(200));
		DropList& pub_target = pub_form.AddRowCtrl<DropList>(String());
		pub_target.LayoutId("publish_target");
		pub_target.Add("win_exe", "Publish as Windows (.exe)");
		pub_target.Add("webgl", "Publish as WebGL (.html)");
		pub_target.Add("mac_app", "Publish as MacOS (.app)");
		pub_target.Add("android_apk", "Publish as Android (.apk)");
		pub_target.SetIndex(0);
		pub_target.WhenAction = [this] { OnAction("publish_target"); };
		group_publish_dropdown.SetListCtrl(pub_form);
		group_publish_dropdown.SetContentMinSize(Size(DPI(260), DPI(24)));
	}
	{
		RibbonGroup& group_publish_actions = page_publish.AddGroup("Publish");
		group_publish_actions.SetLarge([this](Bar& bar) {
			bar.Add("Publishing Settings", [this] { OnAction("publishing_settings"); })
				.Image(GetRibbonIcon("publishing_settings", "gear_cog"));
			bar.Add("Publish and test the application", [this] { OnAction("publish_and_test"); })
				.Image(GetRibbonIcon("publish_and_test", "play_icon"));
		});
	}

	AddContextTabs();
	SetDisplayMode(RibbonBar::RIBBON_ALWAYS);
	ShowContext("picture", false);
	ShowContext("camera", false);
	ShowContext("object", false);
	ShowContext("mesh", false);
	ShowContext("material", false);
	loaded = true;
}

void ModelerAppRibbon::Clear()
{
	ClearTabs();
	control_by_id.Clear();
	owned_ctrls.Clear();
	action_handlers.Clear();
	loaded = false;
}

void ModelerAppRibbon::OnAction(const String& id)
{
	WhenAction(id);
	bool handled = false;
	int idx = action_handlers.Find(id);
	if (idx >= 0) {
		action_handlers[idx]();
		handled = true;
	}
	if (!handled && owner) {
		owner->HandleRibbonAction(id);
		handled = true;
	}
	if (!handled)
		LOG("Ribbon action unhandled: " << id);
}

Ctrl* ModelerAppRibbon::FindControl(const String& id) const
{
	int idx = control_by_id.Find(id);
	if(idx >= 0)
		return control_by_id[idx];

	const Ctrl* root = this;
	Vector<const Ctrl*> stack;
	stack.Add(root);
	while(!stack.IsEmpty()) {
		int n = stack.GetCount() - 1;
		const Ctrl* c = stack[n];
		stack.SetCount(n);
		if(c->GetLayoutId() == id)
			return const_cast<Ctrl*>(c);
		for(const Ctrl* child = c->GetFirstChild(); child; child = child->GetNext())
			stack.Add(child);
	}
	return nullptr;
}

void ModelerAppRibbon::BindAction(const String& id, Event<> cb)
{
	int idx = action_handlers.Find(id);
	if (idx >= 0)
		action_handlers[idx] = cb;
	else
		action_handlers.Add(id, cb);
}

void ModelerAppRibbon::SetupQuickAccess()
{
	SetQuickAccess([this](Bar& b) {
		b.Add(t_("New"), [this] { OnAction("new_file"); }).Image(CtrlImg::new_doc());
		b.Add(t_("Open"), [this] { OnAction("open_file"); }).Image(CtrlImg::open());
		b.Add(t_("Save"), [this] { OnAction("save_file"); }).Image(CtrlImg::save());
		b.Separator();
		b.Add(t_("Undo"), [this] { OnAction("undo"); }).Image(CtrlImg::undo());
		b.Add(t_("Redo"), [this] { OnAction("redo"); }).Image(CtrlImg::redo());
	});
}

void ModelerAppRibbon::AddContextTabs()
{
	RibbonPage& cam = AddContextTab("camera", "Camera Tools");
	RibbonGroup& g = cam.AddGroup("Camera");
	g.SetLarge([this](Bar& b) {
		b.Add(t_("Make Active"), [this] { OnAction("camera_make_active"); }).Image(CtrlImg::smallcheck());
		b.Add(t_("Focus Selected"), [this] { OnAction("focus_selected"); }).Image(CtrlImg::arrow());
	});

	RibbonPage& obj = AddContextTab("object", "Object Tools");
	RibbonGroup& og = obj.AddGroup("Object");
	og.SetLarge([this](Bar& b) {
		b.Add(t_("Focus Selected"), [this] { OnAction("focus_selected"); }).Image(CtrlImg::arrow());
		b.Add(t_("Move object (W)"), [this] { OnAction("move_object"); }).Image(GetRibbonIcon("move_object", "four_arrows_cross")).Key(K_W);
		b.Add(t_("Rotate object (E)"), [this] { OnAction("rotate_object"); }).Image(GetRibbonIcon("rotate_object", "clockwise_arc_arrow")).Key(K_E);
	});

	RibbonPage& mesh = AddContextTab("mesh", "Mesh Tools");
	RibbonGroup& mg = mesh.AddGroup("Mesh");
	mg.SetLarge([this](Bar& b) {
		b.Add(t_("Select"), [this] { OnAction("poly_select"); }).Image(GetRibbonIcon("poly_select", "triangle_over_axis_background_with_mouse_cursor"));
		b.Add(t_("Move Selected"), [this] { OnAction("poly_move_selected"); }).Image(GetRibbonIcon("poly_move_selected", "triangle_over_axis_background_plus_four_direction_arrows"));
		b.Add(t_("Modify UV"), [this] { OnAction("poly_modify_uv"); }).Image(GetRibbonIcon("poly_modify_uv", "gray_background_with_black_UV_text"));
	});

	RibbonPage& mat = AddContextTab("material", "Material Tools");
	RibbonGroup& mag = mat.AddGroup("Material");
	mag.SetLarge([this](Bar& b) {
		b.Add(t_("Modify UV"), [this] { OnAction("poly_modify_uv"); }).Image(GetRibbonIcon("poly_modify_uv", "gray_background_with_black_UV_text"));
		b.Add(t_("Scene Post Effects"), [this] { OnAction("scene_post_effects"); }).Image(GetRibbonIcon("scene_post_effects", "scene_postfx_glyph"));
	});
}

END_UPP_NAMESPACE

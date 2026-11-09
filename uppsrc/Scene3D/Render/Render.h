#ifndef _Scene3D_Render_Render_h_
#define _Scene3D_Render_Render_h_

#include <CtrlLib/CtrlLib.h>
#include <GLCtrl/GLCtrl.h>
#ifdef Complex
#undef Complex
#endif
#ifdef None
#undef None
#endif
#ifdef True
#undef True
#endif
#ifdef False
#undef False
#endif
#include <Scene3D/Core/Core.h>
#include <Scene3D/IO/IO.h>

NAMESPACE_UPP

struct Scene3DRenderConfig {
	Color background_clr;
	bool show_grid = true;
	float grid_major_step = 1.0f;
	float grid_extent = 20.0f;
	int grid_minor_divs = 5;
	Color grid_major_clr;
	Color grid_minor_clr;
	String dump_grid_path;
	bool dump_grid_done = false;
	float mouse_move_sensitivity = 0.01f;

	Scene3DRenderConfig() {
		background_clr = Color(43, 44, 46);
		grid_major_clr = Color(70, 72, 76);
		grid_minor_clr = Color(56, 58, 62);
	}
};

struct Scene3DRenderContext {
	Scene3DRenderConfig* conf = 0;
	GeomWorldState* state = 0;
	GeomAnim* anim = 0;
	GeomVideo* video = 0;
	VfsValue* selected_bone = 0;
	bool show_weights = false;
	String weight_bone;
	Vector<String> hud_lines;
	Vector<String> hud_help;
	bool show_hud = true;
	bool show_hud_help = false;
	Vector<int> selected_mesh_points;
	Vector<int> selected_mesh_lines;
	Vector<int> selected_mesh_faces;
	Vector<int> selected_2d_shapes;
	vec3 selection_center_world = vec3(0);
	bool selection_center_valid = false;
	bool selection_gizmo_enabled = false;
	int selection_kind = 0;
	int gizmo_pixels = 0;
};

struct Scene3DRenderStats {
	int models = 0;
	int triangles = 0;
	int pixels = 0;
	bool rendered = false;
};

bool RenderSceneV2Headless(Scene3DRenderContext& ctx, Size sz, Scene3DRenderStats* out_stats = nullptr,
                           Image* out_image = nullptr, String* out_debug = nullptr, bool dump_first_tri = false,
                           ViewMode view_mode = VIEWMODE_PERSPECTIVE, const GeomCamera* cam_override = nullptr,
                           bool wireframe_only = false);

#include "Renderer.h"
#include "HeadlessTools.h"
#include "SoftRendCtrl.h"

END_UPP_NAMESPACE

#endif

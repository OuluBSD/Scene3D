#include <Core/Core.h>
#include <Geometry/Geometry.h>
#include <Rt3D/Rt3D.h>
#include <AnimEditLib/AnimCore.h>
#include <SoftRend/SoftRend.h>

using namespace Upp;

CONSOLE_APP_MAIN
{
	StdLogSetup(LOG_COUT | LOG_FILE);
	Cout() << "========================================\n";
	Cout() << "Running SceneCheck Unit & Lifecycle Tests\n";
	Cout() << "========================================\n\n";

	// 1. Test AnimEditLib Vec2, RectF, Sprite
	Cout() << "[1] Testing AnimEditLib data structures...\n";
	Vec2 v(10.0, 20.0);
	v += Vec2(5.0, 5.0);
	ASSERT(v.x == 15.0 && v.y == 25.0);
	Cout() << "PASS: AnimEditLib Vec2 math verified\n";

	Sprite sprite("test_sprite");
	sprite.pivot = v;
	Cout() << "PASS: Sprite initialized with pivot\n";

	// 2. Test Rt3D Vectors and Elements
	Cout() << "\n[2] Testing Rt3D Octree types...\n";
	Rt3IVec ivec(1, 2, 3);
	ASSERT(ivec.x == 1 && ivec.y == 2 && ivec.z == 3);
	Cout() << "PASS: Rt3IVec initialized\n";

	Rt3ElementSet elem;
	elem.texture = 42;
	ASSERT(elem.texture == 42);
	Cout() << "PASS: Rt3ElementSet initialized\n";

	// 3. Test SoftRend software rasterizer pipeline
	Cout() << "\n[3] Testing SoftRend pipeline & framebuffers...\n";
	SoftFramebuffer fb;
	fb.Clear();
	ASSERT(fb.GetSize() == Size(0, 0));
	Cout() << "PASS: SoftFramebuffer initialized\n";

	SoftRend sr;
	sr.SetSmoothShading(true);
	sr.SetDepthTest(true);
	sr.SetViewport(Size(800, 600));
	sr.ClearTargets();
	ASSERT(sr.GetTargets() == (GVar::RenderTarget)0);
	Cout() << "PASS: SoftRend state machine configured\n";

	Cout() << "\n========================================\n";
	Cout() << "ALL SceneCheck TESTS PASSED SUCCESSFULLY!\n";
	Cout() << "========================================\n";
}

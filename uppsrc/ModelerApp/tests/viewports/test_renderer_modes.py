log("test_renderer_modes start")
wait_ready()
wait_time(0.2)


def fail(msg):
    log("Error:", msg)
    log(dump_ui())
    exit(1)

geo = find("View/Geometry")
if geo:
    geo.click()
    wait_time(0.1)

hud = find("View/HUD/Show HUD")
if not hud:
    fail("HUD menu entry missing")

if hud.checked:
    hud.click()
    wait_time(0.1)
    hud = find("View/HUD/Show HUD")
if hud.checked:
    fail("HUD state did not toggle off")

hud.click()
wait_time(0.1)
hud = find("View/HUD/Show HUD")
if not hud.checked:
    fail("HUD state did not toggle on")

if not modeler.set_renderer_version(0, 1):
    fail("set_renderer_version V1 failed")
wait_time(0.1)
if modeler.get_renderer_version(0) != 1:
    fail("renderer version not V1")

if not modeler.set_renderer_version(0, 2):
    fail("set_renderer_version V2 failed")
wait_time(0.1)
if modeler.get_renderer_version(0) != 2:
    fail("renderer version not V2")

if not modeler.set_renderer_version(0, 3):
    fail("set_renderer_version V2 Ogl failed")
wait_time(0.1)
if modeler.get_renderer_version(0) != 3:
    fail("renderer version not V2 Ogl")

if not modeler.set_renderer_wireframe(0, True):
    fail("set_renderer_wireframe true failed")
wait_time(0.1)
if not modeler.get_renderer_wireframe(0):
    fail("wireframe state not enabled")

if not modeler.set_renderer_wireframe(0, False):
    fail("set_renderer_wireframe false failed")
wait_time(0.1)
if modeler.get_renderer_wireframe(0):
    fail("wireframe state not disabled")

modeler.set_show_grid(False)
wait_time(0.1)
if modeler.get_show_grid():
    fail("grid state did not toggle off")

modeler.set_show_grid(True)
wait_time(0.1)
if not modeler.get_show_grid():
    fail("grid state did not toggle on")

log("OK: renderer modes")
exit(0)

log("test_viewport_tools_and_gizmo start")
wait_ready()
wait_time(0.3)

def fail(msg):
    log("Error:", msg)
    log(dump_ui())
    exit(1)

# Create a model and move it to origin
modeler.create_object("Scene #0/gizmo_model", "model")
modeler.set_position("Scene #0/gizmo_model", 0.0, 0.0, 0.0)

# Switch tool via menu
tool_move = find("Edit/Object Tools/Move")
if not tool_move:
    fail("Move tool menu not found")
tool_move.click()
wait_time(0.2)

# Enable HUD + debug to expose gizmo state
hud = find("View/HUD/Show HUD")
dbg = find("View/HUD/Show Debug")
if not hud or not dbg:
    fail("HUD menu entries missing")
if not hud.checked:
    hud.click()
    wait_time(0.1)
if not dbg.checked:
    dbg.click()
    wait_time(0.1)

# Select object via tree menu
node = find("Scene Tree/Tree Nodes/gizmo_model")
if node:
    node.click()
    wait_time(0.2)

# Trigger a render update and verify gizmo pixel count
pixels = modeler.get_gizmo_pixels()
if pixels is None:
    fail("gizmo pixel count missing")
if int(pixels) <= 0:
    fail("gizmo not rendered (pixels=" + str(pixels) + ")")

# Switch tool via shortcut (Q->select)
send_key(81)
wait_time(0.1)

log("OK: viewport tools and gizmo rendering")
exit(0)

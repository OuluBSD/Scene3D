log("test_timeline_controls start")
wait_ready()
wait_time(0.3)

def fail(msg):
    log("Error:", msg)
    log(dump_ui())
    exit(1)

# Create a model and add transform keyframes
modeler.create_object("Scene #0/timeline_model", "model")
modeler.set_position("Scene #0/timeline_model", 0.0, 0.0, 0.0)
modeler.add_transform_keyframe("Scene #0/timeline_model", 0, True, True)
modeler.set_position("Scene #0/timeline_model", 10.0, 0.0, 0.0)
modeler.add_transform_keyframe("Scene #0/timeline_model", 10, True, True)

# Expand timeline rows for the object
modeler.set_timeline_expanded("Scene #0/timeline_model", True)
wait_time(0.2)

# Verify timeline rows include transform sub-rows
ui = dump_ui()
if "Timeline/Rows/timeline_model / Transform" not in ui:
    fail("Transform row not present in Timeline rows")
if "Timeline/Rows/timeline_model / Position" not in ui:
    fail("Position row not present in Timeline rows")
if "Timeline/Rows/timeline_model / Orientation" not in ui:
    fail("Orientation row not present in Timeline rows")

# Scrub timeline and verify transform changes
stage.goto(0)
wait_time(0.2)
pos0 = modeler.get_position("Scene #0/timeline_model")
stage.goto(10)
wait_time(0.2)
pos10 = modeler.get_position("Scene #0/timeline_model")
stage.goto(5)
wait_time(0.2)
pos5 = modeler.get_position("Scene #0/timeline_model")

if not pos0 or not pos10 or not pos5:
    fail("Position readback failed after scrubbing")

if abs(pos0[0]) > 0.01:
    fail("Frame 0 position mismatch: " + str(pos0))
if abs(pos10[0] - 10.0) > 0.05:
    fail("Frame 10 position mismatch: " + str(pos10))
if pos5[0] < 0.5 or pos5[0] > 9.5:
    fail("Frame 5 position not interpolated: " + str(pos5))

# Ensure timeline selected column updates
tl = find("Timeline")
if not tl:
    fail("Timeline control not found")
if tl.value != 5:
    fail("Timeline selected column not updated: " + str(tl.value))

log("OK: timeline controls and scrubbing")
exit(0)

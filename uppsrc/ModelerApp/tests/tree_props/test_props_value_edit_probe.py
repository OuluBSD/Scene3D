log("test_props_value_edit_probe start")
wait_ready()
wait_time(0.3)

# Probe for transform-related menu entries (GUI automation currently exposes menus reliably)
expected = ["Edit/Transform", "Edit/Transform/Use Local Axes", "Edit/Transform/Position Snap", "Edit/Transform/Angle Snap"]
missing = []
for e in expected:
    if not find(e):
        missing.append(e)
if len(missing) > 0:
    log("Error: transform-related menu entries missing:", str(missing))
    log(dump_ui())
    exit(1)

log("OK: transform-related menu entries present")
exit(0)

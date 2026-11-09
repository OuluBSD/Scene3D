log("test_timeline_keyframe_labels start")
wait_ready()
wait_time(0.3)
expected = ["Edit/Mesh Animation/Add Keyframe", "Edit/Mesh Animation/Clear Keyframes", "Edit/2D Animation/Add Keyframe", "Edit/2D Animation/Clear Keyframes"]
missing = []
for e in expected:
    if not find(e):
        missing.append(e)
if len(missing) > 0:
    log("Error: keyframe-related menu entries missing:", str(missing))
    log(dump_ui())
    exit(1)

log("OK: keyframe-related menu entries present")
exit(0)

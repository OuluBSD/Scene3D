log("test_gizmo_visibility_hint start")
wait_ready()
wait_time(0.3)
if not find("Edit/Transform"):
    log("Error: Transform menu entry not found")
    log(dump_ui())
    exit(1)
log("OK: Transform menu entry present")
exit(0)

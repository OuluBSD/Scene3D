log("test_blend_mode_presence start")
wait_ready()
wait_time(0.3)
if not find("Edit/Plane/View Plane"):
    log("Error: Plane menu entry not found")
    log(dump_ui())
    exit(1)
log("OK: Plane menu entry present")
exit(0)

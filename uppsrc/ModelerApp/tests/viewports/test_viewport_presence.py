log("test_viewport_presence start")
wait_ready()
wait_time(0.3)
if not find("View/Geometry"):
    log("Error: View/Geometry menu entry not found")
    log(dump_ui())
    exit(1)
log("OK: View/Geometry menu entry present")
exit(0)

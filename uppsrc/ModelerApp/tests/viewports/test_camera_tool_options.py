log("test_camera_tool_options start")
wait_ready()
wait_time(0.3)
if not find("Edit/View (Camera)"):
    log("Error: Edit/View (Camera) menu entry not found")
    log(dump_ui())
    exit(1)
log("OK: camera menu entry present")
exit(0)

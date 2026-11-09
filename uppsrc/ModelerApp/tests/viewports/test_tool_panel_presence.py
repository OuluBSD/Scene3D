log("test_tool_panel_presence start")
wait_ready()
wait_time(0.3)
if not find("View/Tools Panel"):
    log("Error: View/Tools Panel menu entry not found")
    log(dump_ui())
    exit(1)
log("OK: Tools Panel menu entry present")
exit(0)

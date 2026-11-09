log("test_props_panel_presence start")
wait_ready()
wait_time(0.3)
if not find("Windows/All/Properties"):
    log("Error: Properties menu entry not found")
    log(dump_ui())
    exit(1)
log("OK: Properties menu entry present")
exit(0)

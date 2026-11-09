log("test_timeline_presence start")
wait_ready()
wait_time(0.3)
if not find("Windows/All/Timeline"):
    log("Error: Timeline menu entry not found")
    log(dump_ui())
    exit(1)
log("OK: Timeline menu entry present")
exit(0)

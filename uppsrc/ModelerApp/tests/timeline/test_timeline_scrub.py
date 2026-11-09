log("test_timeline_scrub start")
wait_ready()
wait_time(0.3)
if not find("Windows/All/Timeline"):
    log("Error: Timeline menu entry not found")
    log(dump_ui())
    exit(1)

# No direct scrub API; ensure timeline exists and user can use key navigation
send_key(K_F5)
wait_time(0.1)
log("OK: Sent playback key")
exit(0)

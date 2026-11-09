log("test_texture_editor_presence start")
wait_ready()
wait_time(0.3)
if not find("View/Texture Editor"):
    log("Error: Texture Editor menu entry not found")
    log(dump_ui())
    exit(1)
log("OK: Texture Editor menu entry present")
exit(0)

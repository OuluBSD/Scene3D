log("test_view_menu_entries start")
wait_ready()
wait_time(0.3)
expected = ["View/Geometry", "View/Tools Panel", "View/Tree", "View/Asset Browser", "View/Texture Editor", "View/Script Editor"]
missing = []
for e in expected:
    if not find(e):
        missing.append(e)
if len(missing) > 0:
    log("Error: missing View menu entries:", str(missing))
    log(dump_ui())
    exit(1)

log("OK: View menu entries present")
exit(0)

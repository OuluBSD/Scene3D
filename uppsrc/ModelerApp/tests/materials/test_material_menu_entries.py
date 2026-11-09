log("test_material_menu_entries start")
wait_ready()
wait_time(0.3)
expected = ["View/Texture Editor", "View/Asset Browser", "Edit/Plane/View Plane"]
missing = []
for e in expected:
    if not find(e):
        missing.append(e)
if len(missing) > 0:
    log("Error: missing material-related menu entries:", str(missing))
    log(dump_ui())
    exit(1)

log("OK: material-related menu entries present")
exit(0)

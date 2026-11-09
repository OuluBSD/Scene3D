log("test_menu_core_entries start")
wait_ready()
wait_time(0.3)
expected = ["File/New", "File/Open...", "File/Save", "Edit/Undo", "Edit/Redo", "View/Reset Layout", "Windows/Layouts/Default"]
missing = []
for e in expected:
    if not find(e):
        missing.append(e)
if len(missing) > 0:
    log("Error: missing core menu entries:", str(missing))
    log(dump_ui())
    exit(1)

log("OK: core menu entries present")
exit(0)

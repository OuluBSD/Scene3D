log("test_tool_menu_entries start")
wait_ready()
wait_time(0.3)
expected = ["Edit/Object Tools/Select", "Edit/Object Tools/Move", "Edit/Object Tools/Rotate", "Edit/Mesh Tools/Mesh Select", "Edit/Mesh Tools/Point Tool", "Edit/Mesh Tools/Line Tool", "Edit/Mesh Tools/Face Tool", "Edit/Mesh Tools/Erase Tool", "Edit/Mesh Tools/Join Tool", "Edit/Mesh Tools/Split Tool", "Edit/2D Tools/2D Select", "Edit/2D Tools/2D Line", "Edit/2D Tools/2D Rectangle", "Edit/2D Tools/2D Circle", "Edit/2D Tools/2D Polygon", "Edit/2D Tools/2D Erase"]
missing = []
for e in expected:
    if not find(e):
        missing.append(e)
if len(missing) > 0:
    log("Error: missing tool menu entries:", str(missing))
    log(dump_ui())
    exit(1)

log("OK: tool menu entries present")
exit(0)

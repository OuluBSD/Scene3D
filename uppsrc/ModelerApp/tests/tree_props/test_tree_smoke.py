log("test_tree_smoke start")
wait_ready()
wait_time(0.3)
expected = ["Windows/All/Scene Tree", "Windows/All/Properties", "Windows/All/Timeline", "View/Tree", "View/Tools Panel"]
missing = []
for e in expected:
    if not find(e):
        missing.append(e)
if len(missing) > 0:
    log("Error: missing expected menu entries:", str(missing))
    log(dump_ui())
    exit(1)

log("OK: menu-based tree/props/timeline entries present")
exit(0)

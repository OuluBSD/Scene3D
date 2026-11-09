log("test_tree_visibility_toggles start")
wait_ready()
wait_time(0.3)

# Menu-based visibility toggles and window visibility entries
expected = ["View/Tree", "View/HUD/Show HUD", "View/HUD/Show Help", "View/HUD/Show Status", "View/HUD/Show Debug", "Windows/All/Scene Tree/Close", "Windows/All/Properties/Close"]
missing = []
for e in expected:
    if not find(e):
        missing.append(e)
if len(missing) > 0:
    log("Error: expected visibility-related menu entries missing:", str(missing))
    log(dump_ui())
    exit(1)

log("OK: visibility-related menu entries present")
exit(0)

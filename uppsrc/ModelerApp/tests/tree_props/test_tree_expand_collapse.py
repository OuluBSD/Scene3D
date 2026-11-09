log("test_tree_expand_collapse start")
wait_ready()
wait_time(0.3)
parent = find("Windows/All/Scene Tree")
if not parent:
    log("Error: Scene Tree menu entry not found")
    log(dump_ui())
    exit(1)

# Clicking twice should expand/collapse if supported
parent.click()
wait_time(0.2)
parent.click()
wait_time(0.2)

log("OK: Scene Tree menu entry toggle attempt done")
exit(0)

log("test_tree_ops_add_nodes start")
wait_ready()
wait_time(0.3)

def fail(msg):
    log("Error:", msg)
    log(dump_ui())
    exit(1)

def click_menu(path):
    el = find(path)
    if not el:
        fail("Menu entry not found: " + path)
    el.click()
    wait_time(0.2)

expected = [
    "View/Tree/Go to",
    "View/Tree/Add/Directory",
    "View/Tree/Add/Camera",
    "View/Tree/Add/Model",
    "View/Tree/Add/Pointcloud",
    "View/Tree/Add/Pointcloud Dataset",
    "View/Tree/Add/Preset/Sphere",
]
missing = []
for e in expected:
    if not find(e):
        missing.append(e)
if len(missing) > 0:
    fail("Missing Tree menu entries: " + str(missing))

click_menu("View/Tree/Add/Directory")
click_menu("View/Tree/Add/Model")
click_menu("View/Tree/Add/Pointcloud Dataset")

ui = dump_ui()
if "Tree Nodes" not in ui:
    fail("Tree Nodes menu not present in UI dump")
if "Tree Nodes/dir" not in ui and "/dir" not in ui:
    fail("Directory node not found in Tree Nodes")
if "Tree Nodes/model" not in ui and "/model" not in ui:
    fail("Model node not found in Tree Nodes")
if "Tree Nodes/dataset" not in ui and "Pointcloud Dataset" not in ui:
    fail("Dataset node not found in Tree Nodes")

dir_node = find("Scene Tree/Tree Nodes/dir")
if dir_node:
    dir_node.click()
    wait_time(0.1)
model_node = find("Scene Tree/Tree Nodes/model")
if model_node:
    model_node.click()
    wait_time(0.1)

log("OK: tree ops add nodes + menu entries present")
exit(0)

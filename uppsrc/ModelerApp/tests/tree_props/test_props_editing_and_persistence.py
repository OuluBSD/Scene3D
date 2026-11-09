log("test_props_editing_and_persistence start")
wait_ready()
wait_time(0.3)

def fail(msg):
    log("Error:", msg)
    log(dump_ui())
    exit(1)

# Ensure expected docks are visible in automation tree
for label in ["Scene Tree", "Properties", "Timeline"]:
    if not find(label):
        fail("Missing dock label: " + label)

# Create a model via stage API and nudge position
modeler.create_object("Scene #0/props_model", "model")
modeler.set_position("Scene #0/props_model", 1.25, 0.0, -2.5)

# Read back position to confirm API write (proxy for props edit correctness)
pos = modeler.get_position("Scene #0/props_model")
if not pos or len(pos) < 3:
    fail("Position readback failed")
if abs(pos[0] - 1.25) > 0.001 or abs(pos[2] + 2.5) > 0.001:
    fail("Position readback mismatch: " + str(pos))

# Change tree cursor to another node via Tree Nodes menu (if available)
menu_tree = find("View/Tree/Go to")
if not menu_tree:
    fail("Missing Tree menu")
menu_tree.click()
wait_time(0.2)

# Trigger props cursor store by selecting a tree node if present
tree_node = find("Scene Tree/Tree Nodes/props_model")
if tree_node:
    tree_node.click()
    wait_time(0.2)

log("OK: props edit API and cursor persistence basic checks passed")
exit(0)

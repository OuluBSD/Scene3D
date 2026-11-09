log("test_window_layout_persistence start")
wait_ready()
wait_time(0.3)

def fail(msg):
    log("Error:", msg)
    log(dump_ui())
    exit(1)

def toggle_and_check(path):
    el = find(path)
    if not el:
        fail("Menu entry not found: " + path)
    orig = el.checked
    el.click()
    wait_time(0.2)
    el2 = find(path)
    if not el2:
        fail("Menu entry missing after toggle: " + path)
    if el2.checked == orig:
        fail("Menu entry did not toggle: " + path)
    el2.click()
    wait_time(0.2)
    el3 = find(path)
    if not el3 or el3.checked != orig:
        fail("Menu entry did not toggle back: " + path)

# Toggle core panels
toggle_and_check("Windows/All/Scene Tree")
toggle_and_check("Windows/All/Properties")
toggle_and_check("Windows/All/Timeline")

# Ensure layouts list exists
layout_default = find("Windows/Layouts/Default")
if not layout_default:
    fail("Default layout entry missing")

# Check that a layout file exists (persistence probe)
cfg_root = os.getenv("XDG_CONFIG_HOME", "")
if cfg_root == "":
    home = os.getenv("HOME", "")
    if home == "":
        fail("HOME/XDG_CONFIG_HOME not available")
    cfg_root = home + "/.config"
cfg = cfg_root + "/ModelerApp/ModelerApp.view"
if not os.path_exists(cfg):
    fail("Config file not found: " + cfg)

log("OK: window layout toggles and config file present")
exit(0)

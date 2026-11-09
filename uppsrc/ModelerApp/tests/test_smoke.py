log("ModelerApp smoke test: start")
wait_ready()
wait_time(0.3)

expected = ["Windows/All/Scene Tree", "Windows/All/Properties", "Windows/All/Timeline", "View/Geometry", "View/Tools Panel"]
missing = []
for e in expected:
    if not find(e):
        missing.append(e)
if len(missing) > 0:
    log("Error: baseline menu entries missing:", str(missing))
    log(dump_ui())
    exit(1)

log("ModelerApp smoke test: ok")
exit(0)

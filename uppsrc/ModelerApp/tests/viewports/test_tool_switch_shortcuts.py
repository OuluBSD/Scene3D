log("test_tool_switch_shortcuts start")
wait_ready()
wait_time(0.3)
# tool shortcuts: Q/W/E for object, A/S/D/F for mesh, etc.
send_key(81)  # Q
wait_time(0.05)
send_key(87)  # W
wait_time(0.05)
send_key(69)  # E
wait_time(0.05)
log("OK: Sent tool shortcut keys")
exit(0)

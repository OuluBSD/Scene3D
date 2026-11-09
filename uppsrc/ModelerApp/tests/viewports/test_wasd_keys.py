log("ModelerApp WASD key test: start")
wait_ready()
wait_time(0.2)

send_key(K_W)
send_key(K_A)
send_key(K_S)
send_key(K_D)

log("OK: Sent WASD keys")
exit(0)

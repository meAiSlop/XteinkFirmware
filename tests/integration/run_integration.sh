#!/usr/bin/env bash
set -euo pipefail
python3 tests/integration/mock_ha_server.py &
server_pid=$!
trap 'kill ${server_pid}' EXIT
sleep 1

cat > config/home_assistant.json <<JSON
{
  "ha_base_url": "http://127.0.0.1:8123",
  "entity_id": "cover.living_room_blind",
  "token": "dummy-token",
  "allowed_ssids": ["MyHomeWiFi"],
  "http_timeout_ms": 3000,
  "max_retries": 1
}
JSON

export XTEINK_CONNECTED=1
export XTEINK_SSID=MyHomeWiFi
export XTEINK_LOCAL_IP=192.168.1.44
./build/xteink_main
rm -f config/home_assistant.json

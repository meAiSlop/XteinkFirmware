# Xteink X4 Custom Firmware: eReader + Local Home Assistant Blind Control

This repository contains a custom firmware concept for the Xteink X4 that keeps core eReader behavior while adding **local-only Home Assistant control** for a blind.

## Goals
- Keep eReader workflows intact.
- Add a local dashboard card to:
  - read blind state (`open` / `closed` / position)
  - issue commands (`open` / `close`)
- Work **only** when connected to home Wi-Fi.
- Follow safe software development practices: least privilege, no cloud dependency, fail-safe networking, input validation, and explicit threat model.

## High-level architecture
- Existing eReader stack remains primary UI shell.
- New `ha_bridge` module runs as a bounded background service:
  - checks SSID/BSSID allowlist
  - queries Home Assistant local REST API
  - sends service calls for `cover.open_cover` / `cover.close_cover`
- New UI view `BlindControlView` renders current state and provides action buttons.

## Build notes
Build with CMake:

```bash
cmake -S . -B build
cmake --build build -j
ctest --test-dir build --output-on-failure
```

For reproducible artifact flow:

```bash
scripts/build_repro.sh
```

This builds `build/xteink_main` and emits `build/xteink_main.sha256`.

## Configuration
Copy and edit:

```bash
cp config/home_assistant.example.json config/home_assistant.json
```

Fill:
- `ha_base_url`: e.g. `http://homeassistant.local:8123`
- `entity_id`: e.g. `cover.living_room_blind`
- `token`: long-lived Home Assistant token
- `allowed_ssids`: your trusted Wi-Fi SSIDs array

## Runtime wiring implemented
- HTTP calls are executed via libcurl (platform HTTP client) with:
  - `Authorization: Bearer <token>`
  - timeout from `http_timeout_ms`
  - bounded retries from `max_retries`
  - local HA base URL validation (fails closed otherwise)
- `XTEINK_HA_TOKEN` environment variable overrides token from config.
- Network context uses `DeviceSdk` abstraction and `XteinkDeviceSdk` integration seam.
- `BlindControlView` polls in a non-blocking worker thread.

## CI
GitHub Actions workflow (`.github/workflows/ci.yml`) builds, runs unit tests, and runs integration test with local mock HA server.

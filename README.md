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

## Suggested hardware/software assumptions
- Linux-based Xteink firmware with C++ app layer and a lightweight HTTP client.
- Home Assistant reachable via local IP/hostname and long-lived access token.

If your base Xteink firmware differs, adapt the integration seams in `src/main.cpp`.

## Security model
1. **Local-only execution gate**
   - Integration is enabled only when current SSID is in `allowed_ssids` and network is private RFC1918.
2. **No internet fallback**
   - No DNS over public resolvers; no external endpoints.
3. **Token handling**
   - Token is never logged.
   - Runtime redaction in all error paths.
4. **Rate limits and timeouts**
   - Conservative HTTP timeout and retries to prevent UI freeze.
5. **Fail closed**
   - If network check fails or HA is unreachable, control UI becomes read-only with explicit error state.

## Build notes
This is a reference implementation skeleton. Integrate with your Xteink SDK and HTTP/network/display APIs.

## Configuration
Copy and edit:

```bash
cp config/home_assistant.example.json config/home_assistant.json
```

Fill:
- `ha_base_url`: e.g. `http://homeassistant.local:8123`
- `entity_id`: e.g. `cover.living_room_blind`
- `token`: long-lived Home Assistant token
- `allowed_ssids`: your trusted Wi-Fi SSIDs

## Home Assistant endpoints used
- `GET /api/states/{entity_id}`
- `POST /api/services/cover/open_cover`
- `POST /api/services/cover/close_cover`

## Testing checklist
- Connect to trusted SSID => state visible and commands active.
- Connect to untrusted SSID => HA panel disabled.
- Disconnect Wi-Fi => HA panel disabled.
- Token revoked => readable error, no crash, no token leak.
- HA unavailable => timeout respected, eReader UI stays responsive.

## Future improvements
- mTLS with local reverse proxy.
- Device attestation between Xteink and HA gateway.
- Position slider (`set_cover_position`) with optimistic UI.
- E-ink refresh optimization for HA panel.

## Runtime wiring implemented in this skeleton
- HTTP calls are executed via local `curl` invocation with:
  - `Authorization: Bearer <token>`
  - timeout derived from `http_timeout_ms`
  - bounded retries from `max_retries`
  - local HA base URL validation (fails closed otherwise)
- `XTEINK_HA_TOKEN` environment variable overrides token from config.
- Network context currently comes from environment-backed SDK placeholders:
  - `XTEINK_CONNECTED`, `XTEINK_SSID`, `XTEINK_BSSID`, `XTEINK_LOCAL_IP`

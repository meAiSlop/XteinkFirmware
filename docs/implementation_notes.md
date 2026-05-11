# Implementation Notes for Xteink X4 Integration

## Preserve eReader behavior
- Run HA polling on a low-priority worker thread.
- Never block page-turn/render loop on network calls.
- Pause HA polling while reading mode is in active page-transition.

## Suggested UX
- Add "Home" tile in quick panel.
- Show:
  - connectivity badge (trusted/untrusted)
  - blind state text
  - Open/Close buttons
- Disable buttons and show reason when unavailable.

## DevOps / quality practices
- Add unit tests for:
  - SSID allowlist checks
  - private IP detection
  - service payload generation
- Add integration tests with HA test container in CI.
- Secrets scanning in CI to prevent token commits.
- Signed firmware artifacts and reproducible build metadata.

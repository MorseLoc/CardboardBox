# Phase 3B validation

Validated with native GCC C++20 on Linux; this is not a Windows build certification.

| Check | Result |
| --- | --- |
| Shared table tests, optimized build | PASS |
| Shared table tests, AddressSanitizer + UndefinedBehaviorSanitizer | PASS |
| Existing ENet room/connection tests | PASS |
| Existing movement/collision/layout tests | PASS |
| Existing physical card/controller/image tests | PASS |
| Actual GLM camera projection/ray tests | PASS |
| Strict application/graphics syntax against pinned SDL3/GLM headers | PASS |
| Actual renderer in offscreen EGL OpenGL, host/guest/private hands/inspection/menu | PASS, visually inspected |

The new network test suite includes real UDP host/guest draws, play/reveal, tap,
quiet-grab keepalives, disconnect rollback, guest process restart with a saved
token, and fresh-host token recovery. A raw UDP peer sends the same draw request
three times: the host draws one card. A new sequence draws one additional card.
Tests also check all 120 copies remain accounted for through pile and hand moves.
Hidden pile packets contain anonymous slots and remain identical after a hidden
shuffle. Opponent hands expose a count and anonymous backs, without definitions.

Interpolation test: feed 20 Hz snapshots to a simulated 60 FPS display. In the
steady part of the test, more than 140 of 149 frames advance smoothly, and each
frame advances less than 0.025 world units rather than 0.05 snapshot jumps.
Seats and teleports snap directly.

LeakSanitizer cannot scan processes in this container (ptrace/proc restriction).
The sanitizer run therefore used ASAN_OPTIONS=detect_leaks=0; address/undefined
behavior checks passed. It is not a leak-scan result.

Manual checks still required: Visual Studio Debug/Release configuration and build,
Windows SDL input/focus, performance with two windows, Windows Firewall, two
physical PCs over Tailscale, real WAN latency/jitter and interruption recovery.
Use the walkthrough in PHASE3_NETWORK.md. One active table grab at a time is an
intentional milestone limitation. Match state remains in the host's memory.

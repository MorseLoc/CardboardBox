# Phase 4 validation

All six native C++20 suites passed: ClubTests, NetworkTests, TableNetworkTests,
RoomTests, CardTests and CameraTests. Final ClubTests also passed after the
receipt UUID update and added actual host-restart persistence check.

AddressSanitizer and UndefinedBehaviorSanitizer checks passed for the new club
and changed networking/table/controller code. The unchanged catalog and system
libraries were linked without instrumentation. Leak scanning was disabled due
to the container process restriction; this is not a leak-scan result.

Verified: SQLite atomic purchases and backup, exactly-once receipts across
restart, 11/3/1 nonbasic booster composition, one-cent lands, owned-only saved
named decks, stable definition UUID storage, player separation, face validation,
pencil/fill/clear/undo/redo, bounded wire decoding, actual UDP purchases and
face sharing, custom deck setup and private opening hands, life and mutual
winner/reset confirmation, reconnect and host restart, independent object grabs,
120-card regression conservation, movement/collision and camera rays.

Strict application/graphics syntax passed against pinned SDL3/GLM headers.
Actual offscreen EGL/OpenGL rendered all four screens and the textured live orb
preview without GL errors; screens were visually inspected and a deck-footer
layout overlap was corrected. SQLite's pinned vcpkg CMake target was verified
against the port's official usage file.

Remaining: full Visual Studio/vcpkg Debug and Release build, a real Windows SDL
input loop, both physical PCs over Tailscale, WAN interruptions and performance.
This source ZIP does not include Windows executables or the actual card scans.
See README.md and PHASE4_NOTES.md for installation and the acceptance walkthrough.

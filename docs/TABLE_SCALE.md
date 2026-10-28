# Phase 1.1 — tabletop capacity correction

The user confirmed Phase 1 runs and requested a much larger table / smaller
cards before Phase 2. The table must hold a 20 x 20 arrangement of cards.
This patch changes scale and the supporting layout; it does not implement
physical card manipulation or advance to Phase 2.

## Exact dimensions

Dimensions use arbitrary world units. They do not represent meters or impose
any game rule. `src/world/TableLayout.h` is the common source of truth.

| Element | Width X | Depth Z |
| --- | --- | --- |
| One upright card | 0.28 | 0.40 |
| Center-to-center slot pitch | 0.32 | 0.46 |
| Gaps between neighboring cards | 0.04 | 0.06 |
| 20-by-20 grid footprint, including interior gaps | 6.36 | 9.14 |
| Clear felt surface | 7.20 | 10.00 |
| Outer wooden table | 7.60 | 10.40 |
| Room | 20.00 | 20.00 |

The card footprint is `(count - 1) * pitch + card dimension`, leaving 0.42
units of felt at either side and 0.43 at either end. The 400 placeholders fit
without touching each other or spilling off the felt. This target refers to
the shared tabletop, not 400 cards per player. Piles, hands, and eventual toy
placement do not introduce a software-enforced card-count cap.

Both chairs move to Z +/-6.2, the initial pawn spawns at Z +8.45, and stations
move outward. Table/camera obstructions still use the resized room geometry.
A more overhead table overview starts at distance 18.5 and pitch
1.25 radians; the wheel zooms from 5.0 to 25.0, and panning covers the larger
surface. Home restores the overview. The camera is high above the table;
there is still no ceiling mesh in this prototype.

## G capacity preview

G toggles between two decorative sample cards and exactly 400 card-shaped
placeholders. Alternating colors in five-by-five groups help count rows and
columns. The same card dimensions are used in both views. The preview can be
toggled from the room or a chair and does not alter furniture collision.

This is only a visual capacity reference. It is not a collection, deck, match,
network load test, snap-to-grid implementation, or a set of manipulable cards.
Phase 2 will reuse this scale for actual card instances. Card text will need
zoom/inspection rather than being readable across the whole 400-card overview.

## Files changed

`TableLayout.h` centralizes dimensions. `Room.h/.cpp` resizes the environment,
seats and display-card sets. `Player.h` updates the safe spawn. `Camera.h/.cpp`
updates overview, zoom and pan. `Application.cpp` binds G and updates the version.
`Renderer.cpp` draws the chosen preview and displays its HUD hint. Tests and
build/document versions are updated. No third-party dependency changes.

## Verification

Headless C++ tests retain movement, no-tunneling, wall sliding, seat ownership,
safe stand-up, and camera obstruction checks. Their walking paths now go
around the expanded table. Additional tests inspect all 400 generated shapes:

- Exactly 20 distinct X centers and 20 distinct Z centers.
- Consistent dimensions in normal and capacity views.
- Every card completely inside the felt, with positive edge clearance.
- No overlap between any pair of preview cards.
- Toggling G's state restores the two normal samples.
- Both default seat orientations project the full felt clear of the top and
  bottom HUD regions at the tested 16:10 overview aspect ratio.

The world/controller/font test executable is checked with native GCC in
Debug-like and optimized builds. Windows compilation and rendering of the
revised scene still require the user's local build/playtest. The geometric
camera-fit calculation does not replace that visual check.

## Build and check in Visual Studio

Copy this archive's root contents into the existing folder containing
`CMakeLists.txt` and `vcpkg.json`, replacing matching files. Build windows-debug,
select FemboyLover.exe, and F5. No dependency reinstall is required. Then:

1. Approach the near chair, press E, and press G.
2. Confirm twenty rows and twenty columns of small shapes on the felt.
3. Zoom and pan to the edges; Home returns to the overview.
4. Stand, walk around the larger table, and test the opposite chair.
5. Confirm stations and room boundaries still block walking appropriately.
6. Toggle G off, and repeat the checks with windows-release.

If the new scale feels right and these checks pass, resume Phase 2 using these
shared dimensions. Nothing in this update forces cards into designated slots.

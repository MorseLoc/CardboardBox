# Phase 1: a clubhouse you can walk around

**Scale revision:** the user confirmed this milestone runs well and requested
a larger table before Phase 2. Version 0.1.1 now uses a 20-by-20-card capacity
layout and expanded room. See `TABLE_SCALE.md` for current dimensions and tests;
the original dimension descriptions below document the initial Phase 1 build.

## Goal and completion gate

The working Phase 0 graphics setup now supports a local playable space. Explore
as a pawn, collide with the room and furniture, approach either chair, press E
for a table view, and leave the chair safely. Confirm this in both Windows
Debug and Release before starting physical card manipulation in Phase 2.

The user confirmed the Phase 0 checks were done before this milestone began.
Phase 1's Windows compilation and actual SDL/GL output are not verified in the
authoring environment; the native simulation tests below were run here.

## What changed

- Replaced the diagnostic cube/platform scene with a 16-by-12-unit clubhouse,
  plank floor, four walls, central felt table, two chairs, painted night-window
  geometry, lamps, and prototype station furniture.
- Added a pawn-shaped body made by revolving a profile, a spherical head,
  small arms, and a fixed simple face. The real face painter is still later.
- Added camera-relative WASD walking, Shift running, normalized diagonal motion,
  furniture/wall collision, right-drag/arrow-key room camera, and wheel zoom.
- Added both local seats, nearest available seat selection, safe return to the
  pre-seating position, and smooth camera transitions. Each seat views the
  table from its own side. E or Escape returns to room exploration.
- Added table-camera right-drag panning, bounded zoom, Home camera reset, and
  camera obstruction tests against static room/furniture boxes.
- Added a readable in-window HUD, proximity prompts, pause overlay, and automatic
  pause when the app loses keyboard focus. Escape resumes; Q quits while paused.
- Kept the tested SDL3/OpenGL startup and cleanup order, high-DPI framebuffer
  sizing, resize/minimize handling, and explicit modern OpenGL loader.
- Kept the original dependency baseline, the project name, and future design
  additions. No networking or economy dependency was introduced.

## Files and responsibilities

| File | Responsibility |
| --- | --- |
| `src/world/Geometry.h/.cpp` | Small world coordinate structs, circle/box collision, substep motion, camera segment/box intersection |
| `src/world/Room.h/.cpp` | Furniture geometry, collider footprints, camera obstacles, two local seat slots, nearby station labels |
| `src/world/Player.h/.cpp` | Pawn position/facing, walk/run movement, sit/stand state and return position |
| `src/graphics/Camera.h/.cpp` | Following/orbit view, tabletop pan/zoom, camera transitions and obstruction clipping |
| `src/graphics/BitmapFont.h/.cpp` | Built-in 5x7 font and batched HUD triangles; no external font asset |
| `src/graphics/Renderer.h/.cpp` | Box/sphere/pawn meshes, room and pawn rendering, screen-space HUD |
| `src/core/Application.h/.cpp` | SDL events, input mapping, pause/focus handling, frame timing, simulation/render coordination |
| `tests/RoomTests.cpp` | Real movement, collision, seating, camera-ray, and HUD-geometry tests |
| `CMakeLists.txt` | World static library, game executable, headless test executable and CTest registration |

`src/main.cpp` and `src/graphics/GlApi.h/.cpp` are unchanged from Phase 0.
The presets and dependency names/baseline are unchanged; project/manifest
version is now 0.1.0. The renderer changes are specific to this milestone.

## Concepts worth understanding

**World state and rendering are separate.** `Player` stores where the pawn is
and whether it is seated. It does not draw itself or read the keyboard. The
application translates keyboard input into movement intent. The renderer reads
world state and draws it. This separation lets us test gameplay without a
window and later submit movement through an authoritative network session.
It does not mean networking already exists.

**Camera-relative movement** rotates WASD's intended direction by the room
camera yaw. W goes away from the camera along the floor, D moves to its right.
We normalize the two-dimensional input direction so W+D travels at the same
speed as W. Speed multiplied by delta time gives displacement for the frame.

**Collision uses a circle on the floor.** The pawn has a radius; each solid
piece of furniture has an axis-aligned rectangular footprint. The nearest
point on each rectangle tells us whether the circle overlaps it. Movement
is divided into small steps so running cannot skip a thin obstacle. We try
X and Z separately, allowing a blocked pawn to slide along a wall. This is a
simple walking controller, not a general physics engine or thrown-card system.

**Seats are a small state transition.** A free nearby chair is claimed, the
pawn's last safe standing position is stored, and movement stops. Standing
releases the claim and restores that position. There is only one visible local
player in this milestone; the tests use a second Player object to check local
seat exclusivity. Multiplayer arbitration and disconnection leases are later.

**Camera transitions interpolate positions, not matrices.** Each frame blends
current eye/target positions toward the desired view with an exponential
factor based on delta time. A segment/box test shortens the camera boom if
it would cross furniture or a wall. The resulting view matrix comes from
`lookAt`; perspective projection accounts for the drawable's aspect ratio.

**Reusable meshes save duplication.** One box, one low-poly sphere, and one
revolved pawn body are uploaded to the GPU. Object position, scale, and rotation
reuse those meshes. The shader retains simple stylized face shading; the lamps
are decorative colored shapes and do not cast real light or shadows.

**The HUD is batched geometry.** Each lit pixel in an original small font becomes
a quad. We upload batches of triangles and render them with an orthographic
projection after the 3D scene, with depth testing temporarily disabled. This
provides clear prompts without adding a UI library before we need one.

## Verification already performed

The actual world/controller/font source was compiled with GCC in C++20 mode:

- Debug-like build: `-Wall -Wextra -Wpedantic -Werror`; all tests passed.
- Optimized build: `-O2 -DNDEBUG` with the same warnings; all tests passed.
- AddressSanitizer + UndefinedBehaviorSanitizer: all tests passed with leak
  detection disabled because this container's process restrictions prevent
  LeakSanitizer from inspecting threads. This is not a completed leak check.

Coverage includes thin-wall no-tunneling, sliding, boundary containment,
rounded corners, camera segment hits/misses/padding, clear spawn, diagonal
speed, running, camera-relative movement, distant-seat rejection, both chairs,
occupied-seat rejection, seated movement suppression, safe standing, invalid
seat IDs, 15,000 reproducible random movement steps, and HUD quad validity.
These are headless tests, not a Windows graphics or application test.

## Windows completion checklist

Run each item in Debug and Release:

- [ ] The room, pawn, table, two chairs, stations, and readable HUD appear.
- [ ] WASD moves relative to the camera and Shift increases walking speed.
- [ ] Diagonal movement does not appear faster than straight movement.
- [ ] Walls, table, chairs, shop counter, desk, chest, and vanity block walking.
- [ ] Moving diagonally into furniture slides along it without falling through.
- [ ] Right-drag and arrows orbit; wheel zooms; Home resets the camera.
- [ ] Near walls, the camera stays inside rather than looking through them.
- [ ] Approach the near chair until PRESS E TO SIT appears; E seats the pawn.
- [ ] The camera smoothly changes to a table view; WASD no longer moves the pawn.
- [ ] Table right-drag pans, wheel zooms, and Home resets the table view.
- [ ] E or Escape stands without trapping the pawn in the chair.
- [ ] Walk around the table and repeat with the opposite chair/view direction.
- [ ] Escape in the room pauses; Escape resumes; Q while paused exits.
- [ ] Alt-tab pauses; returning and pressing Escape resumes correctly.
- [ ] Resize, minimize, restore, exit, and restart work.
- [ ] `RoomTests.exe` reports PASS in each build.

## Known limits and next step

One local pawn, static furniture, primitive geometry, fixed placeholder face,
simple face shading, and no ceiling mesh. Small camera/geometry intersections
may still need tuning after Windows playtesting. There is no jump, crouch,
animated walking, audio, save data, room-editing UI, shop purchases, deck builder,
face editor, networking, or movable cards. Sample cards are decorative.

Shop/land boxes, deck desk, chest, and vanity deliberately show COMING LATER
labels when nearby. The full title-screen gag and Paint-style creative tools
remain recorded design work.

Once this checklist passes, Phase 2 adds a real local card table: card data,
selection by ray casting, dragging, tapping, rotating, flipping, piles, drawing,
shuffling, and enlarged inspection. Phase 3 introduces the Tailscale/ENet
host/client connection and proper filtered private information.

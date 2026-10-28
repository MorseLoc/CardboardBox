# FemboyLover.exe — Phase 1.1: room for 20 x 20 cards

Complete C++20 source for the next native Windows milestone. Walk around a
small 3D clubhouse as a pawn, collide with furniture, and sit at either chair
for a tabletop camera. Includes a HUD and pause screen. No networking or actual
card manipulation exists yet.

## Upgrade your working Phase 1 project

You have already built and run Phase 1. This update fixes table/card scale
before Phase 2 begins. The felt fits **20 columns x 20 rows of upright cards
with gaps**. Press **G** to toggle a 400-shape capacity preview. The room,
chairs, spawn and tabletop camera are resized to match. Keep your working
Visual Studio/vcpkg setup; there are **no new third-party dependencies**.

1. Close the running game. Make a backup copy of your working project folder.
2. Open the downloaded ZIP. This archive has `CMakeLists.txt`, `src`, `docs`,
   and the other project files directly at its root, with **no outer wrapper
   folder**.
3. Copy all ZIP contents into your existing project root, allowing replacement
   of matching source/configuration/document files. From your screenshots,
   that root is:

   ```text
   C:\Users\flew\source\repos\FemboyLover\FemboyLover
   ```

   This is the folder containing `vcpkg.json`, `src`, and `CMakeLists.txt`.
   Do not paste them into `src`, `out`, or the outer repository folder.
   Existing `out`, `.vs`, and `vcpkg_installed` directories are not included in
   the archive and do not need to be deleted. Extra Visual Studio template
   files such as `FemboyLover.cpp/.h` are not used by our CMake target; leave
   them alone for this upgrade.
4. In Visual Studio, keep/open that exact folder using **File > Open > Folder**.
5. Select **windows-debug** (it may display as **Windows x64 Debug**). Allow
   CMake to reconfigure after the changed `CMakeLists.txt`. If it does not
   automatically do so, right-click `CMakeLists.txt` and choose Configure or
   Generate Cache, depending on your Visual Studio version.
6. Choose **Build > Build All**. Select **FemboyLover.exe** as the startup item
   (not RoomTests.exe), then press **F5**.
7. Repeat with **windows-release**. Test both before advancing to Phase 2.

The archive contains complete revised files; you do not need to paste code
snippets into individual files.

## Expected scene and controls

You spawn behind the near chair, looking toward a green felt card table. A
small pink/purple pawn has a spherical head and a fixed face. Around the room
are wooden walls/furniture, a night-window decoration, lamps, a back-left card
shop and five land boxes, a back-right deck desk, a left-side toy chest, and a
right-side vanity. Their shop/editor functions are future work. The two cards
on the table are still decorative shapes. They are now 0.28 x 0.40 world units
on felt measuring 7.2 x 10.0 units. G swaps them for exactly 20 rows and 20
columns of the same-sized shapes; press G again to hide that preview.

| Input | Room exploration | Seated table view |
| --- | --- | --- |
| WASD | Walk relative to camera | No movement |
| Shift + WASD | Run | No movement |
| Right mouse drag | Orbit camera | Pan across table |
| Arrow keys | Orbit camera | Unused |
| Mouse wheel | Zoom | Zoom |
| Home | Reset camera | Reset table view |
| G | Toggle 400-slot size preview | Toggle 400-slot size preview |
| E | Sit when near a free chair | Stand |
| Escape | Pause / resume | Stand; resume first if paused |
| Q while paused | Quit | Quit |
| Window close | Quit | Quit |

Walk toward a chair until **PRESS E TO SIT** appears. E claims the nearest
available seat and smoothly changes the camera. E/Escape restores your last
standing position. Walk around the table to try the opposite chair.

The game automatically pauses when it loses keyboard focus. On returning,
press Escape to resume. The Phase 0 Space-to-pause-cube control is retired:
there is no diagnostic cube in the clubhouse.

## Run the logic tests

Build All also builds `RoomTests.exe`, a small separate console program.
From Visual Studio's Developer PowerShell at the project root:

```powershell
& .\out\build\windows-debug\RoomTests.exe
& .\out\build\windows-release\RoomTests.exe
```

Run the Release command after building the Release preset. A successful test
run prints PASS. Tests remain enabled in Release; they do not rely on assert.
You can also use CTest:

```powershell
ctest --test-dir out/build/windows-debug --output-on-failure
ctest --test-dir out/build/windows-release --output-on-failure
```

These test real movement/collision/seating code without opening a graphics
window. They do not verify SDL/OpenGL rendering or multiplayer. Detailed tests,
C++ explanations, changed-file responsibilities, and the manual checklist are
in **docs/PHASE1_NOTES.md**.

## Quick manual completion check

In both Debug and Release: press G and inspect the full 20 x 20 preview from
both chairs, test table pan/zoom and Home reset, then walk/run, orbit/zoom,
collide with every major piece
of furniture and all four walls, sit/stand at both chairs, pan the tabletop,
pause/resume, alt-tab, resize/minimize/restore, close, and relaunch. Run the
headless test executable too. If all work, Phase 1 is complete.

**Verification here:** the world/controller/font code passed native GCC
Debug-like and optimized tests, including the new 400-card layout and camera
fit checks. Source/configuration
and archive checks were performed. The complete Phase 1 Windows application
has not been compiled or visually tested in this authoring environment. Your
Windows build and manual checklist are the remaining gate.

The scale-specific calculations, changes, and test checklist are in
**docs/TABLE_SCALE.md**. These supersede the original small-table dimensions
in the Phase 1 historical notes.

## Important files

| File | Purpose |
| --- | --- |
| `CMakeLists.txt` | Builds the world library, game, and tests |
| `CMakePresets.json` | Existing x64 Windows Debug/Release presets |
| `vcpkg.json` | Same pinned SDL3/GLM dependencies, project version 0.1.1 |
| `src/core/Application.h/.cpp` | Window lifetime, input, timing, pause, main loop |
| `src/world/TableLayout.h` | Shared card, capacity, table, room, seating, and overview dimensions |
| `src/world/Geometry.h/.cpp` | Floor-plane collision and camera intersection math |
| `src/world/Room.h/.cpp` | Room objects, static colliders, chairs, station proximity |
| `src/world/Player.h/.cpp` | Walking/facing, local seat ownership, safe stand-up |
| `src/graphics/Camera.h/.cpp` | Room and table camera with smooth transitions |
| `src/graphics/Renderer.h/.cpp` | Primitive meshes, room/pawn rendering, HUD |
| `src/graphics/BitmapFont.h/.cpp` | Batched built-in text geometry |
| `src/graphics/GlApi.h/.cpp` | Unchanged modern OpenGL entry-point loader |
| `src/main.cpp` | Unchanged entry point and exception reporting |
| `tests/RoomTests.cpp` | Headless simulation and HUD-geometry checks |
| `docs/PHASE1_NOTES.md` | Milestone explanations, tests, limits, next steps |
| `docs/DESIGN_BASELINE.md` | Supplied GDD requirements record and amendments |
| `docs/CREATIVE_TOOLS.md` | Future shape builder and paintable world |
| `docs/TITLE_DIRECTION.md` | Future title-screen gag |

The world simulation uses simple C++ types and no SDL/GLM dependency; graphics
uses GLM for matrices. `ClubhouseWorld` is an internal static library, not an
extra DLL to distribute. No empty placeholder classes have been added for later
systems.

## Fresh Windows setup, if needed

Use Visual Studio 2022 17.8+ or a newer compatible Visual Studio version with
Desktop development with C++, an x64 MSVC toolchain, Windows SDK, C++ CMake
tools, and Ninja. CMake 3.25+ and an OpenGL 3.3-capable GPU driver are required.
Install Git for Windows. Open the folder containing this README and the CMake
manifest directly rather than creating a new Visual Studio template project.

In PowerShell, use a dedicated vcpkg checkout (skip if already set up):

```powershell
New-Item -ItemType Directory -Force C:\dev | Out-Null
git clone --branch 2025.09.17 https://github.com/microsoft/vcpkg.git C:\dev\vcpkg-femboylover
git -C C:\dev\vcpkg-femboylover checkout --detach 4334d8b4c8916018600212ab4dd4bbdc343065d1
& C:\dev\vcpkg-femboylover\bootstrap-vcpkg.bat -disableMetrics
if ($LASTEXITCODE -ne 0) { throw 'vcpkg bootstrap failed' }
$env:VCPKG_ROOT = 'C:\dev\vcpkg-femboylover'
[Environment]::SetEnvironmentVariable('VCPKG_ROOT', $env:VCPKG_ROOT, 'User')
```

Restart Visual Studio after setting the environment variable. The presets
explicitly use `$env{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake`; a global
`vcpkg integrate install` is not needed. First configuration installs SDL3
(3.2.22 at this baseline) and GLM automatically and needs internet access.
OpenGL links through the platform library and uses the installed GPU driver.

Command-line alternative, in an x64 Visual Studio developer shell at the root:

```powershell
cmake --preset windows-debug
cmake --build --preset windows-debug
& .\out\build\windows-debug\FemboyLover.exe
cmake --preset windows-release
cmake --build --preset windows-release
& .\out\build\windows-release\FemboyLover.exe
```

Ninja is a single-configuration generator. Select the right preset rather than
using `--config Release` on a Debug build. SDL3.dll is copied beside the game
executable by CMake. This remains development source, not the final installer;
separate-PC runtime packaging comes later.

## Troubleshooting

- **Only generic x64 Debug/Release options:** confirm the opened folder is the
  one with our manifest and presets, not the outer repository/template folder.
- **SDL3 not found:** confirm the correct preset and `VCPKG_ROOT`; do not copy
  DLLs into random folders or substitute SDL2 libraries.
- **Source file missing:** the `src/world` files must be copied too; this update
  adds files as well as replacing existing ones.
- **Wrong program starts:** choose FemboyLover.exe, not RoomTests.exe, beside
  the green Run button.
- **No movement:** click the game window; if PAUSED appears, press Escape.
  WASD intentionally does not move a seated pawn.
- **Cannot sit:** walk closer to a chair until the prompt appears. E at a shop
  or another station does not open a menu yet.
- **Compile/configure error:** send the first error plus nearby Output lines,
  your selected preset, and a screenshot of the project folder layout.
- **GL context/shader error:** preserve the full dialog/console message and
  check the GPU driver. Actual Windows graphics remain to be playtested.

## What is next

After this milestone passes, Phase 2 implements local physical card actions.
Phase 3 adds authoritative ENet networking over the planned Tailscale direct
IP connection. There is still no second player, saved collection, card rules
engine, shop transaction, deck builder, editable face, physics throw, or working
title menu. Future paintable toys/furniture/walls remain in the design record.

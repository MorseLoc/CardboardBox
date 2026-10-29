# FemboyLover.exe — Phase 2: physical tabletop

A local C++20 / SDL3 / OpenGL card-table prototype on top of the working
clubhouse. The table now fits **20 cards wide by 10 cards deep**, with both
chairs facing each other across the long edges. It remains a free tabletop;
this capacity guide does not enforce placement or limit the number of cards.

## Version 0.2.2: Unlimited data and real image loading

Your supplied **MTGJSON `2ED.json`** has been imported: 302 Unlimited printings,
15 basic-land artwork variants, rarity metadata, original printed-text fields,
current rules text kept separately, and the set's booster recipe/sheet data.
It is not Scryfall's JSON format, but every record has a Scryfall printing ID.
There are no image bytes or direct image URLs in the supplied file.

The test libraries now use Unlimited definitions instead of the eight invented
cards. These are temporary 60-card demo lists, not a saved collection or a
new deck builder. Both sides still have 24 basic lands and 36 other cards.
The 20-wide/10-deep table and the empty-pile fix are retained.

**Original scans are not included in this ZIP.** The game displays explicitly
labeled text fallback faces until you download the images on your Windows PC:

1. Close the game and copy the archive's root contents into your existing
   project root, replacing matching files, including `tools` and `data`.
2. Select windows-debug and let CMake reconfigure. The manifest adds **stb**, a
   small header-only image decoder installed automatically through your existing
   vcpkg setup. No Python, extra DLL, or manual image-library installation is
   required for building or playing.
3. In Visual Studio's Developer PowerShell at the project root, run:

   ```powershell
   powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\tools\Download-CardImages.ps1
   ```

4. Let the downloader finish, then Build All and F5. If you already ran the
   game before downloading, close and relaunch it to refresh cached textures.

The downloader looks up each exact Unlimited printing, retrieves its full-card
image URL, validates the returned set/ID, downloads JPEG/PNG images with pacing
and retries, and skips valid-looking files already cached. Rerun to resume.
`-Force` replaces existing cached files; `-Limit 3` is available for a small
connection test. Images are stored in your per-user cache:

```text
%APPDATA%\FemboyLover\Clubhouse\cards\2ed
```

The game uses the matching SDL preference path. Your source/installation folder
is not used for writable image caching. `download-report.json` records failures.
Missing or corrupt files keep their readable fallback rather than preventing
launch. Live image downloads and the PowerShell script could not be executed
in the authoring environment; this one-time step needs your Windows internet
connection. The game makes no image requests during normal play.

Only Unlimited was supplied. Beta artwork/printing variants are not added by
this file. See **docs/UNLIMITED_IMPORT.md** for field mapping, reproducibility,
image-cache details, and the Windows verification checklist.

## Version 0.2.1: empty-pile fix

Player-made piles now disappear when their last card is drawn, moved to a hand,
returned to a library, or merged into another pile. Empty piles no longer draw
a card back or zero label, cannot be picked/dragged, and do not support cards
as invisible stacks. A deleted pile is cleared from selection.

The two original libraries also disappear visually at zero cards. Their return
addresses remain internally so L / Shift+L can refill them at their remembered
positions. This does not retain a visible or clickable zero-card object.

To reproduce the fix: play one card, select it, B to group it, then double-click
that pile to draw its last card. The pile should vanish. Also try merging a
whole pile into another. The losing/source pile should disappear without
losing any card copies. Build instructions and dependencies are unchanged.

## Upgrade your working project

1. Close the game and keep a backup of your working project folder.
2. Copy **all contents** of this ZIP into your existing project root, replacing
   matching files. The archive has no wrapper folder. From your screenshots:

   ```text
   C:\Users\flew\source\repos\FemboyLover\FemboyLover
   ```

   `CMakeLists.txt`, `vcpkg.json`, `src`, and this README belong together. Copy
   the new `src/cards` and test files too. Keep your existing build/cache folders.
3. Open that exact folder in Visual Studio. Select **windows-debug**, allow
   CMake to configure, then **Build > Build All**.
4. Select **FemboyLover.exe** as the startup item and press **F5**. Do not select
   RoomTests, CardTests, or CameraTests for playing the game.
5. Repeat with **windows-release** once Debug works.

**One new header-only dependency (`stb`), installed automatically.** Keep your existing vcpkg installation,
`VCPKG_ROOT`, SDL3/GLM baseline, and presets. This is complete source, not a
precompiled Windows executable. Output remains under `out/build/windows-debug`
or `out/build/windows-release`, with SDL3.dll copied beside the game.

## First tabletop session

1. Walk toward the near chair and press **E** when prompted.
2. There are two card-back piles on the table, initially labeled **60**. They
   are temporary Unlimited demo decks: 24 basic lands and 36 other cards per deck.
3. Press **Shift+D** to draw seven from your own library. Your hand appears
   along the bottom. Or double-click your near-side library to draw one.
4. Drag a hand thumbnail onto the felt. It becomes a public face-up card.
5. Click it, press T to tap, F to flip, or R / Shift+R to rotate. Right-click a
   face-up card for an enlarged readable inspection; click or Escape closes it.
6. Draw and place another card. Ctrl-click cards to multi-select, drag them as
   a group, or press B to collect selected tabletop cards into an ordered pile.
7. Right-click a pile for its context menu. Try shuffle, split, draw, and public
   pile inspection. Shift-drag a pile onto another pile to merge them.
8. Press E to stand, walk around the table, and sit at the other chair to test
   the opposite side. Each side retains its own hand and library during this run.

This is **one local client testing two sides**, not two-player multiplayer.
You can switch seats to act as either side. An opponent's hand is represented
only by its count in the current seat's HUD; it is not drawn in the room.
Restarting the app resets these temporary test decks and all table state.

## Controls

Room controls remain WASD, Shift to run, right-drag/arrow keys to orbit, wheel
to zoom, Home to reset, E to sit, Escape to pause/resume, and Q while paused to
quit. Losing keyboard focus pauses and cancels an unfinished card drag safely.

While seated, WASD movement is disabled; D and S become card shortcuts:

| Input | Table action |
| --- | --- |
| Left click | Select a card, hand thumbnail, or pile |
| Ctrl+click | Add/remove a card from a multi-selection |
| Left drag | Move selected cards or the selected pile |
| Drag hand thumbnail onto felt | Play it face up |
| Drag a loose selection onto hand bar / H | Put cards into your hand |
| Right click face-up card or own hand card | Enlarged inspection |
| Shift+right click card / right click pile | Context menu |
| Click / Escape during inspection | Close inspection |
| Wheel during public-pile inspection | Browse that pile in order |
| Double-click a permitted pile / D | Draw one to your hand |
| Shift+D | Draw seven |
| S | Shuffle selected pile, or own library if none selected |
| T | Tap/untap selected cards or pile by 90 degrees |
| F | Flip selected cards; reveal/hide a selected pile |
| R / Shift+R | Rotate selected cards or pile by +/-15 degrees |
| B | Group selected tabletop cards into an ordered pile |
| X | Split the selected pile's top half into a new pile |
| Shift+drop onto another pile | Merge the dragged cards/pile into it |
| L / Shift+L | Return selected cards to own library top / bottom |
| C | Bring the selected cards/pile back to the table center |
| Middle drag / right drag on empty felt | Pan tabletop camera |
| Wheel over table | Zoom camera |
| Wheel over hand / bracket buttons | Scroll a large hand |
| Home | Reset tabletop view |
| G | Toggle the 20-by-10 capacity preview |
| Escape while dragging | Cancel and restore the pre-drag state |
| E | Stand; first place or cancel any held card |

Right drag over a card opens inspection/context rather than panning. Middle
mouse drag is the reliable pan control when cards cover the felt. Unavailable
context-menu options appear dimmed. Keyboard draw/shuffle shortcuts use your
library when no pile is selected. Select a different pile to act on it.

The G preview swaps visible game cards for 200 inert reference shapes without
resetting the underlying table. Press G again to return to the real test decks.
There is no forced snap grid. Top/bottom return and merge use selection/pile
order; pile storage is bottom-to-top, and drawing removes its last element.

## What's implemented

- Separate reusable card definitions and uniquely identified session copies.
- Exact-printing image-cache loading, labeled metadata fallback fronts, and an original shared back.
- Actual camera-ray intersections with rotated card/pile rectangles.
- Selection, multi-selection, dragging, lift/shadow feedback, surface stacking,
  tapping, free rotation increments, face flipping, and enlarged inspection.
- Ordered piles: moving, grouping, shuffling, drawing, splitting, merging,
  top/bottom returns, public viewing, and preserving empty library anchors.
- Separate local hands, scrolling, hand-to-table play, and local visibility
  filtering for hand faces, hidden piles, and face-down cards.
- Safe drag cancellation on Escape, focus loss, or entering the capacity preview.
- Retained room walking, collision, both chairs, camera transitions, HUD/pause.

No printed ability, mana requirement, turn, combat, damage, or winner is
interpreted or resolved. A card saying "draw two" never draws cards by itself.
Use the physical actions to carry out what the players agree to do.

## Files and concepts

| File | Responsibility |
| --- | --- |
| `src/cards/Tabletop.h/.cpp` | Session card/pile data, ordering, movement, visibility, draw/shuffle/split/merge operations and invariants |
| `src/cards/TabletopController.h/.cpp` | Selection, mouse gestures, hand/context-menu hit tests, inspection and keyboard actions |
| `src/cards/CardArt.h/.cpp` | Readable fallback fronts and an original shared back |
| `src/cards/CardCatalog.h/.cpp` and `UnlimitedCards.inc` | Imported 302-printing catalog and demo-list construction |
| `src/cards/CardImages.h/.cpp` | Validated JPEG/PNG decoding and small texture creation |
| `tools/Download-CardImages.ps1` | One-time exact-printing image downloader for Windows |
| `tools/import_mtgjson.py` | Optional reproducible source-data converter; not needed for builds |
| `src/graphics/Camera.h/.cpp` | Existing cameras plus screen-to-world rays |
| `src/graphics/Renderer.h/.cpp` | 3D cards, texture upload, highlights, hand thumbnails, menus and inspection |
| `src/graphics/GlApi.h/.cpp` | Adds typed texture-unit/uniform entry points |
| `src/graphics/BitmapFont.h/.cpp` | Existing built-in font; vertex format now also carries UV coordinates |
| `src/core/Application.h/.cpp` | Routes SDL3 events and high-DPI mouse coordinates to the tabletop controller |
| `src/world/TableLayout.h` | Shared 20-wide/10-deep dimensions, card thickness, seats, spawn and overview |
| `tests/CardTests.cpp` | Actual card-state/controller/art checks |
| `tests/CameraTests.cpp` | Actual GLM projection/ray checks for both seats and different aspect ratios |
| `tests/RoomTests.cpp` | Retained movement/seat checks and updated 200-slot layout checks |
| `docs/PHASE2_NOTES.md` | Implementation explanations, verification detail and completion checklist |
| `docs/DESIGN_BASELINE.md` | Authoritative supplied-design requirements record and amendments |

A definition describes what a card is. An instance describes where one physical
copy is and which zone/pile contains it. Drawing moves an existing ID; it does
not create another copy. Ownership is unchanged by ordinary moves. These are
session identities, not yet saved collectible ownership records.

Rendering and input are separate from the card model. That lets tests verify
card counts and pile order without a window, and lets Phase 3 introduce host
requests and filtered replication without putting card rules in the renderer.
The controller keeps a pre-drag snapshot so cancellation restores the actual
old zone and position rather than leaving a card in two places.

## Verification and your Windows completion gate

Build All produces three separate test programs. After building each preset:

```powershell
& .\out\build\windows-debug\RoomTests.exe
& .\out\build\windows-debug\CardTests.exe
& .\out\build\windows-debug\CameraTests.exe
ctest --test-dir out/build/windows-debug --output-on-failure
```

Repeat with `windows-release` after building Release. Tests do not rely on
assert, so their checks remain active in Release.

Authoring checks include native C++20 compilation of the logic tests, full
application-source syntax checking against the pinned SDL3 3.2.22 / GLM 1.0.1 / stb
headers, and actual off-screen OpenGL rendering of the real renderer, shaders,
textures, HUD, inspection and room using a separate EGL harness. That harness
substitutes only the context/function-loading setup; it does not exercise the
SDL window/input loop. It is not a Windows MSVC build or a two-PC test.

On Windows, check: room movement still works; both chairs work; the table is
20 wide/10 deep; G shows 200 references; drawing changes counts correctly;
hand play, tap/untap, rotation, flip and inspection work; hidden fronts cannot
be inspected; multi-drag and Escape cancellation work; grouping, split, merge,
shuffle, returns and public-pile browsing preserve all copies; large hands
scroll; resizing and focus loss do not strand a card; and both presets run.

## Limits and next milestone

This remains local and temporary. No ENet/Tailscale connection, match/result
flow, life counters, saved collection/decks, shop/economy, ante, face painter,
toy chest actions, throws, sounds, or finished title screen exist yet. The shop
and other station furniture remain placeholders. Current keyboard bindings are
defaults, with configurable bindings/settings still to come.

Face filtering here is a local presentation policy. Both sides' full test state
exists in this process, and seat switching intentionally exposes the chosen
side for testing. **It is not the required network security boundary.** Phase 3
must keep hidden identities out of unauthorized client payloads altogether.

Cards are thin kinematic tabletop objects. Surface stacking is approximate;
piles are ordered containers. Cards may be dragged outside the felt, retaining
the tabletop working-plane height; they do not fall or throw yet. C recalls a
selected off-table object. There is no rigid-body card physics in this phase.

The Unlimited definitions come from your supplied file. Image downloads remain
a separate per-PC setup step, and the shared card back remains original. Beta
printings are not imported yet. No spell text is automatically interpreted.

After both Windows builds pass the checklist, Phase 3 adds the private
host/client connection, authoritative ordering, simultaneous-grab handling,
filtered card information, movement synchronization, and reconnection.

## Fresh setup

For a new development PC, use Visual Studio with Desktop development with C++,
x64 MSVC, Windows SDK, C++ CMake tools/Ninja, CMake 3.25+, Git, and an OpenGL
3.3-capable graphics driver. Use a dedicated vcpkg checkout:

```powershell
git clone --branch 2025.09.17 https://github.com/microsoft/vcpkg.git C:\dev\vcpkg-femboylover
git -C C:\dev\vcpkg-femboylover checkout --detach 4334d8b4c8916018600212ab4dd4bbdc343065d1
& C:\dev\vcpkg-femboylover\bootstrap-vcpkg.bat -disableMetrics
$env:VCPKG_ROOT = 'C:\dev\vcpkg-femboylover'
[Environment]::SetEnvironmentVariable('VCPKG_ROOT', $env:VCPKG_ROOT, 'User')
```

Restart Visual Studio after setting the environment variable, then open the
folder containing the manifest/presets directly. The existing preset toolchain
installs the pinned SDL3 and GLM dependencies automatically. You do not need to
create a new Visual Studio template project or run global vcpkg integration.

# Phase 3A: the first real connection

This milestone starts Phase 3; it does not complete networked card play.
It is based on MorseLoc/CardboardBox master commit
`14784fbbe9c9e17c3e93c3d979d7748821754cc8` (Added Unlimited Cards).
The source files match that commit, with changes listed below. The unused
Visual Studio starter files at the repository root are retained.

## What to verify before adding card replication

Two running copies must connect, display pink host and blue guest pawns,
replicate movement, respect furniture and room collisions, occupy their
assigned opposite seats, and reconnect without resetting the guest pawn.
A third player and incompatible builds/content must be refused.

**Network mode synchronizes the room and seating only.** It renders an empty
card table and disables card input. Choose L on the main menu to use every
existing local tabletop control and your downloaded Unlimited art. A network
room must never show independently simulated decks as if they were shared.

## Build in Visual Studio

1. Close the game. Back up your working folder or commit it first.
2. Copy the complete update into the same project root as CMakeLists.txt.
   Keep your existing VCPKG_ROOT and presets. The package has no wrapper folder.
3. Open that folder in Visual Studio, select windows-debug, and let CMake
   reconfigure. vcpkg now installs ENet 1.3.18 in addition to SDL3, GLM and stb.
   This uses the same pinned baseline. No new account, service or editor is needed.
4. Build > Build All. Select FemboyLover.exe, then F5.
5. Repeat the build and tests with windows-release once Debug works.

ENet uses the `unofficial::enet::enet` imported CMake target. Its vcpkg port
builds a static library; this change does not add an ENet DLL to distribute.
Windows system socket/timer libraries ws2_32 and winmm are linked explicitly.
SDL3.dll is still copied beside the executable. These are source files, not
a precompiled Windows release.

Run the checks through Test Explorer or Developer PowerShell:

```powershell
ctest --test-dir .\out\build\windows-debug --output-on-failure
ctest --test-dir .\out\build\windows-release --output-on-failure
```

## First: two windows on one PC

Launch the first copy, press H to host. Then open a second copy, for example:

```powershell
Start-Process .\out\build\windows-debug\FemboyLover.exe
```

In the second window, press J, use Ctrl+A to clear the address, type
`127.0.0.1`, then Enter. Ctrl+V pastes an IPv4 address or hostname, replacing
the field. Escape backs out of address entry. UDP port 33445 is fixed in this
milestone; do not append `:33445` to the address.

Both windows should show two pawns and a connected status. Pink is host/player
one; blue is guest/player two. They spawn on opposite sides of the room.
WASD moves; Shift runs; right-drag changes the camera. Movement is calculated
by the host. The other pawn is smoothed for display. The local guest pawn uses
host snapshots directly, so a small input delay is expected.

Walk up to your own chair and E to sit. Each network player is assigned their
own seat, pink on the positive-Z side and blue on the negative-Z side. E or
Escape stands. Sitting does not start a card match in Phase 3A.

Switching windows pauses local input; press Escape to resume. The host keeps
servicing the guest even when the host is paused or minimized. F2 leaves the
room and returns to the main menu. Q quits from that menu or while paused.
L starts the unchanged local card-table mode, with your existing cached art.

## Then: two Windows PCs

Both machines need this same build and card definitions. On your existing
private Tailscale network, the host presses H. The guest presses J and enters
the host PC's Tailscale IPv4 address. The game uses ordinary ENet UDP sockets;
Tailscale provides the network route without a special game-side integration.
A LAN IPv4 address also works if both PCs are on the same LAN. There is no
room-code, discovery server, NAT traversal service, or relay built into the game.

If Windows Firewall asks, permit the executable on the network profile used
for this private connection. A blocked UDP 33445 rule, an unreachable address,
or a Tailscale access policy can prevent joining. Run the same-PC test first
to separate application problems from routing/firewall problems. No router
port forwarding is required for the intended Tailscale connection.

Each PC loads artwork from its own existing preference cache. Room networking
does not download, copy, or transmit image files. Your current caching job can
finish normally. The downloader default-manifest-path bug is also fixed here;
its earlier explicit-path command remains valid.

## Reconnection and boundaries

A temporary disconnection freezes the guest's input and triggers automatic
retries. F5 on the guest requests a reconnect using the same in-memory resume
token. The host retains pawn position and seat while the room remains open.
The disconnected seat stays reserved for that guest. No match result or reward
is inferred from disconnection.

A guest process restart loses its in-memory token. The host must use F2 and H
to create a fresh room before a restarted guest can join. Closing the host also
ends the room. This build has no persistent room save, collection database,
accounts or economy. Never treat this room token as a permanent player ID.

ENet itself does not encrypt packets. This private prototype relies on the
chosen private network and an honest host. Resume tokens prevent accidental
slot takeover; they are not an Internet authentication system.

## Files and concepts

- `src/networking/RoomProtocol.h/.cpp`: bounded binary messages, explicit little
  endian serialization, finite numeric checks, protocol/build versions, and a
  stable fingerprint of all public card-definition data. PNG/JPEG cache files
  are excluded from the fingerprint so a missing image does not block joining.
- `src/networking/NetworkRoom.h/.cpp`: RAII ownership of ENet resources; a
  nonblocking event pump; host-side 60 Hz simulation; 20 Hz movement/snapshot
  messages; reliable handshakes/seating; session token and reconnect handling.
- `src/world/Player.h/.cpp`: explicit seat choice with distance and occupancy
  validation; application of an already validated host state for rendering.
- `src/ui/ConnectionView.h`: simple presentation data for the menu and status.
- `Application`: input/menu integration; networking continues while paused;
  guest sends directional inputs instead of assigning its world position.
- `Renderer`: second pawn, distinguishing body colors, menu/status rendering,
  and an explicit room-only network view.
- `tests/NetworkTests.cpp`: strict decoding plus actual ENet UDP loopback.

The reliable channel carries handshake and seat requests. The separate
unreliable sequenced channel carries replaceable movement and snapshots, so
outdated motion does not accumulate behind reliable retransmissions. Stale
guest input expires after 0.3 seconds. Packet sizes and waiting data are bounded;
packet structs are never serialized by copying their compiler-dependent memory.

The host maintains Room and Player simulation state independently of graphics.
Rendering interpolates the remote pawn, not the collision simulation. Its
authoritative positions include room/furniture collision and pawn separation.

## Validation and limits

Native tests exercise real ENet 1.3.18 UDP sockets, handshake rejection for
wrong build and card content, player assignment, host-applied guest movement,
wall collision, an occupied room, reserved-slot reconnect, seat-distance checks,
and guest stand requests. Room regression tests cover the existing collision
and seating behavior. Strict native compiler checks cover changed application
and rendering code. An offscreen OpenGL render checks the menu and second pawn.

The authoring environment is not Windows: Visual Studio/vcpkg Debug and Release,
Windows firewall behavior, two physical PCs, WAN latency, VPN interruptions and
suspend/resume still require testing on your machines. A live Windows SDL window
and input loop cannot be certified by the headless rendering test.

## Next: Phase 3B

Once these connection checks pass, add host-authoritative card requests,
per-card manipulation leases, public/owner-specific snapshots, and packet-level
privacy tests. A guest must start from a filtered view, never from a complete
authoritative Tabletop disguised with card-back textures. Integrate draw,
shuffle, drag, rotate, tap, flip and pile operations through that boundary.
Collections, rewards and ante remain later milestones.

# Phase 3B: shared cards and smooth pawns

This extends the user's working Phase 3A source, originally based on
MorseLoc/CardboardBox commit `14784fbbe9c9e17c3e93c3d979d7748821754cc8`.
The GitHub head was checked before this update; the local Phase 3A package is
newer than that head and its room/connection features are preserved here.
No packaging work for the second PC or later game phases is included.

## Build and run

Copy all archive contents over the working project root (beside CMakeLists.txt),
keeping your VCPKG_ROOT environment setting. No extra dependency is introduced.
Select windows-debug, wait for configuration, Build All, and launch
FemboyLover.exe. Repeat with windows-release. This is a source update, not a
precompiled Windows executable. Both players must use Phase 3B: its wire version
and build number intentionally reject 3A peers.

Two windows on one PC: H in the first; J, Ctrl+A, `127.0.0.1`, Enter in the
second. You can launch the second copy from Developer PowerShell:

```powershell
Start-Process .\out\build\windows-debug\FemboyLover.exe
```

Two PCs: use the host's Tailscale IPv4 address instead. Both PCs must be on the
same permitted private network. UDP port 33445 is unchanged; do not append a
port to the address field. Permit the game in the relevant Windows Firewall
profile if prompted. There is no public matchmaking or built-in relay.

Switching windows pauses the window that loses focus; Escape resumes it.
The host continues networking while paused or minimized. F2 leaves the room;
Q exits from the menu or pause screen. L on the menu starts local tabletop mode.

## Completion checklist: run on both Debug and Release

1. Connect and walk both pawns. Look at the guest's own movement and the other
   pawn in both windows. They should animate between updates rather than jump
   at the 20 Hz snapshot rate. Some input delay remains on the guest.
2. Approach your assigned chair and E to sit: pink host on one long edge, blue
   guest on the other. Attempting to sit from far away must fail.
3. Shift+D in each window. Your hand should have seven readable cards, the other
   hand only a count. Opponent hand contents are absent from the incoming view.
4. Drag one hand card onto the felt. Both windows must show it in the same
   position; the playing hand count drops by one. Try both players in turn.
5. Select it and test T twice, F twice, R, and Shift+R. Right-click a face-up
   card to inspect it. A face-down card must not reveal its front by inspection.
6. Play another card, Ctrl-click both, then B. A public pile should appear.
   Right-click or Shift-right-click to inspect/browse or open the actions menu.
   X splits a multi-card pile. Select and S to shuffle. Shift-drop a pile onto
   another to merge. Drawing the last card deletes a player-made pile.
7. H moves selected loose cards into your hand; L returns them to your library
   top, Shift+L to its bottom. Double-click your library to draw one. A hidden
   opponent pile cannot be drawn/revealed/shuffled/manipulated by your client.
8. Have one player hold a card. A second player's conflicting operation must
   receive TABLE BUSY. Release before the second player retries. This build
   serializes all grabs: independent simultaneous drags are not implemented.
9. Cancel a drag with Escape/right-click, change focus, or disconnect the guest
   while dragging. The card must return to its original hand/position without
   duplication. A stationary held grab stays alive via heartbeat packets.
10. Press F5 in the guest: its hand, piles, pawn and seat should recover. Close
    only the guest, reopen it, and join the same address. It should resume the
    reserved seat/hand and still accept new card commands. Keep the host open.
11. Try a third player or a 3A build. The existing guest must stay connected.
12. Repeat on the two real Windows PCs, including Tailscale disconnection/recovery.

Both PCs load scans from their own `%APPDATA%\FemboyLover\Clubhouse\cards`
cache. No artwork crosses the game connection. Your completed download does not
need to be repeated on this PC. Missing artwork keeps the readable fallback.

## Files and responsibilities

- `TableCommand.h`: explicit card requests without definitions or client state.
- `TableSession`: the host's authoritative Tabletop, permission checks, one
  manipulation lease, bounds checks, cancellation, and lease expiry.
- `TableProtocol`: bounded, explicit little-endian command/view serialization;
  finite-float checks and rejection of truncated or trailing payloads.
- `Tabletop::viewFor`: recipient filtering. Hidden libraries/piles contain
  anonymous placeholders, not original instance IDs or their shuffled order.
  The other player's hand also has only anonymous cards. Definitions are sent
  only for that viewer's hand or revealed public cards. Display snapshots are
  validated before replacing the client's view.
- `NetworkRoom`: reliable ordered commands, monotonic deduplication, host
  replies, reliable filtered table snapshots, 20 Hz coalesced drag movement,
  movement updates on the separate unreliable channel, and reconnection.
- `TabletopController`: existing controls route network mutations to the host;
  local mode still uses the original Tabletop methods. Inspection stays local
  and can only display identities in the filtered view.
- `PawnSmoother`: 75 ms display interpolation, including the guest's own pawn.
  Seat transitions, teleports and reconnects snap; authoritative collisions are
  unchanged. No prediction/reconciliation is attempted in this milestone.
- `Application` / `Renderer`: shared table rendering, private hand UI, busy
  feedback, and continuous network servicing while paused.

The host generates shuffles. It serializes both players' card requests, rejects
requests from unseated players, rejects private-zone access, and suppresses
repeated command sequence numbers. The current board has 120 physical copies
in two 60-card demo libraries. Player-made empty piles disappear; original empty
library anchors remain available internally for returns.

Drag movement is sent at most 20 times a second; stationary holds renew twice a
second. A lease expires after three seconds without a renewal. Standing or
losing the guest connection cancels its active drag. A grab cancel rolls back
the original physical state; atomic completed actions are never rolled back
merely because their sender disconnects. Updated recipient views are sent only
when the table changes, at up to 20 Hz, plus the initial/reconnect snapshot.
The packet size cap is 32 KiB; snapshots stay below that cap. Commands have a
rate bound and fixed maximum selection length. Stale movement input expires.

Resume tokens are saved under `%APPDATA%\FemboyLover\Clubhouse\sessions`, keyed
by host address and port. Use the same address when reopening the guest. The
welcome includes the last processed command sequence so a restarted client
continues without replaying old draws. An old token for a newly reopened host
is cleared and retried once; it cannot claim somebody else's reserved room.
Closing the host ends this temporary session. This is not a permanent player
identity, account system, or disk save of a match.

The host necessarily knows all authoritative cards. Privacy protects the guest's
view from receiving opponent identities; it cannot hide cards from the host
process. ENet does not encrypt; the intended private Tailscale connection supplies
the private route. Do not expose this prototype as a public server.

## Automated checks and remaining platform checks

```powershell
ctest --test-dir .\out\build\windows-debug --output-on-failure
ctest --test-dir .\out\build\windows-release --output-on-failure
```

`TableNetworkTests` exercises filtered packet privacy, hidden shuffle anonymity,
round trips, malformed/truncated views, permission rejection, draw/return counts,
play/reveal/tap/flip/rotate, group/split/merge/shuffle, empty piles, grab exclusion,
lease expiry, network controller hand dragging, pawn interpolation, actual UDP
shared cards, repeated-request deduplication, reconnect rollback, guest-process
restart, and fresh-host token recovery. Existing Network/Room/Card/Camera tests
remain. Native source checks and offscreen OpenGL exercise the real renderer.

Visual Studio/vcpkg Debug and Release, a real SDL Windows input loop, Windows
Firewall, VPN interruptions, two real PCs, and WAN jitter still need your tests.
Headless tests do not certify those platform behaviors.

## Boundaries and next phase

One active tabletop drag is supported at a time. Other actions receive a busy
reply instead of being queued invisibly. Cards follow authoritative snapshots;
there is no client-side card prediction yet. UI pile selection may need a new
click after another player splits/merges/removes that pile. This is a manual
sandbox: card abilities, mana and outcomes are not interpreted automatically.

No shop, collection ownership, economy, deck builder, persistent match save,
rewards, ante, toy physics, face painter, or room painting has been added. The
next milestone is the shop/collection foundation using the Unlimited catalog,
including one-cent basic-land purchases and the recorded booster recipe.

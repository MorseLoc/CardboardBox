# Phase 4 implementation and Windows completion checklist

## Completion walkthrough

1. Build and run Debug on both PCs, with both title/HUD labels showing Phase 4.
   H hosts; J joins the host's Tailscale address. Same-PC test: 127.0.0.1.
2. F3 on each player. Verify separate 500.00 balances and Starter inventories.
   Buy one booster: balance becomes 400.00 and exactly 15 new copies appear.
   A booster has 11 common, 3 uncommon, 1 rare, no mythics. Basic-land boxes
   each sell one copy for 0.01. Click pack rows to inspect scans/fallbacks.
3. F4: make a named deck from owned cards. Add/remove/search/filter, Save, and
   Use Saved Deck. An unowned quantity cannot be added. Saving a deck never
   changes collection counts. Browse/edit/delete named saved decks as desired.
4. F6: draw a face; check brush sizes, colors, eraser, connected-area bucket fill,
   clear, undo/redo, live orb mapping, Save and Share. The other client should
   see the saved face. Cancel an unsaved edit and verify the old saved face stays.
5. F7: each presses Ready. Verify the selected decks replace the temporary demo
   board, shuffle, and draw private opening hands of up to seven cards each.
   Sit and play. Verify both players can move different objects simultaneously.
6. Use F7 life +/-; both clients should agree. Vote different winners: match
   stays active. Vote the same winner: result becomes agreed. No reward is
   awarded in this phase. Both may ready again with other saved decks.
7. During an active match request Reset from only one player: it must wait.
   Confirm on the other: cards return to reset libraries, hands clear, life
   resets. Both Ready starts the next game. Stale-generation card commands
   cannot change the new table after setup/reset.
8. F5 reconnect the guest, then close/reopen just the guest and rejoin. Inventory,
   selected deck, face, hand and room state should recover from the host.
9. Close/reopen the host too. Balances, owned copies, decks and faces persist;
   the temporary match restarts. Repeat in Release and on the actual two PCs.

Function shortcuts work in the room; use panel tabs while a panel is open.
E also opens the nearest shop/deck/vanity station while standing. F2 leaves the
room; Escape closes an editor (unsaved face/deck edits are discarded on leaving
that editor). Ctrl+S saves; face Ctrl+Z/Ctrl+Y undo/redo. The old L sandbox remains
available separately from persistent Host/Join profiles.

## Files and subsystems

| File | Responsibility |
| --- | --- |
| src/club/Club.h | Bounded profile, saved deck, request, match and face models |
| src/club/Club.cpp | SQLite schema, unique owned copies, atomic purchases/receipts, profile/deck/face persistence and online backup |
| src/club/ClubProtocol.cpp | Explicit bounded serialization; no C++ object-memory copies |
| src/club/BoosterWeights.inc | Generated default MTGJSON sheet weights |
| src/club/ClubUi.h/.cpp | Shop/deck/match actions, search/filtering, palette drawing, fill and undo/redo |
| src/networking/NetworkRoom | Host-only mutation, per-player views, saved identities, public face transfer, ready/life/results/reset |
| src/cards/Tabletop | Variable deck sizes up to 120 per player, generation changes and filtered views |
| src/networking/TableSession | Existing independent object leases plus stale-generation rejection |
| src/graphics/Renderer | Card previews, modal screens, live textured curved orb face patch |
| src/core/Application | Station/shortcut/input integration, saved identity and paths, editor focus |
| tests/ClubTests.cpp | Storage, protocol, painter and actual two-peer UDP integration checks |

The sole new dependency is vcpkg's sqlite3. The pinned port's usage explicitly
provides `find_package(unofficial-sqlite3 CONFIG REQUIRED)` and the imported target
`unofficial::sqlite3::sqlite3`; those are the names used by this build. SDL3/GLM/stb/
ENet and existing environment settings remain unchanged. No database server,
Python runtime, web account or external editor is needed for playing.

## Save model

The canonical database lives on the host under the existing SDL preference root.
`players` stores identity, integer cents, selected deck, and the 256x256 indexed
face. `copies` gives every owned copy a unique persistent integer ID and stable
MTGJSON definition UUID. `decks` stores named lists of stable definition UUIDs;
the host verifies requested quantities against owned copies. A copy may be listed
in multiple alternative decks; only the selected deck is loaded for that player.
Session tabletop IDs are temporary handles, distinct from permanent inventory IDs.

`receipts` is keyed by player and request UUID and includes the original response
and awarded definition UUIDs. Purchases update balance, copies and receipt in one
BEGIN IMMEDIATE/COMMIT transaction with synchronous=FULL and foreign keys enabled.
Rollback leaves the old state intact. Retrying a UUID returns the recorded result
without another debit/credit, even after a host restart. Invalid/insufficient
requests also have receipts so replays cannot change their meaning later.

The default grant is 50,000 cents (500 coins) and one owned 60-card Starter deck,
exactly once per player. Boosters cost 10,000 cents; basic lands cost one cent.
Boosters draw weighted nonbasic cards of matching rarity from the supplied
MTGJSON default sheets, with replacement: 11 common / 3 uncommon / 1 rare.
This preserves the requested gameplay recipe; it does not reproduce historical
print-run collation or allow a basic land to consume the rare slot. The generator
`tools/import_booster_weights.py` regenerates compiled weights from data/source/2ED.json.
It is a development tool only, not a build or play dependency.

Face images use 16 palette colors at 256x256. The editor keeps up to 32 undo steps.
Faces transfer only on save/join/reconnect, never for every brush stroke. Invalid
sizes/indices are rejected and the last valid saved face is retained. The guest
receives its own inventory/decks/balance; it never receives the host's private
inventory. Both saved faces and ready/life/result state are public. Private match
hands and hidden library ordering still use the original per-view filtering.

A saved player-id.txt identifies a device. Same-PC loopback guests use an alternate
identity so a two-window test has separate collections. Each real computer uses
its own saved identity. Keep the same host database for the same private campaign;
host migration/cloud saves/account authentication are not implemented. The host
is trusted, as in Phase 3. Do not expose this as a public server.

## Backups and recovery

A clean host stop creates/updates clubhouse-backup.sqlite3 using SQLite's backup
API. SQLite rollback journaling protects interrupted transactions. Back up both
the database and player-id.txt before manually moving data. To restore: close all
games, preserve a copy of the current database, then copy a known-good backup to
clubhouse.sqlite3 in the same preference folder. Relaunch the original host.
The application reports database errors rather than silently deleting/resetting
collections. Backup files may include the last clean shutdown rather than a later
crashed session. There is no in-game restore button in this milestone.

## Limits and remaining work

- Full Windows Visual Studio Debug/Release, actual SDL input/focus, firewall,
  Tailscale/WAN interruptions and graphics performance require the user's tests.
- Profile features operate in Host/Join mode. Host alone to edit before the guest
  arrives; L retains the original local tabletop sandbox without profile features.
- 16 saved decks, 1–120 cards each. This is manual play; four-copy rules, minimum
  tournament deck size, mana and card effects are not automatically interpreted.
- Reset/setup ends both manipulation leases and advances a table generation so
  queued actions cannot mutate a replacement board. A guest cannot reset alone.
- Collection/deck/face/balance persist. An in-progress room/match is not saved to
  disk and ends if the host closes. Reconnect while the host is open retains it.
- Rewards, ante, throw physics, toys, full surface/furniture painting and final
  presentation polish remain later milestones. The face editor is reusable
  groundwork for the later painting systems, not an implementation of them.

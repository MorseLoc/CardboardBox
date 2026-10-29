# FemboyLover.exe — implementation baseline

Recorded 2026-10-28 from the user's supplied Game Design Document v1.0.
This is a condensed requirements record, not a verbatim copy or replacement
of the original document. The original supplied GDD remains authoritative;
the amendments below override the affected original defaults. Keep this file
with the source so later implementation steps retain the project's direction.

## Amendments supplied with the GDD

1. The eventual title screen should be deliberately ridiculous, raunchy,
   suggestive, and surprising for the player's girlfriend. The game underneath
   remains a cozy shared clubhouse. See `TITLE_DIRECTION.md`.
2. The intended final card content is original Magic: The Gathering Beta /
   Unlimited, not a growing pool of modern sets. Develop the engine using
   original/placeholder assets, with user-supplied definitions/images supported.
   Import and printing selection are future content work; no official card
   database or art is included in Phase 0.
3. Add five basic-land boxes next to the card shop, for Plains, Island, Swamp,
   Mountain, and Forest. Each basic land costs one fictional cent. It becomes
   a persistent uniquely identified owned copy, just like another purchased
   card. Nonbasic lands are not part of this one-cent basic-land service.
4. Use common/uncommon/rare booster slots. Proposed default: 15 cards,
   11 common, 3 uncommon, 1 rare, with no mythics. This is a gameplay collation
   proposal, not a claim of exact historical Beta/Unlimited printing-sheet
   replication. Land inclusion, card-level weights, duplicate handling, and
   historical sheet fidelity must be decided before the pack system is built.
   Once decided, the displayed composition and implementation must agree.
5. Proposed currency representation: signed 64-bit integer cents, with
   100 cents = 1 existing game coin. Starting 500 coins = 50,000 cents;
   basic booster 100 coins = 10,000 cents; premium placeholder 250 coins =
   25,000 cents; completion reward 75 coins = 7,500 cents; winner bonus
   50 coins = 5,000 cents; optional daily allowance 100 coins = 10,000 cents.
   One basic land costs 1 cent. These are fictional units with no real-money
   purchase, cash value, or cash-equivalent stakes. This conversion preserves
   the GDD's example coin prices. No currency code exists yet.
6. Future addition requested 2026-10-28: a simple shape-based toy/token creator
   using the same Paint-style editor as avatar faces. Extend shape building to
   furniture, and let both players paint room surfaces, including walls.
   See section 64 and `CREATIVE_TOOLS.md`. This is planned work; Phase 0
   scope is unchanged by that future creative-tools request.

7. Before Phase 2, resize the shared table/card scale to hold at least 20 columns
   by 20 rows of laid-flat, upright cards, with small gaps and edge margins.
   This is a capacity target, not an enforced placement grid or a 400-card cap.
   Version 0.1.1 uses 0.28 x 0.40 cards and 7.2 x 10.0 felt, with matching
   room/seat/camera adjustments and a toggleable 400-placeholder sizing preview.
   Future physical card dimensions must reuse `src/world/TableLayout.h`.

8. Phase 2 amendment: the user revised the capacity to **20 cards wide by
   10 tall/deep**, replacing the 20-by-20 target in amendment 7. Use the shared
   `TableLayout.h` dimensions: 7.2 x 5.4 felt with 0.28 x 0.40 cards and small
   gaps. Both seats face across the long edges. G now shows 200 reference
   shapes. This remains a capacity guide, not an enforced grid or card limit.

## 1–5. Vision and room

- Private native Windows PC game for exactly two people using the same
  executable, `FemboyLover.exe`. C++20, Visual Studio, CMake.
- Shared small 3D clubhouse: warm wood, muted colors, low-poly charm, readable
  cards, playful physical comedy. It is a social tabletop simulator.
- Physical freedom, persistent collecting, a shared environment, personal
  customization, private multiplayer, and manual rules are the design pillars.
- Main menu: Play, Character Customization, Settings, Exit. Play offers Host
  Room and Join Room with direct IP entry over a private VPN such as Tailscale.
  Never present a room-code system as functional without signaling/relay.
- Both players have the same gameplay functionality, including the guest.
- Required stations: central table and two chairs, booster shop, deck builder,
  toy chest, and face-customization mirror/vanity. Add basic-land boxes beside
  the shop per the amendment above. Decorations and music come later.
- WASD movement, mouse camera, Shift run, E interact, Escape back/pause,
  Enter chat. Smooth movement, furniture/wall collision, synchronized avatars,
  modest movement interpolation. These are gameplay controls, not the Phase 0
  orbit-demo controls.

## 6–7. Avatars

- Small pawn body, large spherical head, optional tiny limbs, body colors later.
- Player draws face on a 256x256 2D canvas mapped onto the front of the head.
- Pencil, eraser, color picker, brush size, fill, undo/redo, clear, save.
- Persist and synchronize face textures. Silly scribbles are welcome.
- Hats, clothing, accessories, different pawn shapes, emotes are optional later.

## 8–15. Physical card table

- Approach table: "Press E to sit." Choose nearest free seat and transition
  camera. Table camera supports pan, zoom, inspection, and each player's near
  side. Leave to explore; first release prevents leaving while holding a card
  during an active match.
- Organizational regions: each side, shared middle, decks, graveyards, exile,
  life counters, optional extra zones. Do not enforce placement rules.
- Separate reusable card definitions from physical instances. Instances track
  stable identity, position, orientation, face state, owner, and container.
- Pick up, drag, rotate, tap/untap 90 degrees, flip, stack, multiselect/move,
  shuffle, draw, inspect, throw, return to deck, enter hand, and create/remove
  temporary gameplay representations without changing permanent ownership.
- Suggested configurable controls: left click/select and drag, right click
  inspect, T tap, F flip, R/Shift+R rotate, double-click deck draw, Ctrl-click
  multiselect, context menu shuffle, explicit held throw control. Drag hover
  and shadow; release returns to surface. Dragging must not accidentally throw.
- Throw physics: 3D velocity, gravity, angular velocity/rotation, table/card/toy
  collision, stable landing, contact indication for Chaos Orb-like throws.
  The host simulates the result. Players judge its rule implications.
- Ordered piles: shuffle, draw one/many, permitted viewing, move, split/merge,
  put on top/bottom. Host owns deterministic ordering. Support public ordered
  graveyards and hidden ordered libraries.
- Private hand faces only to owner; opponents get backs/counts. Face-down
  cards remain unidentified until revealed. Cameras must never bypass privacy.
  Filter network data: do not transmit unauthorized definitions or identities.
  A card back over an already transmitted identity is insufficient. The trusted
  host has complete authoritative state; honest clients are the accepted model.
- Life defaults to 20 and is configurable/manually adjustable. Provide poison,
  turn marker (optional/manual), coin flip, dice, plus/minus and generic counters.
  No automatic winner detection.

## 16–19. Toys and collection data

- Toy chest spawns d6, d20, coin, +1/+1 and -1/-1 counters, number/color
  markers, creature tokens, and blank tokens. Move/remove toys, attach counters
  above cards, edit token labels/values. Temporary objects never alter ownership.
- Reusable token definitions can spawn unlimited session-only instances.
- Every owned collectible copy has a unique ID, definition ID, owner ID,
  acquisition method/time, optional cosmetics. Duplicates are valid.
- Only explicit purchases, rewards, or confirmed transfers change ownership;
  moving a physical card does not.
- Structured definitions: ID, name, type, mana cost, rules text, power/toughness,
  rarity, set ID, front/back images. Use metadata for display/search/sort only.
  Never parse printed abilities into automatic gameplay.
- Keep engine independent of protected assets; support custom content folders.
  Review applicable permissions before packaging official assets. A private or
  free build alone does not grant distribution rights.

## 20–26. Shop, packs, and decks

- Persistent fictional coin balance; completion/win rewards, optional starting
  and daily grants. No real purchases, microtransactions, or excessive grinding.
- Shop displays pack name, price, count, set, rarity composition, artwork.
  Host validates funds and generates contents.
- Atomic purchase commits deduction, complete owned contents, transaction ID,
  and reveal/resume state BEFORE animation. Disconnecting during an opening
  cannot consume coins without delivering cards. Resume/skip reveals.
- Centrally define and test rarity composition; the UI must match it.
  Premium odds and example prices are editable design placeholders.
- Collection browser: images, owned/assigned quantities, search, filters for
  name, type, color, mana cost, rarity, set, owned quantity.
- Multiple named decks: create/save/load/rename/delete, main deck, optional
  sideboard, counts, quantities, selected deck for next match. Owned cards only.
- Deck lists are reusable plans, not exclusive ownership allocations. Different
  lists may share references. Host resolves actual copies at match start.
  Ante-lost cards make lists incomplete with a warning. No mandatory Magic
  legality rules; optional informational warnings only.

## 27–30. Matches and ante

- Both seated players choose decks and confirm matching settings: initial life,
  opening hand size, optional starter/free play, ante toggle. Validate ownership,
  instantiate a fresh state, shuffle both libraries, let players draw manually.
  No automated mulligans, phases, mana, spells, combat, damage, or effects.
- Players manually manipulate cards, toys, life, and zones and resolve printed
  text verbally or through text chat.
- Either proposes P1 win, P2 win, draw, or cancel without rewards. Both must
  confirm the SAME outcome. Mismatch leaves unresolved; disconnect suspends.
  Never award a unilateral disconnect victory.
- Ante disabled by default; both opt in and nominate one owned copy each.
  Lock copies against conflicting ownership operations. Show exact transfer in
  result confirmation. Winner keeps their own copy and gains the loser's.
  Draw/cancel releases both without transfer.
- Commit results, rewards, transfer, history, and idempotency transaction ID
  atomically before showing success. Retries/reconnects cannot duplicate rewards
  or transfers. History records both players and transferred IDs. Optional
  consensual grace-period undo can come later.

## 31–36. Network and storage

- Host runs client plus authoritative session; guest sends requests. Exactly
  two players, matching application/protocol/content versions, private direct
  connection first. No public server or account infrastructure needed.
- Host owns balances, copies, packs, validation, matches, shuffles, privacy,
  card motion/throw physics, rewards/ante, and persistent shared database.
- Explicit versioned serialization and stable identifiers for connection
  (hello/version/accept/reject/disconnect), room/avatar/interaction, table
  (draw/move/rotate/tap/flip/shuffle/throw/toys/state), economy
  (purchase/confirmation/collection/deck), and match/result/ante messages.
- Interpolate movement; prioritize consistent card outcomes. Host orders
  concurrent requests. Temporary manipulation leases resolve simultaneous
  grabs and expire/release on disconnect.
- Disconnect preserves committed collection data and active match where
  practical, suspends unresolved results, releases stale grabs, permits reconnect.
  Save regularly and around important economic operations.
- SQLite permanent state: players, balances, copies, decks, appearances,
  matches, transaction history, settings. Automatic backups. Store writable data
  in the Windows per-user application data directory, never installation files.
  Host DB is authoritative; guest caches are not restore authority. Encrypted
  export/restore is optional later.

## 37–43. Software and presentation

- Target SDL3, OpenGL, ENet, SQLite, JSON, early Dear ImGui, lightweight/custom
  physics or an appropriate library. Use reproducible CMake/vcpkg dependencies.
  No visual scene editor required. Compile on Windows/Visual Studio in C++20.
- Start small; extract modules as needed. Separate application, rendering,
  input, simulation, network, persistence. Avoid empty-class frameworks or one
  giant source file. Example target domains: core, graphics, world, cards,
  networking, economy, collections, avatar, UI, persistence, assets/data/tests.
- States: menu, connection, clubhouse exploration, shop, builder, face painter,
  table setup/active/result, settings. Two primary cameras: room third person
  and table, with smooth transitions and inspection for legible card text.
- Required interfaces: menu, connection, settings, shop/opening, collection,
  decks, face painter, tabletop HUD, results, pause, chat. Utility first, themed
  typography/transitions later. Cozy low-poly visuals; readability first.
- Effects may include footsteps, chair, cards, shuffle, tap, dice, coins, pack,
  purchase, match completion. Adjustable volume/mute required; music optional.

## 44–54. Roadmap and gates

| Phase | Deliverable and completion gate |
| --- | --- |
| 0 | CMake/SDL3/window/events/OpenGL/camera; clean Debug AND Release launch/exit |
| 1 | Floor/walls/pawn/movement/follow camera/collision/table/chairs; explore and sit |
| 2 | Local textured cards/raycast/drag/tap/rotate/flip/piles/draw/shuffle/hand/inspect; manipulate a test deck |
| 3 | ENet host/join/version/movement/seats/card/pile replication/filtered privacy/authority/reconnect; two PCs share room and cards |
| 4 | Deck selection/setup/opening hand/life/accessories/results/dual confirmation/reset; complete a manually governed match |
| 5 | SQLite/players/copies/definitions/browser/decks/resolution/save/load; collection/decks survive restart |
| 6 | Balances/shop/pack composition/reveal/atomic purchases/rewards; complete collecting loop (including added land boxes) |
| 7 | Ante nomination/validation/locks/agreement/transfer/log/retry handling; persistent exactly-once consensual transfer |
| 8 | Throws/angular motion/collisions/landing/replication/toy chest/dice/counters/tokens/coins; physical accessories work |
| 9 | Face drawing/erase/color/save/orb mapping/sync; both faces persist and display |
| 10 | Lighting/UI/audio/animation/settings/restart/disconnect/ownership testing; Release package and separate-PC test without dev tools |

Status: the user confirmed the clubhouse runs well and authorized Phase 2,
with a revised 20-wide/10-deep table. Phase 2 source adds local physical cards,
ordered piles, hands, inspection and tests. It is not multiplayer and has no
persistent collection. Its Windows Debug/Release interaction checklist is
pending. Later systems remain unimplemented. See `PHASE2_NOTES.md`.

## 55. Required verification as systems arrive

- Network: two-PC connection, both avatars/movement/cards, guest reconnect,
  disconnect preserves collections.
- Privacy: inspect actual outgoing payloads for hidden hand/library identities;
  camera cannot reveal private fronts; public reveals propagate correctly.
- Table: tap/untap, no lost cards, shuffle count preservation, draw without
  duplication, ordered stacks, simultaneous grab arbitration, identical throws.
- Economy: no negative balances, rollback on failure, purchased cards survive
  interrupted reveals, duplicate purchase/reward/ante requests apply once.
- Persistence: restart preserves copies/decks/faces/balances; backups restore.
- Call out features requiring two PCs. Never describe untested behavior as
  verified. Test important logic before advancing.

## 56–63. Scope, priorities, workflow, and success

- Exclude automatic Magic rules/effect interpretation, matchmaking, >2 players,
  public servers, real-money purchases, rankings, mobile, accounts, sophisticated
  NPCs, and large maps. Future room/furniture/sleeve/playmat/binder/replay/photo/
  decoration/music/emote ideas must not delay the reliable core.
- Success is two separate Windows computers sharing a cozy room, custom faces,
  movement, packs, collections, decks, seats, hidden hands, free card handling,
  manual Magic, toys/throws, agreed results/rewards/ante, and safe restarts.
- Development partner should teach C++ and game development, not just paste
  unexplained code. Begin at Phase 0 and implement incrementally. Supply entire
  compile-ready files where practical, exact paths, dependencies/build/run
  instructions, expected output, tests, limitations, and the next milestone.
- Begin each phase with goal/purpose/files/implementation/gate. Stop at a
  failing foundation; never generate the entire game at once. Separate working
  systems from placeholders.
- Read existing code before edits, retain working behavior/comments, fix the
  actual bug, avoid unrelated refactors, respect existing architecture, and ask
  for genuinely needed missing files rather than inventing project details.
- Priorities in order: correctness, compilability, multiplayer consistency,
  hidden information privacy, save integrity, readable architecture, performance,
  polish. Modern C++; no unnecessary globals, unsafe ownership, hardcoded asset
  paths, or premature frameworks.
- Preserve pawn/orb faces, tactile physical comedy, collecting/shop, toys,
  consensual ante, and eventual gift distribution for someone without Visual
  Studio. Comfortable shared play is the measure of success.

## 64. Future creative tools: toys, furniture, and paintable clubhouse

- Make small toys/tokens by adding simple primitive shapes, moving, rotating,
  resizing, duplicating, and deleting them. Use friendly controls and previews;
  no modeling experience or external editor should be required.
- Reuse the avatar face painter's pencil, eraser, color picker, brush size,
  fill, undo/redo, clear, and save. Support solid part colors and drawn details.
- Save named reusable creations to a personal catalog accessible from the toy
  chest. Spawn copies as tabletop accessories with optional labels/values.
  Saved creations persist; ordinary match token instances remain temporary
  and never modify collectible card ownership.
- Extend the same builder to simple furniture using starter templates. Players
  can recolor/draw on existing furniture and their custom furniture.
- Both players can paint environmental surfaces, including walls, floors,
  ceiling, table, chairs, counters, shelves, doors, and placed decorations.
  Structural surfaces need independently saved paint layers, not changes to
  the room's collision geometry. "Everything paintable" refers to world-object
  surfaces; it does not replace card identity, printed rules, or system UI.
- Start with selecting an object/part/surface and opening a flat paint canvas
  with a 3D preview. Direct brush strokes on 3D objects are a later convenience.
- Persist room paint and placed furniture independently of match cleanup.
  Synchronize saved creations/paint through the authoritative host, including
  to reconnecting players. Share the editor with guests as well as the host.
- Use versioned saves and a temporary edit lock so simultaneous edits do not
  silently overwrite one another. Preview locally and send compressed saved
  textures/shape descriptions, rather than full textures every render frame.
- Implement after the reliable core, reusing the Phase 8 toys and Phase 9
  painter. Proposed sequence: toy builder, shared surface painting, then
  furniture construction. Detailed plan and future tests: `CREATIVE_TOOLS.md`.

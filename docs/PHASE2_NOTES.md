# Phase 2 implementation notes

**Version 0.2.2 update:** Unlimited data now replaces the initial invented card
set, and cached JPEG/PNG images replace fallbacks when available. See
`UNLIMITED_IMPORT.md`; the original implementation narrative below remains
historical where it describes the eight placeholder definitions.

## Scope and changed layout

The user approved the working clubhouse and changed the capacity request to
20 cards wide by 10 cards tall/deep before proceeding. `TableLayout.h` now uses
20 columns, 10 rows, card dimensions 0.28 x 0.40, and felt 7.2 x 5.4. Slot
pitch stays 0.32 x 0.46, leaving a 6.36 x 4.54 reference-card footprint with
positive gaps and margins. The wooden table is 7.6 x 5.8. Cards are thinner
(0.0015) so a full deck is a plausible small stack. Seats move to Z +/-3.85;
the room remains the existing 20 x 20, with the existing stations and decor.
The overview is at distance 12.8 and pitch 1.12 radians.

The 200-shape G preview is optional and inert. It does not enforce a grid,
consume actual card copies, or demonstrate network performance. The normal
state contains two 60-card test libraries and a separate hand for each side.
This is one local application; opposite-seat testing is not multiplayer.

## Card state and identity

`CardDefinition` contains identity, name, type, printed cost/text, optional
power/toughness text, and a color for original placeholder art. Definition data
is currently supplied in C++, not imported from JSON or an MTG database.

`CardInstance` has a session ID, definition reference, original owner, zone,
hand holder/pile ID, transform, face/tap state, height and ordering serial.
The original owner does not change when another side handles the card. These
session IDs are not a substitute for the permanent owned-copy IDs in Phase 5.

`CardPile` is a bottom-to-top vector of existing IDs. Drawing removes back(),
shuffling changes only order, splitting moves the top half into a second
container, and merge/return detach old membership before adding new membership.
Empty player-made piles are deleted after a complete ownership-neutral move.
Only the two original-library anchors remain internally for returns; when empty,
they are not rendered, labeled, picked, dragged, or used as stacking support.
`valid()` checks that every piled card belongs to exactly one corresponding
pile and every definition/hand reference is valid.

## Selection, rays and dragging

The camera inverts its view/projection matrix to unproject normalized mouse
coordinates at the near/far clip planes. The difference gives a world ray.
The application converts SDL logical mouse coordinates to drawable pixels for
the hand/menu UI; normalized coordinates and the drawable aspect ratio are
used consistently for camera rays on high-DPI displays.

Picking intersects each object's top plane, inverse-rotates the hit into its
rectangle's local coordinates, and chooses the nearest hit, with stable order
for ties. Thus a tapped card uses its actual rotated footprint. Piles are
picked at their stack height; hands use screen-space thumbnail rectangles.

Dragging starts after a small mouse-motion threshold. A snapshot preserves
old state and zone; the object lifts with shadow feedback. Escape, focus loss,
or capacity-preview entry restore the snapshot. Releasing normally commits
position. Hand-to-table motion explicitly makes the card public. Dropping over
the hand bar moves selected loose cards into that side's hand. Standing while
a drag is active is blocked; cancel/place first.

Ctrl-click selects multiple cards from the same kind of zone. B groups loose
cards into an ordered pile. Dragging a pile moves the whole container without
changing the ID order; Shift-dropping onto another pile explicitly merges.
A separating-axis overlap test raises free cards above supporting cards/piles,
including partial edge overlaps. This is kinematic stacking, not rigid-body
physics. No default snap-to-grid, placement legality, or card-count cap exists.

## Fronts, backs, hands and inspection

`visibleDefinition` returns an optional definition reference. Own-hand cards
are visible to that viewer; another hand is hidden. Public face-up loose cards
and revealed face-up pile cards are visible; hidden piles and face-down cards
return no definition. The renderer uses that result for both table textures
and inspection, avoiding an accidental face-up inspection bypass.

A player may explicitly reveal/hide a selected pile with F; no Magic legality
check is applied. Hidden-pile draws require the controlling side; a revealed
public pile can be drawn from manually. Right-click opens visible-card
inspection, while Shift-right-click opens context actions. Public-pile
inspection can browse its existing order with the wheel. Inapplicable context
actions are dimmed and do nothing rather than silently using another pile.

Hands retain draw/add order, support scrolling, and clear private selection
and inspection on seat changes. Room mode does not draw either hand. The
opposite hand is represented by a count in the seated HUD.

This protects local presentation only. The process still contains both test
hands, because there is no network separation yet. Phase 3 must construct
viewer-specific payloads at the host, with opaque hidden handles that do not
expose definition-linked IDs or tracked public-to-hidden identities; this function alone is insufficient for
secure network privacy. No network-privacy test is claimed in this phase.

## Graphics

Eight original fronts and one shared back are generated as RGBA images using
the existing built-in font and a simple procedural pawn/landscape illustration.
Art is clipped to its panel. This avoids external asset paths and extra image
libraries while exercising actual textured 3D cards and large readable fronts.
The renderer uploads the images to OpenGL textures at startup.

The vertex format adds UV coordinates. The existing shader can switch between
untextured room/HUD geometry and a sampled card image. Card faces use a thin
horizontal quad; side thickness, selection borders and hover shadows remain
simple geometry. The hand and inspection reuse those textures in orthographic
screen space. The room's geometry, lighting style and pawn remain intact.

## Automated checks and what they mean

RoomTests checks the updated 200-shape layout, collision, both chairs, safe
standing, camera obstruction, randomized movement and HUD geometry.

CardTests checks card conservation, private/public visibility decisions,
ordered drawing and top/bottom returns, tapping/untapping, flipping, rotation,
hand drag/cancel/play, grouping/splitting/merging/shuffling, oversized and empty
library draws, reuse of emptied anchors, rotated ray picking, edge-overlap
stacking, upper-card picking, controller input and generated image buffers.

CameraTests uses actual GLM camera matrices to round-trip world points through
screen coordinates and back into table rays. It covers both seats, three
aspect ratios, and panned/zoomed views. No SDL window is needed for these tests.

The complete application sources are also syntax-checked with native GCC
against official pinned SDL3 3.2.22 and GLM 1.0.1 headers. This checks C++/API
compatibility on that compiler; it does not verify Windows linking or MSVC.

A separate temporary EGL harness compiled and rendered the actual Renderer,
Camera, textures, shaders, world, pawn, hands, and inspection off-screen using
software OpenGL. Images were visually inspected. The harness substitutes
context setup/function lookup, and therefore does not test SDL window events,
Windows drivers, or two PCs. It is an authoring diagnostic, not a shipped game
entry point or a new project dependency.

## Windows completion checklist

Run in Debug and Release:

- [ ] Walk, collide, sit/stand at either chair, pause, resize and restart.
- [ ] G shows 20 columns x 10 rows; Home frames the whole table.
- [ ] Two libraries begin with 60 copies; Shift+D draws seven to the active hand.
- [ ] Double-click drawing changes library/hand counts by exactly one.
- [ ] Hand drag places a public front; H/hand-bar drop returns it to the hand.
- [ ] T toggles 90 degrees; R and Shift+R rotate; F flips fronts/backs.
- [ ] Right-click fronts/hand cards opens readable inspection.
- [ ] Face-down cards and hidden piles cannot expose fronts through inspection.
- [ ] Ctrl-click/multi-drag works; Escape/focus loss restores unfinished drags.
- [ ] B groups, X splits, S shuffles, and Shift-drop merges without lost copies.
- [ ] L/Shift+L returns selected cards to top/bottom in the documented order.
- [ ] Public-pile inspection browses with the wheel; hidden-pile inspect is dimmed.
- [ ] Large hands scroll; private UI clears when changing seats.
- [ ] C recalls a selected object moved outside the felt.
- [ ] RoomTests, CardTests and CameraTests all print PASS.

If these pass, the next milestone is Phase 3: authoritative host/client ENet
networking using the planned private Tailscale IP connection. Do not call the
current local prototype a connected two-player game.

## Version 0.2.1 regression fix

The previous renderer used max(1, pile size), which gave an empty container a
one-card body/back, while the HUD still printed zero. Custom containers also
remained in the model after their final card moved out.

The fix deletes emptied custom piles only after a complete draw/move/merge,
so a return to the same pile is not interrupted midway through detach/add.
Picking, dragging, rendering, labels and stacking all ignore empty original
library anchors. The controller clears a selection whose pile has disappeared.
No physical card instance is deleted, and emptied libraries still accept
returns. New CardTests cover last-card draws, hand moves, library returns,
same-pile returns, merge-source deletion, non-pickable empty libraries,
refill/reappearance and stale selection, preserving all 120 copies.

The next phase remains the GDD's Phase 3: private host/join, version agreement,
two synchronized pawns and seats, authoritative card/pile actions, per-client
hidden-information filtering, temporary manipulation locks, disconnection and
reconnection. Two separate Windows computers are needed for its final test.
Shop, persistent collections, ante, physical throws and face painting keep
their later milestones; they are not added by this bug fix.

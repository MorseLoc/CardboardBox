# Phase 3B.1 validation

This is a source patch to the working 3B build. The included earlier
PHASE3B_ORIGINAL_VALIDATION.md records baseline checks, not new 3B.1 results.

## New checks

- PASS: native C++20 TableNetworkTests, including 150 rounds of simultaneous
  grabs/cancels/finishes, independent cards and piles, same-object contention,
  held-pile mutation rejection, cancellation of only one player's lease,
  expiry, and preventing a held card from supporting another card's drop.
- PASS: actual ENet UDP host/guest simultaneous hand-card dragging, unrelated
  draws during another grab, saved-token reconnect and process restart,
  repeated-command deduplication, filtered private hands/hidden-pile anonymity,
  and 120-card conservation.
- PASS: existing physical card/controller/visibility/image tests after the
  drag-context refactor. Local mode retains the original single-pointer drag.
- PASS: strict application/graphics/network syntax against pinned SDL3/GLM
  headers with -Wall -Wextra -Wpedantic.
- PASS: real offscreen EGL/OpenGL renderer initialized with an EMPTY user cache
  and a separate bundled-art root. Fixture artwork appears on the table;
  host/guest hands, inspection, room, and menu frames render without GL errors.
  The host frame was visually inspected.

Windows Visual Studio Debug/Release and two real PCs remain manual checks.
Check each player can hold/move a different card at once; then try the same
card, pile and merge target. Closing a guest mid-drag must not cancel the host's
held object. Copying art alone needs only a relaunch, not a rebuild.

The new artwork root is resolved from the executable directory via SDL_GetBasePath.
No actual Unlimited scans are bundled into this source ZIP. No shop, deck builder,
or face editor is implemented in this patch; their combined Phase 4 scope is in
PHASE4_SCOPE.md.

# Phase 4: shop, decks, face painting, and match setup

This is the next implementation milestone requested by the user. None of these
new systems is implemented in Phase 3B.1. Retain the tested multiplayer foundation,
independent object locks, private hands, Unlimited-only pool, and 20-by-10 table.

## Persistent collecting and shop

- Separate persistent player balances and uniquely identified owned card copies.
- Preserve the recorded integer-cent economy. Five basic-land boxes sell each
  basic land for one fictional cent; no real money or real-money purchases.
- Buy/open Unlimited boosters with the imported 11 common / 3 uncommon / 1 rare
  recipe and the supplied rarity metadata/weights. No mythic slot.
- Validate and commit purchases on the host. Retrying/reconnecting cannot charge
  or award cards twice; failed purchases do not lose coins.
- Persist collection and balances through restart, with recoverable saves.
- Present the shop through its existing room station and show pack contents.

## Deck builder

- Browse/search/filter owned Unlimited cards with artwork and quantities.
- Add/remove quantities; enforce owned-copy availability without interpreting
  Magic card rules. Do not let building a deck create collectible copies.
- Save, name, edit, select and delete multiple decks per player.
- Keep deck edits independent from other decks: a saved deck is a list of owned
  copies/quantities, not destructive movement of cards out of the collection.
- Load the selected deck for each player at the shared table with correct hidden
  libraries, shuffled setup and private opening hands. Returning to collection
  must not duplicate, consume or change ownership of copies.
- Persist decks and selection through restart. Use the existing deck desk.

## Face painter

- Use the vanity to open a simple Paint-style 256-by-256 canvas.
- Pencil, eraser, color palette/picker, brush sizes, clear, undo/redo, save/cancel.
- Show a live preview mapped to the front of the orb head.
- Persist the saved face and synchronize it so both players see each other.
- Validate image dimensions and size; transfer on save/join/reconnect, not every
  brush stroke. A canceled edit leaves the last saved face unchanged.
- Reuse this editor later for toy, furniture and environmental painting; do not
  pull the full shape builder or room-painting feature into this milestone.

## Manual match flow

Retain the original next-phase requirement for deck selection/setup, shuffled
libraries, private opening hands, life tracking, mutually confirmed outcomes and
safe table reset. Keep all card abilities/combat/mana resolution manual. Detailed
rewards and consensual ante remain separate later milestones.

## Completion checks

Both Windows players can buy/open packs and one-cent lands, restart without
losing balance/collection, build and reload owned-card decks, play those decks at
the shared table, paint/save faces, and see the saved faces after reconnecting.
Transactions and deck loads preserve copy counts and private information.
Development may use incremental source updates under this combined Phase 4;
no feature is reported complete before its implementation and checks pass.

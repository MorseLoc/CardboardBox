# Future addition: make little things and paint the clubhouse

Requested 2026-10-28. Design only: no creative-tool runtime code is implemented.
This expands the original toy chest, avatar painter, and future furniture
customization. It must not delay verifying Phase 0 or building reliable cards,
networking, collections, and matches.

## Player experience

At the toy chest, choose **Create Toy**. Start with a blank project or a token
base. Add cubes, spheres, cylinders, cones, or flat discs. Select each part to
move, rotate, resize, duplicate, delete, or choose its color. Keep these as
simple controls with obvious handles and a live preview.

Choose **Paint** to open the same friendly editor used for drawing orb faces:
pencil, eraser, colors, adjustable brush, fill, undo/redo, clear, and save.
Select a part or surface to draw details on its flat canvas and see the result
on the 3D preview. Solid coloring alone should be enough to make a useful toy.
Begin with simple surface layouts; avoid making players understand UV mapping.

Save it with a name such as "Extremely Suspicious Goblin." It appears in a
saved-creations tab of the toy chest. Spawn copies, pick them up, move them,
and use them as toys or labeled game tokens. Tokens can have editable numbers
without any automatic interpretation of their game rules.

The same editing approach later supports furniture: begin with a stool,
chair, shelf, or table template, adjust primitive parts, and paint it.
Furniture templates and previews should keep the first version approachable.
Freeform mesh editing, sculpting, bones, and animation are outside this scope.

## Paintable world

Players can select an existing object or room surface and choose **Paint**.
The goal is paintable toys, furniture, decorations, table surfaces, walls,
floor, ceiling, doors, and fixtures. Each wall and furniture surface can carry
its own drawing. Painting one chair must not accidentally repaint all chairs
that share its original material.

Use an original material plus a persistent per-object paint layer. Erasing
removes paint to expose the original surface; **Reset Surface** restores the
whole selected surface. A 3D preview makes the selected face unambiguous.
Painting an object's appearance does not change its physical shape.

The first version edits a selected surface in a 2D canvas. Direct painting
onto a 3D object with the mouse is an optional later improvement using the
same underlying paint layers. Do not require this harder interface to deliver
player-painted walls and toys.

## Shared implementation approach

- One reusable paint document/tool implementation, with adapters for avatar
  faces, toy parts, furniture, and environmental surfaces.
- A creation definition stores stable IDs, a name, primitive shapes, part
  transforms, colors/materials, paint references, and optional token defaults.
- A spawned instance references its creation definition and has its own ID,
  transform, physics state, and editable token values. Keep saved templates
  separate from instances. Editing a saved template must not silently mutate
  all existing spawned copies; use explicit definition revisions.
- Start with the existing 256x256 face canvas for small toy details. Room
  surfaces can use bounded larger canvases after visual/performance testing;
  their appropriate resolution is undecided. Avoid unlimited texture sizes.
- Physics uses simple primitive or compound colliders. Paint does not alter
  them. Furniture placement needs collision checks so pieces do not trap players
  or block required interactions. Editing walls does not require remodeling
  structural geometry.
- Persist reusable creations, paint assets, furniture placements, and room
  appearance in writable per-user storage, through the host's authoritative
  save system. Preserve original assets for reliable resets.
- Match cleanup removes temporary tokens/accessories while retaining their
  saved definitions and the clubhouse's paint/furniture.

## Two-player behavior and connection use

Both players can create and paint. A brief host-managed edit lease protects
an object/surface while it is being edited; disconnects release the lease.
The other player can continue editing a different object.

Preview brush strokes locally. On Save, submit the expected previous revision,
shape description, and compressed paint assets. The host validates and commits
the edit, then distributes the new revision. Reject stale saves with a clear
reload choice, preserving the unsaved local drawing for recovery. Undo/redo
initially operates on the local editing session rather than undoing the other
player's later committed work.

Cache assets by content hash. Reuse unchanged textures, send missing assets
when joining/reconnecting, and avoid sending a complete texture every frame.
This adds occasional asset transfers; actual bandwidth depends on texture size
and how often players save, so measure it once implemented.

## Delivery order and acceptance criteria

After the reliable core and the reusable avatar painter exist:

1. **Toy/token creator:** build a small multi-part toy, color/draw on it, save,
   restart, spawn copies, label a token, and manipulate it physically.
2. **Paintable clubhouse:** select and paint existing furniture and individual
   room surfaces; reset a surface; verify changes survive restart and are
   visible to both players, including after reconnecting.
3. **Furniture builder:** edit a simple furniture template, save/place a piece,
   preserve paint and collision, and restore it after restart.

Future tests must cover independent material instances, undo/redo, erased
transparency over the original material, template/instance separation,
disconnect during editing, stale saves, asset caching, room paint surviving
match cleanup, and both clients receiving identical saved revisions. Multiplayer
behavior requires host/client testing; none of it is verified by Phase 0.

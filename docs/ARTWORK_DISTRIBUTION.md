# Card artwork: development and final distribution

The game's connection sends card IDs/state, not JPEG or PNG files. Each computer
needs its own local artwork files. Text fallback faces do not indicate a network
failure.

## Make the second PC display scans now

Close its game. On the computer with the scans, open this folder in File Explorer:

```text
%APPDATA%\FemboyLover\Clubhouse\cards\2ed
```

Copy the entire `2ed` directory to the same per-user folder on the other computer.
Keep all filenames unchanged and relaunch the game. If you prefer downloading,
run tools/Download-CardImages.ps1 on that second computer from its project root.
It uses the same per-user cache. No recompile is needed just for cached images.

## Ready-to-play ZIP layout supported by Phase 3B.1

```text
FemboyLover.exe
SDL3.dll
assets/cards/2ed/<printing-scryfall-id>.jpg
assets/cards/2ed/<printing-scryfall-id>.png
```

Every printing only needs its actual image file, JPG or PNG. Include the complete
validated cached set so card inspection works offline. The renderer resolves
`assets/cards` from SDL_GetBasePath, which points beside the executable; it does
not depend on the launcher's working directory. It checks the writable per-user
cache first, then bundled files, then readable text fallback faces.

No first-run internet download is required when artwork is bundled. This patch
adds the loading path; it does not build the final Windows package or bundle the
actual scans into this source update. Final packaging remains after gameplay is
complete. It will gather the Release executable/runtime plus these cached scans,
then be tested offline on the second Windows computer. If any scan is absent or
corrupt, the fallback remains usable; it is not silently downloaded by the game.

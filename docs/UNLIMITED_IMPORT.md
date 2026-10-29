# Unlimited import — version 0.2.2

## What the supplied file contains

The uploaded `2ED.json` is MTGJSON 5.3.0+20261028 data for Unlimited Edition,
not Scryfall's native JSON schema. It contains 302 printing records with unique
MTGJSON UUIDs and Scryfall printing UUIDs. All are English records for 2ED.
Rarity counts are 117 rare, 95 uncommon, and 90 common; those are catalog counts,
not pack probabilities. The 15 basic-land printings are three variants each
of Plains, Island, Swamp, Mountain, and Forest.

Its booster section explicitly describes 11 common, 3 uncommon, and 1 rare
slots, plus weighted sheet membership. That data is preserved for Phase 6;
no functioning shop/pack-opening system is introduced in this update.

The file has no `image_uris` objects or embedded artwork. Scryfall printing IDs
provide the link to retrieve the exact printing's image metadata. The downloader
uses those IDs rather than searching by name and potentially choosing a modern
reprint.

## Field mapping

| MTGJSON source | Game/import output |
| --- | --- |
| `uuid` | Stable definition ID; also matches supplied booster-sheet keys |
| `name` | Display name |
| `originalType`, falling back to `type` | Printed-type field for fallback display |
| `manaCost` | Printed cost string; never automatically spent/validated |
| `originalText`, falling back to `text` if absent | Rules text shown by text fallback |
| `text` | Current rules text retained separately as `oracleText` |
| `power`, `toughness` | Display-only power/toughness string |
| `rarity`, `number`, `colorIdentity` | Collection/pack/display metadata |
| `supertypes` containing Basic | Basic-land classification |
| `identifiers.scryfallId` | Printing-specific image lookup/cache filename |
| `booster` | Preserved future pack configuration, not executed yet |

Source fields are carried as data. The engine does not parse or execute card
abilities. The provided `original*` values are not independently corrected or
claimed to be a historical transcription; the exact-printing scan, when
available, shows the actual card face.

## Files

- `data/source/2ED.json`: the supplied file, unchanged.
- `tools/import_mtgjson.py`: standard-library Python conversion tool.
- `src/cards/UnlimitedCards.inc`: generated C++ entries, already included.
- `data/sets/unlimited.json`: readable normalized definitions plus booster data.
- `data/sets/unlimited-images.json`: printing-ID download manifest.
- `src/cards/CardCatalog.h/.cpp`: catalog and temporary 60-card demo-list helper.
- `tools/Download-CardImages.ps1`: one-time Windows downloader.
- `src/cards/CardImages.h/.cpp`: native JPEG/PNG decoder using pinned stb.

Python is needed only if a developer changes/reimports the source JSON. The
archive already contains generated C++ and manifests, so Visual Studio builds
and the game itself do not require Python or a runtime JSON parser. To regenerate:

```powershell
python tools/import_mtgjson.py --input data/source/2ED.json --root .
```

The converter validates the expected set/count and unique printing IDs. It
currently targets this 2ED milestone; importing Beta is a deliberate later
extension, not a filename substitution that silently changes the set.

## Image setup and runtime

Close the game and run from the project root in Windows PowerShell 5.1:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\tools\Download-CardImages.ps1
```

The execution-policy setting applies only to that launched PowerShell process.
The script sends a descriptive User-Agent, paces requests, retries temporary
failures, validates exact printing/set identity and HTTPS image hosts, writes
to a partial file, checks image signatures, then replaces the cached file.
Existing valid-looking cached files are skipped unless `-Force` is supplied.
Access denial stops the request loop, and a JSON report records failures.
No accounts, paid services, real-money prices, or credentials are involved.

The default cache is `%APPDATA%\FemboyLover\Clubhouse\cards\2ed`. The application
obtains the corresponding root with SDL_GetPrefPath and handles it as a UTF-8
filesystem path. The downloader's `-CacheRoot` option is for troubleshooting;
normal gameplay uses the default SDL preference location.

The decoder accepts JPEG/PNG, rejects missing/malformed/oversized images, and
preserves row orientation. It does not trust a `.jpg` extension alone. Small
192x270 textures load lazily for table/hand views; only one full-resolution
inspection texture is retained at a time. If all 302 front thumbnails are seen,
their raw GPU pixel storage is about 60 MiB, rather than loading hundreds of
full-resolution textures at startup. Existing card back art stays original.

Cached scans are local files; the game does not fetch artwork during play.
Restart after downloading or replacing files, because already loaded GPU
textures remain cached for that run. On a missing/corrupt image, the game
shows a labeled text fallback using the uploaded metadata. It does not invent
artwork or pass a generated picture off as an original scan.

## Scope

The imported catalog is 302 definitions. The local demo still instantiates two
temporary decks of 60 copies: 24 basic lands and 36 compatible-color spells or
artifacts. Side one uses blue/green lands and side two red/white. These are
prototype lists, not a saved deck builder, legal-format guarantee, earned
collection, booster purchase, or permanent ownership grant.

All 15 land image variants are retained. Basic-land buying boxes remain
placeholders for Phase 6. Card throwing, face painting, persistent collections,
ante, and networking remain in their existing later phases. The 20-wide by
10-deep table, manual card controls, and empty-pile fix are preserved.

## Verification

The import is checked for 302 unique definitions/printing IDs, 15 basic-land
variants, expected rarity totals, preserved original/current text, and two
60-card demo lists with 24 lands each. Existing card-conservation, pile cleanup,
privacy-presentation, interaction, and camera tests remain applicable.

Image tests use small synthetic JPEG/PNG fixtures, not alleged MTG scans, to
check decoding, orientation, resizing, missing files, and corrupt-file fallback.
Native source syntax is checked against the pinned headers. Off-screen
OpenGL checks exercise the actual renderer's fallback and external-cache paths.
These checks do not establish that the online API is reachable on your PC.

Windows PowerShell and live Scryfall image downloads could not be executed in
the authoring environment. The provided tool must be run on your Windows PC;
original image files are not included in this source archive. Review the final
counts/report if some images are unavailable. Missing ones remain usable as
text fallbacks. Windows MSVC build, cache-path agreement, and visual checks
are still required locally.

Windows completion check: configure/build after stb is installed; run the
image downloader; launch the game; draw/play/inspect cards; verify downloaded
images replace fallbacks and have Unlimited borders/printing; confirm library
counts and empty-pile cleanup still work; then repeat Release. Phase 3 remains
the next development milestone.

# Town Centre street pass 18

October 12, 2026 UTC. Live editor authoring continued on PID `13584` in `C:/dev/cribtest-game/apps/companion-unreal/`.

## District

- Selected district remains `town-centre` from `output/cribtest11-visible-world-2026-10-11/town-centre-district-selection.json`.
- Canonical bounds: `minX -556.625`, `maxX 573.92`, `minZ -992.69`, `maxZ 28.855`.
- Roads in current proof focus: `market-road`, `civic-street`.
- Connected destinations still in the loaded neighbourhood context: `corner-supermarket`, `salon`, `dance-again`, `pool-courtyard`, `civic-reception`, `civic-terrace-lounge`.

## What changed

- Added `apps/companion-unreal/Scripts/WorldGeneration/Connected/town_centre_fill_pass16.py`.
- Executed three saved live passes against `/Game/BaddieGeneratedWorld/Connected/TownCentre_20261011/Maps/L_town_centre_neighbourhood_20261011_streetpass01`:
  - `town-centre-street-pass-16-corner-fabric.json`
  - `town-centre-street-pass-17-corner-fabric-reanchored.json`
  - `town-centre-street-pass-18-corner-fabric-grounded.json`
- Pass 18 is the latest saved state. It clears the old `TC_Fill13_`/`TC_Fill14_` actors, then rebuilds the failed proof corridors with frontage-facing cloned market/civic rows plus added pavement slabs.
- The parent map and detail level were saved and safely reloaded after pass 18.

## Actual evidence

- Latest preview set:
  - `output/cribtest11-visible-world-2026-10-11/previews/town-centre-streetpass01-fill18-daylight/market-road-frontage-a.png`
  - `output/cribtest11-visible-world-2026-10-11/previews/town-centre-streetpass01-fill18-daylight/market-road-frontage-b.png`
  - `output/cribtest11-visible-world-2026-10-11/previews/town-centre-streetpass01-fill18-daylight/civic-street-approach.png`
  - `output/cribtest11-visible-world-2026-10-11/previews/town-centre-streetpass01-fill18-daylight/civic-terrace-open-a.png`
  - `output/cribtest11-visible-world-2026-10-11/previews/town-centre-streetpass01-fill18-daylight/civic-terrace-open-b.png`
- Latest honest review:
  - `output/cribtest11-visible-world-2026-10-11/town-centre-street-pass-18-defects.json`

## Current truth

- `market-road-frontage-b` improved materially: readable shutters, balconies and a more believable pass-by frontage rhythm.
- `market-road-frontage-a` improved, but still opens into unfinished void beyond the frontage line.
- `civic-terrace-open-a` improved on the left edge, but still has a plain opposite shell plane.
- `civic-street-approach` is currently broken by a severe black overhang/occlusion.
- `civic-terrace-open-b` still reads mostly as a side wall, even with better ground support.

## Next best move

- Replace the civic proof-corridor clones with shorter parcel-length slices rather than whole copied regions.
- Remove the overhanging civic clone affecting `civic-street-approach`.
- Continue closing visible rear/side parcel ground so the player-height market and civic views stop exposing blue void.

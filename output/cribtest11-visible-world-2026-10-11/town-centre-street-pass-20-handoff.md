# Town Centre street pass 20

October 12, 2026 UTC. Live editor authoring continued on PID `13584` in `C:/dev/cribtest-game/apps/companion-unreal/`.

## District

- Active district remains `town-centre` from `output/cribtest11-visible-world-2026-10-11/town-centre-district-selection.json`.
- Proof focus remains `market-road` and `civic-street`.
- Current loaded connected map after save/reload: `/Game/BaddieGeneratedWorld/Connected/TownCentre_20261011/Maps/L_town_centre_neighbourhood_20261011_streetpass01`.

## What changed this continuation

- Continued using `apps/companion-unreal/Scripts/WorldGeneration/Connected/town_centre_fill_pass16.py`.
- Saved two new bounded passes after the previous pass-18 checkpoint:
  - `town-centre-street-pass-19-corner-fabric-trimmed.json`
  - `town-centre-street-pass-20-corner-fabric-civic-ground-fix.json`
- Pass 19 proved trimming `civic_face_a` was not enough to remove the broken civic approach frame.
- Direct actor-bounds inspection around `civic-street-approach` identified `TC_Fill16_civic_ground_a` as the real overhang culprit.
- Pass 20 removes that oversized civic ground slab and resaves the same connected map/detail level.

## Actual evidence

- Latest receipt:
  - `output/cribtest11-visible-world-2026-10-11/town-centre-street-pass-20-corner-fabric-civic-ground-fix.json`
- Latest previews:
  - `output/cribtest11-visible-world-2026-10-11/previews/town-centre-streetpass01-fill20-daylight/market-road-frontage-a.png`
  - `output/cribtest11-visible-world-2026-10-11/previews/town-centre-streetpass01-fill20-daylight/market-road-frontage-b.png`
  - `output/cribtest11-visible-world-2026-10-11/previews/town-centre-streetpass01-fill20-daylight/civic-street-approach.png`
  - `output/cribtest11-visible-world-2026-10-11/previews/town-centre-streetpass01-fill20-daylight/civic-terrace-open-a.png`
  - `output/cribtest11-visible-world-2026-10-11/previews/town-centre-streetpass01-fill20-daylight/civic-terrace-open-b.png`
- Latest honest review:
  - `output/cribtest11-visible-world-2026-10-11/town-centre-street-pass-20-defects.json`

## Current truth

- `civic-street-approach` is no longer broken by a black overhang. This is the key improvement from this continuation.
- `market-road-frontage-b` remains the strongest improved proof view, with readable frontage rhythm.
- `market-road-frontage-a`, `civic-terrace-open-a`, and `civic-terrace-open-b` are still incomplete because they fall into blue void or long side-wall reads.
- The district is still editor-authored only. No runtime traffic, pedestrians, gameplay admission, or true playability was established.

## Best next move

- Build shorter parcel-level civic replacements instead of whole copied frontage rows for the remaining side-wall views.
- Add explicit parcel backing and opposing-side closure on Market Road before moving to another district.
- Keep Town Centre active; do not move to the next district until the remaining proof views stop reading as shell edges and unsupported void.

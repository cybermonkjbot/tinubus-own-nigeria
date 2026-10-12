# Town Centre parcel alignment and inland support

We continued on verified editor PID `13584`, project `C:/dev/cribtest-game/apps/companion-unreal/`, Unreal `5.8.3-58210709+++UE5+Release-5.8`. Previous checkpoint was `c166e1f`. This is partial world authoring, not a completed district or playable game.

## Saved native output

Pass23 replaced owned camera-aligned filler and identity-matched original frontage on nine canonical parcels: `market-road-5`, `market-road-6`, `market-road-7`, `market-road-8`, `market-road-9`, `civic-street-2`, `civic-street-4`, `civic-street-5`, `civic-street-6`. Replacement packs use saved entrance-to-parcel-centre direction and fit their frontage width to parcel width. Exact original removals, accepted asset identities/materials/source hashes and new transforms are retained in `town-centre-street-pass-23-parcel-frontage.json`. No hero destination or canonical parcel was relocated.

Pass24 adds instanced ground within saved Town Centre X `-556.625..573.92`, Z `-992.69..28.855` metre bounds. Source lagoon south-bank interpolation proves the minimum bank Z over this interval is beyond the northern bound. Ground therefore closes inland gaps without changing a coastline. Exact source, bank check, dimensions and package hashes are in `town-centre-street-pass-24-inland-support.json`. Ground top is Unreal Z `-24` cm. All 2,964 instances survived a safe reload.

Saved maps:

- `/Game/BaddieGeneratedWorld/Connected/TownCentre_20261011/Maps/L_town_centre_neighbourhood_20261011_streetpass01`
- `/Game/BaddieGeneratedWorld/Connected/TownCentre_20261011/Maps/L_town_centre_neighbourhood_20261011_streetpass01_detail01`

Both passes were saved and safely reloaded with exception/transient references released. Matched five-view captures are preserved in `previews/town-centre-streetpass01-parcel23-daylight` and `previews/town-centre-streetpass01-inland24-daylight`. Every actual PNG was inspected. The large foreground blue voids are absent in the pass24 set; this is the clear visual improvement. Ground continuity does not prove navigable or playable destination routes.

## Defects and recovery

Pass23 exposed sparse, thin frontage strips and blank back/side walls when old camera-aligned clones were removed. Pass24 fixes underlying voids, not those building defects. The uniform tiled concrete land finish, surface seams, raised slab cuts, absent parcel-depth massing and unfinished surroundings remain unacceptable as a finished Lagos neighbourhood. Civic approach is still dominated by a blank back wall. Market proof views now show buildings mostly side-on. Do not restore camera-facing clones to disguise this.

The first pass24 attempt failed on unavailable `Actor.add_instance_component`. The partially constructed component had no mesh and zero instances. Recovery inspected and removed only that empty owned actor, saved the owned dirty maps, cleared exception references, then used the established `SubobjectDataSubsystem` instancing pattern. No editor restart occurred. The failure is preserved in `town-centre-street-pass-24-failure.json`.

The old region snapshot helper includes already-cloned TC actors within source bounds; do not reuse it for new source extraction without excluding copies. Pass23 instead uses accepted immutable frontage receipts directly.

## Next dependency-ready work

Town Centre remains active. Build parcel-depth exterior bodies and corner returns behind the aligned frontage, maintaining source entrances and canonical setbacks. Resolve blank side walls, road/pavement heights and seams, then add varied compound boundaries, services, rooflines and street activity props from accepted libraries. Use additional source-aligned approach views alongside the fixed regression cameras; do not move existing cameras to conceal defects.

No interior expansion, traffic/NPC simulation, runtime admission, navigation generation or gameplay verification occurred. Existing static danfo remains static. No tests, lint, typechecks, build, compilation, packaging, installation, deployment or production configuration changes ran. Unrelated project, Config, native-generator and documentation edits remain untouched. Full city and gameplay inventories remain incomplete; this evidence supersedes historical zero-output status only for the saved Town Centre content it covers.

# College Lane shop exterior

October 12, 2026. Continued checkpoint 691aa7ba37852ff0949c0e03165f168124def6a3 in this chat. Fresh process and authenticated bridge inspection confirmed running editor PID13584, BaddieCompanion in C:/dev/cribtest-game/apps/companion-unreal/, UE5.8.3. No other worker or restart. The full-game goal remains active and incomplete.

## Saved output

College Lane18 now has a source-centred three-storey pass-by shop exterior: five closed ground-floor shutter bays, upper windows, four corner balconies, AC assemblies, structural slabs/posts, supported corrugated shed roof, five gutters and two straight rainwater pipes. This is new exterior content on an empty source parcel, not a finished connected neighbourhood. No destination or road was relocated.

Source college-lane-18: town-centre, corner-shop, X503.28/Z-210.36 metres, width22.33/depth17.33m, three floors, entrance [509.92,0,-219.95], yaw2.5355709047201156 radians. Road college-lane retains its 14 source polyline points, 11m width and 3m pavement definition. Town Centre remains X-556.625..573.92, Z-992.69..28.855m, 138 parcels. Pinned plan SHA25613a2699ee6ad513616ca25a41bbb05f165e9aeaf3e52b5d89e550ed8b523ad9a. No coastline decision was made.

New native packages under /Game/BaddieGeneratedWorld/Connected/TownCentre_20261011/Fabric49/Meshes/:

- SM_college_lane_18_StructuralFrame
- SM_college_lane_18_corrugated-roof
- SM_college_lane_18_pipe_-1
- SM_college_lane_18_pipe_1

Saved maps:

- /Game/BaddieGeneratedWorld/Connected/TownCentre_20261011/Maps/L_town_centre_neighbourhood_20261011_streetpass01
- /Game/BaddieGeneratedWorld/Connected/TownCentre_20261011/Maps/L_town_centre_neighbourhood_20261011_streetpass01_detail01

Receipt town-centre-street-pass-49-parcel-frontage.json contains125 actors, exact module/source identities, transforms/materials, four native assets and immutable OBJ hashes in fabric49-import-source/. Existing accepted frontage-v3 manifest and hash verifier were used. Unit-scale wall/window/balcony/shutter/AC/gutter modules retain source FBX hashes, pivots and materials. Fitted native cm meshes retain installed OBJ Y reflection, winding conversion and UV0 U=(X+Z)/271,V=1-Y/271 for frame/pipes, U=X/271 for roof. Complex-as-simple static collision is authored, not gameplay admission.

Both maps saved and safely reloaded after releasing transient Python modules/UObject wrappers, standard last-exception references and gc.collect(). Saved readback verifies125 actors,12,540 UV probes, exact transforms/material bindings, three source floors, hashes, retained source entrance ground support, clean packages and no game world.

## Selection correction and actual images

The initial two-parcel read-only preflight is preserved as town-centre-street-pass-49-preflight.json. Four before images were captured. Direct street inspection exposed existing signature geometry at College Lane16, missed by ordinary parcel tags. Live actor inspection identified its native signature level:

/Game/BaddieGeneratedWorld/R_c58c0f7905cfbecf99734bf7/Maps/L_city_signatures_college_lane_16_b1decfaf70

No mutation had occurred. Selection narrowed to College Lane18; shop-preflight.json records the final plan. Authoring now rejects canonical signature IDs explicitly. College Lane3/4 are outside Town Centre and untouched. No old frontage actors matched College18, so no geometry was removed. The signature level, assets and adjoining destinations remain unchanged.

Both College18 after images in previews/town-centre-fabric49-after/ were directly inspected against their retained before images. Matched cameras:165cm height, pitch5, FOV78, unchanged daylight,705x486 software rendering. They show the empty parcel replaced by the shop block without apparent gross floating/merged-frontage overlap. Final visual acceptance remains incomplete: sparse context, similar shutter distress, no signs/services, pavement seams/forecourt joins, roof/gutter outlets, and rear/side contact checks remain. Street view clips part of the roof edge. town-centre-street-pass-49-defects.json records these gaps.

No new destination-to-destination route was completed or accepted. Existing College Lane road/entrance supports remain, but physical walking and drain-transition acceptance are not established. No interior expansion. Static closed shutters are not functioning shops; static collision is not playable movement.

## Inventory and next work

world-inventory-reconciled-20261011-pass49.json retains item-level evidence for all seven districts,806 parcels,264 roads,19 signatures,40 destinations,49 Home recipes,twelve waterfront destinations and five rural areas. Historical zeros do not overwrite newer output. College18 is partial native content, not complete. Town Centre stays partial; no completion percentage is inferred.

Next dependency-ready area remains Town Centre: inspect canonical ordinary Arcade Crescent parcel IDs and their native content before authoring varied adjoining blocks; finish College18 forecourt/source entrance contacts. College16 requires source-specific signature details, not generic shell replacement. No new external asset input is required for the next ordinary exterior batch. Rejected foliage/transport/soil art remains excluded; no asset downloads or imports beyond the owned fitted meshes occurred.

Matching approved native binaries, existing admitted entry/movement/control owners, character bindings and navigation integration remain runtime dependencies. No codec, wallet/save, credential, authority/readiness, gameplay actor or production configuration changed. No gameplay movement, residents, traffic, working drainage or account access was inspected.

Checkpoint only owned Fabric49 assets, two maps, three scripts, receipts/inputs/captures/inventory and these records with hooks disabled. Preserve unrelated .uproject, Config, native_generator.py, port-document changes, unowned assets, previous maps/captures and Blender workbench. Native-only authoring changes no consumed mobile/web renderer document. No tests, lint, typechecks, compilation, project generation, full builds, HLOD, packaging, tools installation, deployment or pushes ran. Unreal's ordinary import/save asset processing occurred through the existing helpers.

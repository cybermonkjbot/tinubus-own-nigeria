# Ebute Metta street-level detail checkpoint

October 13, 2026. We implemented an inert detail planner and integrated its dimensioned output into the offline Ebute exporter. The full design is in `ebute-metta-detail-v1.md`.

## Delivered

The schema-v2 packet contains five mapped streets, 17 unavailable walking edges, 18 individually described fictional premises and 792 inherited source-module placements. Added authoring data includes 54 room volumes, 54 door targets, 95 fit-out prop placements, 25 mesh recipes, six material recipes, five exposure/maintenance finish recipes and 18 positioned audio emitters. Eight premises have bounded service capacities and separately positioned waiting recesses. Work-state sequences, game-pacing ranges, cancellation rules, privacy transitions and dry/rain/haze/night intent are included.

We corrected native coordinates to UE X south, Y east, Z up and anchored all new room/prop/door/audio positions to the same rotated frontage frames. The previous immutable packet `1dbf62be33b2f139806099900780d34e1546417e252f9292842e4ce543b08086` has the incorrect ground/vertical transform and must not be used for scene placement. Historical outputs are preserved. The config records the fresh October 13 read; the inherited namespace's `20261014` suffix remains an identity, not the access date.

## Packet

Path: `output/lagos-mosaic-authoring/mosaic-ebute-metta-98a5b42c88a75e047ebea7dfe66d889ea4401ed87defd5bea27c6e3b7bf9520b.json`.

SHA-256: `98a5b42c88a75e047ebea7dfe66d889ea4401ed87defd5bea27c6e3b7bf9520b`.

The exporter fingerprints the detail source and readable design document alongside the existing pinned inputs. Generation checks proposed prop bounds against their rooms and verifies work-audio state references. Source review also moved waiting sockets out of counter volumes, aligned counter depth with the frontage frame, put clamps above the bench and mounted job boards above floor level. These checks do not establish collision-free circulation or visual acceptance.

## Verification And Remaining Work

Live OSM full-way reads matched all retained coordinates and versions: Freeman 7, Market 11, Griffith 9, Willoughby 11 and Kadara 8. Names/connectivity are map-derived; contemporary facade imagery, surveyed parcels, grades and private interiors remain unavailable. All fit-outs and occupants are fictional design, not verified tenants.

The offline exporter completed and the resulting JSON/hash were inspected. No tests, lint, typechecks, app builds, server launches or editor actions ran. The editor recovery hold is unchanged. Proposed deep rooms require replacement geometry; connecting corridors, exits, parcel overlap, collision, NPC controllers, licensed audio, cultural review and rendered acceptance remain incomplete. No native map save, production renderer registration, mobile runtime change, R2 delivery or release occurred.

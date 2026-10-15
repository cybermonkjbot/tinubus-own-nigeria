# Detailing & Polish Manifest: Brown Street–Oshodi Road approaches

October 11, 2026. Secondary art-direction handoff for Sector 02.

## Primary input and scope lock

Primary input: [Sector 02 layout and Developer Asset Checklist](sector-02-oshodi-market-approaches.md), with [its evidence register](sector-02-sources.md). This manifest is an authored detailing proposal, not a contemporary condition survey or placed scene.

Work along the existing Oshodi Road → MacCarthy Lane → Brown Street → Oredeyin Street → Oshodi Road sequence, with the primary Araromi and Banjoko connections intact. Junction node IDs below identify review areas, not prop coordinates. Final placements require approved parent meshes and local attachment transforms; geographic coordinates are not engine coordinates.

Do not move buildings, widen roads, change terrain, create drainage, add poles or generators, modify navigation, or turn decorative planks into new crossings. No approved building catalogue or final frontage coordinates exist in the primary brief. Every placement therefore remains conditional until its host is supplied. Missing hosts mean omit the detail, not invent the infrastructure.

Earlier secondary-detailing kits remain untouched. Reuse compatible existing assets after checking their manifests; the waterfront kit is not evidence for placing waterfront props in this sector.

## Placement anchors and priority

| Anchor | Primary reference | Detailing emphasis | Protected space |
| --- | --- | --- | --- |
| A01 | Oshodi Road/MacCarthy, node 1079849258 | Arrival-side paper layering, contact dust, sparse curb litter | Sector 01 interface, crossings, vehicle visibility |
| A02 | MacCarthy/Araromi, node 1079849303 | Maintained frontage beside an older finish; drain-edge detail where a drain exists | Cross-route, doors, utility access |
| A03 | MacCarthy/Banjoko, node 4395533076 | Handling marks, repaired timber, tied cable detail on approved supports | Delivery swept volumes and public passage |
| A04 | Brown/Oredeyin, node 1079849221 | Covered-delivery vignette supporting primary event S02-E01 | Receiving reservation, shelter and event recovery positions |
| A05 | Oredeyin/Oshodi Road, node 1079849462 | Return-side sign wear, cleaned threshold, restrained wrapper catchments | Junction sightlines and continuous return route |

First pass: physically grounded weathering and clean/used contrast. Second: readable signage and four small story clusters. Third: secondary scatter and state-driven VFX. Do not distribute all pillars uniformly; a swept entrance, repainted shutter and repaired roof are as important as deterioration.

## 1. Decal placements

All wording below is proposed artwork. Real campaign identities, portraits, church names, obituary subjects and Gala packaging require reference and rights clearance. Store fictional identity metadata in production records, not explanatory labels on the artwork.

| ID | Host and location condition | Artwork and layering | Material treatment |
| --- | --- | --- | --- |
| D01 | Approved public wall near A01; bridge pillar only if supplied by the primary | Three generations of fictional campaign sheets; lower corners missing, newer strip over an older portrait fragment | Uneven paste halo, faded ink, exposed paper fibres; slight curled-edge geometry only near camera |
| D02 | Existing unfinished fence or masonry panel, outside an active sign/door | `THIS HOUSE IS NOT FOR SALE - BEWARE OF 419`; irregular hand spacing across two or three lines | Spray overshoot follows wall relief; interrupted letters at mortar recesses; retain readable key words |
| D03 | Approved blank wall near A02 or A05 | `POST NO BILL`, with a few surviving paste patches nearby, not posters over every letter | Chalky, partly abraded paint; no invented demolition marks |
| D04 | ARC-02-02 approved shop sign face | Hand-painted `PROVISIONS` or `PHONE REPAIRS`, only where the approved frontage use matches | Sun-exposed face faded; sheltered lower edge darker; repaired lettering retains legibility |
| D05 | Existing approved banner anchors, not a new overhead frame | Fictional church-crusade artwork overlapping one older banner remnant | Attachment holes and edge wear; no denomination inferred from the street; no unrelated current dates |
| D06 | Approved notice surface at a quieter frontage | Torn fictional obituary notice with a remaining ornamental border and partial text | Rain bleed below torn edge; no real portrait or invented real death notice |
| D07 | INF-02-01 wall directly beside a supplied exhaust outlet | Dense soot core and directional plume footprint; secondary oily deposit near serviced equipment | Restrict projection to the receiving wall; distinguish matte soot from glossy grease |
| D08 | Approved threshold, cart receiving edge or gate contact zone | Scrapes, crate drag marks, cleaned strip and older paint repair | Match actual contact direction; never paint a crack that implies unmodelled structural failure |

Use host-local receivers to prevent decals projecting through walls, onto people or across a door opening. Paper order must read as a history, not repeated random stamps. Keep private-window sightlines and traffic signs clear. Retain one readable hero text surface per story cluster; supporting layers may fragment.

## 2. Vertex painting and PBR notes

These channel assignments and numerical ranges are authoring proposals, not changes to the existing material contract. Confirm the actual material channels before using them; if occupied, supply separate masks rather than overwrite their meaning.

Suggested masks: R = laterite dust, G = persistent damp/algae, B = material-specific deterioration, A = contact grime. Keep transient wetness separate. Generator soot uses its own localized mask/decal, not a zone-wide dirt multiplier.

| Surface | Paint/mask direction | PBR response and exclusions |
| --- | --- | --- |
| Lower building walls | Requested dust band concentrated in the lower 0.5 m above each local ground contact; irregular feathered upper boundary, sheltered crevice buildup and rain-washed interruptions | Warm red-brown tint mixed with the underlying finish; proposed dry roughness 0.75–0.95. No continuous ruler-straight stripe across steps or raised thresholds |
| Approved vehicle tires | Dust in tread recesses and lower sidewalls; rubbed contact regions less powdery | Preserve rubber identity; do not recolour the entire wheel or alter vehicle materials owned by another pass |
| Existing foliage | Light deposits on lower, road-exposed leaves and stems only | Preserve leaf variation and translucency; rain reduces exposed dust. No red tint over the whole canopy |
| GEO-02-03 open-drain concrete | Damp at retained-water contact and shaded joints; algae in persistent damp pockets, darker streaks beneath seep paths | Proposed dry concrete roughness 0.65–0.90 and wet film 0.25–0.50. Waterline follows supplied drain state, not an arbitrary horizontal puddle |
| Existing shaded concrete mass | Local green growth and black staining beneath leaking joints and sheltered runoff routes | Shade alone is not enough: require a moisture pathway. Preserve maintained and exposed dry patches; do not invent a brutalist building |
| INF-02-01 exhaust wall/casing | Soot strongest at confirmed outlet; grease near service contact and joints | Matte soot with distinct lower-roughness oily patches. Residue may remain when off; heat VFX must not |
| ARC-02-03 corrugated roof/gate | Severe rust on selected older sheets, cut edges, fasteners and water-catching seams; fresher replacement sheet alongside | Oxide is dielectric, not fully metallic orange. Exposed sound metal may remain metallic; painted surfaces keep a paint layer. Preserve roof pitch and silhouette |
| Timber crossing/counter/cart | Polished contact strip, edge splinters, repaired nail positions and dark damp end grain | Reuse existing geometry; no structural breakage, altered cart pivots or hazardous crossing clearance |

Broad vertex fields cannot resolve individual mold streaks or print: combine them with appropriately scaled texture masks and decals. Avoid subdividing primary buildings solely for stains. Keep rain, standing water, algae history and dust accumulation independent enough that a wet surface does not instantly grow algae or lose all persistent dirt.

Brown Street's present paving remains unresolved in the primary brief. Do not replace it with laterite terrain. Dust is a surface deposit; heat shimmer attaches only to an approved hot surface of the actual supplied material.

## 3. Scatter/clutter prop list

Counts are initial art-direction limits per approved cluster, not citywide density or measured conditions. Use deterministic host-based placement and inspect contact; do not scatter uniformly across roads.

| ID | Prop variants | Initial placement recipe | Ownership/clearance |
| --- | --- | --- | --- |
| P01 | Crushed clear pure-water sachets: flat, folded, partially inflated | 3–7 in one drain/curb catchment, 0–2 near a frontage; small reflective folds rather than large transparent sheets | Fit actual ground/drain bed; no default collision or invented water surface |
| P02 | Crumpled sausage-roll wrappers; Gala artwork conditional on clearance | 1–3 mixed into an approved transport-side litter catchment | Use unbranded art until cleared; original UV artwork, no copied photo texture |
| P03 | Black nylon bags: tied, collapsed, snagged | 1–2 beside an approved waste point or existing obstruction | Do not place waste at every frontage or add a dump to justify flies |
| P04 | Irregular heavy roof stones | 2–4 on an approved older zinc-roof detail slot | Primary must approve support and positioning; retain roof silhouette and drains, omit without approval |
| P05 | Worn tires: laid flat, upright against wall, existing seat arrangement | 1–2 in a recess; match the primary's approved seating/barricade use | No new traffic barrier, default navigation blockage or unsupported collision change |
| P06 | Worn wooden gutter-cover boards with end wear | Dress only an existing primary crossing/support span; polished walking strip and dark ends | Do not add a crossing, cover a drain wholesale or change walkable collision |
| P07 | Crate fragments, basket rim repair, short timber offcuts | 2–5 at A03/A04 receiving-slot margins | Reuse PROP-02-01/02; remain outside reserved delivery and pedestrian space |
| P08 | Tarpaulin folds, clips, tied cord and one patched corner | On existing PROP-02-03 attachments at A04 | Cloth ends cannot cross doors or route clearances; no independent shelter geometry |
| P09 | Electrical cable bundles and secondary drops | Follow INF-02-02 approved poles, building endpoints and route envelope | Preserve utility ownership and existing supply state; no invented pole, live connection or illegal tap |

### Wire-web treatment

Make the approved network visually irregular through unequal sag, parallel runs of different diameters, slack service loops and restrained junction bundles. Splines terminate at actual supplied fittings; attachment tangents follow their support orientation. Keep conductors above the primary clearance envelope, out of branches and off unsupported walls.

An unauthorized connection is a specific primary-approved story state, not something inferred from tangled wiring. If supplied, dress its approved branch with a visibly distinct tie/bundle arrangement; do not simulate electrical legality or add a new connection. Decorative geometry must not overwrite utilities or introduce climbable routes.

### Scatter distribution

Prefer obstruction pockets, existing collection points and low-energy recesses. Leave swept door approaches, stocked counters and maintained frontages visibly cleaner. Use ground-normal fitting without changing terrain. Wind variants remain within their local bounds; streaming cannot duplicate litter or send it through walls. Small fragments are decorative unless the primary already supplies an interaction contract.

## 4. Localized micro-VFX

Niagara briefs only; reuse existing weather, utilities and actor state. No new authoritative systems, per-prop ticking actors or globally enabled effects are introduced by this manifest.

| ID | Required host/state | Effect brief | Stop/omit condition |
| --- | --- | --- | --- |
| FX01 | Existing exposed organic waste point | Small intermittent fly loops concentrated just above waste; limited nearby audible buzz if existing audio supports it | No waste host, covered/removed waste, or distance cull; nylon litter alone does not justify flies |
| FX02 | Supplied operating window-unit AC and drain outlet | Irregular single drops falling from the actual outlet; tiny impact darkening on existing receiving surface | AC off, missing outlet/state, incompatible destination; do not create puddle depth or traversal effects |
| FX03 | Approved hot asphalt patch or operating generator casing | Low-amplitude localized heat refraction with a soft edge; generator volume follows its casing/exhaust region | Cool/off state, unsuitable surface, missing temperature proxy, or rain state that removes the approved heat condition |
| FX04 | Primary rain plus approved roof/awning drip edge | Nonuniform drips at actual edge low points; small existing-surface impacts | Rain/residual-drip state ended, unsupported edge, or no valid receiver |

Keep refraction away from sign readability and interaction targets. Use bounded shared emitters, distance/scalability culling and low-overdraw particles. Exact particle caps and distances require target profiling; none are claimed measured here. Effects can be absent at low quality without changing gameplay or primary state.

## 5. Micro-storytelling vignettes

Fictional production briefs. Each uses existing approved frontage/prop slots; if the required use is absent, retain the vignette unplaced rather than relocate architecture.

### V01 — The delivery that stayed dry

**Anchor:** A04; companion detail to primary event S02-E01. **Hosts:** receiving frontage, handcart, tarp attachments and covered recess already approved by the primary.

A patched tarpaulin has a darker fold where it was rolled while damp. Two clips differ in finish. A repaired basket rim and a worn crate sit inside the receiving space, leaving a deliberate clear strip to the doorway. Old handcart scuffs stop at the sheltered edge; outside, dust is interrupted by rain streaks.

The story is a practiced solution to a delivery problem. Dress the primary's before/after states without inventing spilled stock, rewards or new unloading actions. Clear the visual receiving slot when its reservation requires it.

### V02 — The sign kept, the shutter repaired

**Anchor:** A02. **Hosts:** approved shop frontage and shutter matching the supplied use.

The sign's original brush strokes remain, but one repaired letter is darker. A newer shutter patch contrasts with worn lower paint. The threshold has a swept centre and dust in its corners; a repaired container waits inside the approved recess. Nearby old paste marks remain on the public wall.

Maintenance and continued use are the narrative. Do not imply closure from faded paint, add an unsupported business identity or obstruct the opening.

### V03 — The generator's service corner

**Anchor:** an actual INF-02-01 installation, not an arbitrary junction. **Hosts:** supplied generator, exhaust, receiving wall and approved service recess.

A black plume stain identifies the exhaust direction. A smaller oil mark sits beside the existing maintenance contact; the gate edge shows repeated handling, not blanket corrosion. A repaired older roof sheet may contrast with a replacement where the primary supplies both. Service access stays clear.

The residue records use even when the generator is silent. Heat shimmer and operating audio follow primary utility state. No fuel container, electrical supply, exposed wiring hazard or fire event is added.

### V04 — The kiosk awaiting reuse

**Anchor:** A03 or A05 only if the primary supplies an unused kiosk; otherwise unplaced. **Hosts:** existing kiosk, sign panel and frontage recess.

A faded sign face has mounting holes where a newer panel was removed. Inside the recess, two short offcuts and a collapsed nylon bag sit behind an existing loose crate. One paper remnant has been peeled cleanly from the shutter, leaving a brighter rectangle. Outside, the walking strip remains unobstructed.

Use suggests a former fit-out, not a verified closure, eviction or crime. Do not create a new kiosk, infer abandonment from appearance alone or spawn occupants.

## 6. Primary-model handoff and review

For each accepted detail, record: detail ID; primary host ID and revision; slot/local transform; source status; artwork/provenance approval; deterministic seed; material-mask mapping; receiver bounds; route/utility exclusions; required primary state; LOD/cull policy; reviewer and acceptance state. Keep exact world transform null until primary registration and host placement are approved.

Review gates:

- Host integrity: compare primary transforms, silhouettes, doors, road graph, terrain, drain geometry and collision; this pass must leave them unchanged.
- Physical cause: every soot plume has an exhaust, damp streak a moisture path, cable an endpoint, roof stone approved support, drip a receiver and fly emitter an organic-waste host.
- Readability and contrast: text reads at intended distance; repaired and swept areas balance weathered ones; no uniform red band, algae coat, rust sheet or litter carpet.
- State integrity: effects consume existing state; decorative props do not create access, inventory, power, route reservations or gameplay outcomes.
- Visual review: use the existing primary camera/viewport and matched dry/wet and day/night states when available. Inspect at player height and at junction sightlines; native placement and visual acceptance remain separate from document review.

Delivery status: manifest authored from local primary documents. No scene, mesh, material, runtime data or renderer registration changed. Native placement, rendered inspection, cultural/rights clearance and performance acceptance remain pending. No tests, builds, packaging, import or deployment performed.

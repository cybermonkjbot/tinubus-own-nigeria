# Sector 06 — Baderin, Alhaji Mukandasi and Makinde

October 11, 2026. Conditional research and production blueprint; no native content created.

## 1. Direction and identity

We leave Sector 05 at the southwestern end of Ogunoloko, follow Baderin to Alhaji Mukandasi, and turn into Makinde. A short unnamed connector closes the first circuit; a second circuit extends south through Agbelekale and Amusa. These are source-graph connections, not yet admitted driving or walking routes. [K01]

The sector's visual premise is **paving, frontage and usable passage**. A November 1, 2021 publisher-labelled Baderin Link video shows a block-paved street beside frontages and a drainage edge. A separate June 25, 2026 Mukandasi post shows uneven block paving, exposed/missing sections, painted facades and street-side activity. These are separately captioned, unregistered views: they are not an established before-and-after pair or proof that the same work failed. [K02, K03]

Our proposed play experience makes everyday access consequential. People carry goods, negotiate stopping space, meet deadlines and continue around a short interruption. Do not make damage, shouting or congestion universal scenery. The contrast is between through movement, a working frontage and a small inner block—not a fabricated jump to an Island skyline.

Evidence vocabulary: **mapped** = OSM geometry/tag; **observed/reported** = bounded imagery/publication evidence; **proposed** = our simulation or art direction; **unresolved** = withheld from final placement. All simulation numbers below are proposed. See [sources](sector-06-sources.md).

## 2. Spatial layout and route choices

The fresh query rectangle is longitude 3.3280–3.3360 E, latitude 6.5540–6.5605 N. Fourteen complete road ways are retained, including ways extending outside that rectangle. No clipping, runtime ownership boundary or complete neighbourhood inventory is implied. [K01]

| Control point | Longitude E | Latitude N | OSM node |
| --- | --- | --- | --- |
| Ogunoloko / Baderin / two unnamed branches; S05 interface | 3.3330860 | 6.5590862 | 4395533108 |
| Baderin / Alhaji Mukandasi | 3.3317891 | 6.5586070 | 1073317441 |
| Alhaji Mukandasi / Makinde | 3.3320306 | 6.5580125 | 1073319531 |
| Makinde / unnamed return link 535191773 | 3.3333138 | 6.5584963 | 4395533109 |
| Makinde / unnamed southern link 93156426 | 3.3342152 | 6.5588374 | 1079925444 |
| Southern link / Agbelekale | 3.3344395 | 6.5582372 | 1079925435 |
| Agbelekale / Amusa | 3.3316615 | 6.5571824 | 1079925447 |
| Amusa / Makinde | 3.3314198 | 6.5577577 | 1073318247 |
| Amusa / Adesanya; next southern discovery lead | 3.3318778 | 6.5566101 | 1073317008 |

These are map coordinates, not survey control. [K01]

### Primary circuit: 446.0 m

`Baderin 535191771 → Alhaji Mukandasi 589854946 → Makinde subchain 589854945 → unnamed 535191773 → Baderin`.

The four legs are approximately 152.9, 71.3, 151.6 and 70.3 m. Total length is calculated before rounding, so rounded legs differ from the rounded total by 0.1 m. All exact node sequences appear in [connections](sector-06-connections.json). The anonymous return link is not renamed Baderin Link merely because that phrase appears in K03. [K01, K03]

### Southern circuit: 800.4 m

`Makinde → unnamed 93156426 → Agbelekale → Amusa → Makinde`.

This uses Makinde from node 1073319531 to 1079925444, the northern subchain of 93156426, all of Agbelekale 93156422, and short Amusa/Makinde subchains back to the start. It is a candidate distribution route, not a licence to divert through-traffic. Agbelekale's `lanes=2` tag is retained but does not establish lane widths, markings or current passability. [K01]

The retained network has 23 shared-node pairs at 14 unique nodes: three same-road joins and twenty cross-street pairs. Preserve four source features shared with S05: 496763689, 535191771, 535191773 and 441859117. Geometry, node order, node versions and way versions match. One future owner must hold their collision, reservations, persistent NPCs and audio sources.

No route here is claimed as a bypass of Sector 04's bridge. The primary circuit can avoid only the selected Makinde subchain after the alternate's access is verified.

## 3. Footprints, terrain and coordinate contract

Forty complete OSM building rings have representative points inside the primary circuit polygon. Selection uses the arithmetic mean of each ring's non-closing vertices, not a parcel centroid or whole-footprint containment rule. The polygon follows the circuit centreline; it is not a cadastral boundary. All selected ways have `building=yes`, version 1 and no height/use tags. Thirty-nine were edited April 23, 2024 and one April 24, 2024; neither date is a building age. [K01]

Do not infer shops, flats, tenancy, roof material or floor count from these rings. The distant multi-storey building visible in K03 is a reference-image observation, not an identified sampled footprint. Workflow: preserve ring → distinguish roof/wall outline → register contemporary camera views → measure envelope/height → establish entrance/access → author exterior details. No exact real business is assigned to a generic footprint.

No surveyed topographic section, kerb level, threshold elevation, paving thickness, drain invert or hydraulic outlet is available. The returned extract contains no `waterway` feature, which is not evidence that drainage is absent. K03 visibly shows an edge/open channel but does not register it to the map. Never fill this gap by tracing a drainage network from road names. [K01, K03]

Nine fresh nearest-neighbour SRTM samples at the listed control coordinates range from 22 to 29 m in the source's EGM96 vertical datum. The provider identifies SRTM version 3, with nominal one-arc-second spacing (about 30 m); USGS dates the radar mission to February 11–22, 2000. This is coarse historical terrain context, not a surveyed present-day street surface. Do not interpret the difference between two samples as road gradient, a drain direction or a flood depth. [K05]

The [terrain-context record](sector-06-terrain-context.json) keeps raster elevations separate from road geometry. It explicitly withholds collision, road-grade and flood-calibration use. Knowing the raster datum does not resolve our engine datum or canonical registration.

### Required reference/survey package

1. At each primary-circuit corner: registered views along every approach, clear public passage, street-name evidence and turning envelope.
2. At the candidate Makinde inspection area: level profile, accessible refuge, traffic clearance and the exact extent of any surface defect.
3. On Alhaji Mukandasi: current images sufficient to locate K02's reference area; distinguish intact pavers, displaced/missing pieces, deposited material and repairs.
4. On the southern circuit: Agbelekale lane/width evidence, Amusa turning geometry and any pedestrian discontinuities.
5. Drainage: opening/cover inventory, bed levels, connections and downstream constraints sufficient to model runoff. A regional DEM cannot certify these details.

Evidence remains WGS84 `[longitude,latitude]`, without altitude. Proposed horizontal projection: EPSG:32631; shared research origin longitude 3.3510 E / latitude 6.5560 N. Projected offsets have not been calculated. Research source metres are `(NorthDelta,UpDelta,EastDelta)` and the existing adapter remains `UE(X,Y,Z)=100*source(Z,X,Y)`. This yields research centimetres `(EastDelta,NorthDelta,UpDelta)`. Vertical datum and canonical-world registration remain null. No degrees are fed to the metre adapter and no existing map is moved.

## 4. Paving, facades and architectural assets

**Observed reference limits:** K02's inspected thumbnail and sampled video frames show block patterns with uneven/missing portions, a dark compact van, yellow/orange vehicles, painted facades, a metal roof, railings, umbrellas and street-side goods/containers. K03's sampled street view shows comparatively regular block paving, an open drainage edge, small frontage activity and a taller building at the far end. Vehicle makes, exact uses, structures and dimensions remain unverified. [K02, K03]

**Proposed construction system:** author paving as a substrate plus replaceable surface modules, not a single noisy ground material.

| Layer | Render treatment | Physical/placement rule |
| --- | --- | --- |
| Intact paving | Repeating bond with subtle per-piece variation, joint roughness and correct texture scale | Bond and dimensions sampled from registered evidence; no arbitrary bump field |
| Settled or missing section | Separate displacement/missing-piece geometry and exposed bed material | Measured relief drives wheel and foot contact; a normal map cannot create a collision hole |
| Repaired section | Bounded patch mask; replacement pieces can differ in weathering | Repair position/history must be evidenced or clearly part of the fictional event layer |
| Road-edge/drain interface | Covers, gaps and thresholds as distinct meshes | Solid covers and openings agree with navigation; do not assume bearing capacity |
| Painted frontage | Independent coating, substrate, runoff and maintenance masks | No universal peeling plaster; retain clean paint and maintained surfaces |
| Canopy/umbrella | Cloth thickness, seams, mild wind response and wetting | Attachment and occupied space need approved locations |
| Wiring/roof | Measured pole attachments, roof pitch and overlap | No random tangles or rust on every roof |

Procedural wear responds to observed defects, exposure and contact. Do not put red laterite everywhere, render every stain as diesel soot, or use grease/rainbows without an appropriate source. Generators need verified equipment positions; no fenced generator yard is invented. Political portraits and publisher overlays in K02 are not street posters. Do not recreate them as environmental decals.

Colonial, brutalist, glass-tower and luxury-estate assets remain outside this slice without specific location evidence. Exact MTN/Glo signs, real shop names, vehicle logos and packaging require date/location/rights review. The sector's material contrast already comes from paving, repairs, paint, roofs and active frontages.

Proposed visual detail bands: individual paving/threshold relief in the nearest 15 m; consolidated surface and frontage modules to about 50 m; silhouette-preserving exterior massing beyond, reviewed by screen size. Actual collision and public/private access never simplify with visual LOD. No performance result is claimed.

## 5. Traffic, pedestrians and street commerce

**Proposed network behaviour:** every vehicle has a destination, admissible route and stopping reservation before spawning. Baderin and Mukandasi serve local access; Makinde links the circuits. Network class alone supplies neither priority rules nor free capacity. Surveyed width, policy date, vehicle dimensions, stop eligibility, turning clearance and queue storage are required per directed link.

- Reserve both the junction envelope and downstream storage before entry. An occupied exit backs up the upstream link; it does not force vehicles through pedestrians.
- Use per-vehicle wheelbase, steering limits, mirrors, body overhang and load envelopes. Low-speed wheel contact responds to measured paving relief; do not add a random chassis-shake effect labelled realism.
- A bottleneck changes capacity through actual geometry and demand. Initial design target for the inspection event is one controlled movement at a time, with a 15–25 s release window, subject to measured clearance. This is not a claim about local traffic regulation.
- Route queues around the primary circuit only if every link is eligible and has storage. Do not turn residential frontages into an unlimited diversion lane. Abort the event before either junction gridlocks.
- Danfo requires model/livery/route/stop references before admission. The videos' visible yellow/orange vehicles do not establish a Volkswagen fleet, scheduled routes or a motor park. Do not invent destination calls.
- Keke and okada remain separate optional vehicle classes with dated, route-specific admission. Do not infer current legality from a video, make every keke green, or allow motorcycles to use every pedestrian gap.
- Hawkers require a verified activity location, finite inventory, a refuge and a reserved return path. Proposed lane-entry condition: vehicle speed below 0.5 m/s with sufficient sightline/clearance. Close the interaction when traffic moves. Stock and money persist; camera streaming cannot manufacture sales.
- Gala, plantain chips and water remain requested product candidates until the relevant product form, local placement and branding are evidenced. Street-side bundles visible in footage are not automatically rubbish, food or a named vendor's stock.

Pedestrian agents choose routes by passability, purpose and mobility needs. They do not use a private compound as an emergency shortcut unless that access is specifically admitted. Waiting, returning later and refusing a transaction are valid outcomes.

## 6. NPC and spatial-audio blueprint

**Proposed roles:** residents, traders, drivers, delivery workers, visitors and a temporary inspection crew. Roles are activities, not stereotypes or demographic factions. Market negotiation depends on stock, urgency, trust and the participant's willingness; aggression is not attached to gender. Tech workers may travel here without a fabricated tech cluster. Agbero/dues-collection nodes remain absent until a motor-park institution and its actual behaviour are established.

Priority tree: `immediate safety → valid access → committed task → consented interaction → optional trade/social time → rest`. Shared blackboard fields include destination, load dimensions, inventory, interaction partner, waiting deadline, safe refuge and route eligibility. A character can decline help; the player cannot commandeer a person by entering an interaction radius.

Proposed distances: trade 1.5–3 m, passage negotiation 2–4 m, crew warning range determined by actual visibility and stopping clearance. Do not play a destination call across the sector simply because a character is classified as a driver.

The carried-forward British Council lexical reference supports the narrow meanings of “How far?”, “Abeg” and “No wahala”. Proposed use is greeting, request and acknowledgement respectively, subject to fluent local review and recording consent. No accent, intonation or universal speaker behaviour is established by the written guide. [K04]

Audio sources follow world objects: tyre contact changes with paving state; moving engines and horns use relative-velocity Doppler, while stopped queues remain spatially stable. Frontage conversations and handling sounds are localized, with walls/corners occluding direct sound. The sampled footage was reviewed visually; no field-audio dataset or transcript has been acquired.

Generator audio follows a verified unit's start/load/shutdown state and its own power supply; outages do not synchronize every engine. Commission or license shop music, including any Afrobeats/Fuji references, then review bass transmission separately from speech clarity. Keep off-screen ambience tied to plausible sources instead of an unbroken loud-city loop. Rain on a metal roof and on a canopy use different emitters only after their surfaces are established.

## 7. Weather, light and surface response

**Proposed, uncalibrated settings:** distinguish aerosol haze, humidity, local dust, wetness, stored water and moving runoff. A Harmattan candidate reduces distant contrast and shifts sunlight with atmospheric scattering, not a permanent orange grade. Local dust requires loose dry material and an emitter; intact wet paving cannot keep emitting dust.

Rain darkens exposed surfaces, produces impacts and changes shelter choices before it changes flood navigation. Paving joints, intact faces and exposed bed material have separate wetness/drying responses. Do not infer permeability from their appearance alone. Assigning engineering values requires material and drainage evidence.

Water storage follows rainfall/inflow minus infiltration/discharge/outflow in measured basins. The loop centreline polygon is not a catchment. No global water plane passes through the block or automatically floods a house. Surface depressions and threshold crossings need measured elevations before standing water can alter navigation. No flood-depth or rainfall calibration is derived from K02/K03.

Replanning considers depth, flow, visibility, hidden edges and mobility requirements. Reflective puddles do not automatically close a street; visibly dry surfaces cannot conceal an active flood collider. Keep storm intensity, visibility and water state separate so a cancelled inspection event does not alter unrelated weather.

## 8. Developer Asset Checklist

Each item is unbuilt. Source availability, reference registration, placement approval, native integration and acceptance must be tracked separately.

### 3D meshes and architecture

- [ ] S06-R01 — fourteen road source ways and exact circuit subchains; width/access/sections pending.
- [ ] S06-R02 — individual paver, joint, exposed-bed and bounded repair families; register K02/K03 before assignment.
- [ ] S06-F01 — forty primary-block source rings; envelope, height, roof and entrance verification per ring.
- [ ] S06-F02 — frontage threshold, railings, canopy, shutter and drainage-edge kit from bounded visual references.
- [ ] S06-V01 — vehicle-specific minibus/van and optional keke/okada models; dimensions, markings and routes pending.
- [ ] S06-P01 — portable straightedge, measuring staff, low temporary markers and crew kit for the fictional inspection.
- [ ] S06-P02 — vendor display/containers and movable street furniture; exact goods, occupancy and rights pending.

### Material shaders and textures

- [ ] S06-M01 — paving bond, edge rounding, joint roughness and per-piece variation at measured scale.
- [ ] S06-M02 — substrate/exposed-bed, settled-piece and repaired-patch states with matching physical geometry.
- [ ] S06-M03 — maintained/weathered paint, roof and canopy materials with causally placed wear.
- [ ] S06-M04 — independent dust, exposure, wetness, drying, water depth and flow inputs.
- [ ] S06-M05 — stable near/far paving transitions; no collision or navigation changes with LOD.
- [ ] S06-X01 — source/artwork/voice/music rights ledger; no publisher graphics or real portraits reused.

### Dynamic systems and spawners

- [ ] S06-G01 — four shared S05 way owners with persistent traffic, reservation and audio state.
- [ ] S06-T01 — directed link eligibility, destination-driven demand and downstream queue storage.
- [ ] S06-N01 — mobility-aware passage, finite stock and consented negotiation with locally reviewed dialogue.
- [ ] S06-A01 — paving-aware tyre sounds, relative-motion horns, object occlusion and licensed shop audio.
- [ ] S06-W01 — surveyed surface/drainage relationships and independent wetness/flood navigation states.
- [ ] S06-W02 — regional raster provenance and local survey reconciliation; nine coarse samples retained, no road draping approved.
- [ ] S06-E01 — bounded inspection window, conditional alternate route and cancellation/clearance behaviour.
- [ ] S06-Q01 — separate geometry, imagery, asset, native, visual and playable acceptance records.

## 9. Micro-narrative S06-E01 — a short paving inspection

**Fictional event, not a reenactment or allegation about a real contractor.** Candidate area: the Makinde subchain between Mukandasi node 1073319531 and unnamed-link node 4395533109. Exact staging remains unplaced until public access, geometry and crew authority within the fiction are defined. The event does not assert that K02 was filmed here.

An inspection crew checks a suspected settled patch using a portable straightedge. Temporary markers occupy a small, pre-reserved part of the passage while one worker measures and another manages movement. A delivery driver chooses whether to wait or use the northern circuit; a trader delays a handover rather than abandon stock. The player can take a clear route, wait for release, or ask when passage will reopen. The task is inspection, not instantaneous reconstruction of the whole road.

Candidate alternate between the same two nodes: **Alhaji Mukandasi → Baderin → unnamed 535191773**, approximately **294.4 m**, versus **151.6 m** on the affected Makinde subchain. That is a source-graph alternative only. Before offering it, verify all links, access, turning envelopes and storage; if any fails, offer a bounded wait or rescheduled trip instead. This alternate does not avoid Sector 04's bridge. [K01]

State sequence: `reserve work/refuge/clearance → place markers → measure during controlled gap → record finding → withdraw tools → clear markers → release`. Proposed active window 60–90 s, with a 120 s hard cap. Cancel before activation if the crew, refuge or alternate safety conditions are unavailable. During activity, emergency demand, unsafe rain/water, loss of passage or excessive queue length triggers tool withdrawal and clearance, not teleportation.

Persist crew IDs, phase, timer, reservations and any player commitment across streaming. No duplicated tool rewards or repeated payment from leaving and re-entering. If the player is helping, release them safely on cancellation. Do not require entering a drain, lifting heavy paving, paying a real institution or confronting residents.

Acceptance requires observed end-to-end passage, safe queue recovery, an honest alternate-route eligibility check, persistent state and agreement among visuals, collision and navigation. A source loop or an event state enum alone does not pass gameplay acceptance.

## 10. Next adjacency and open work

Next research corridor: **Amusa → Adesanya → Olaiya/Mafoluku approaches**, entering at node 1073317008. Freshly fetch its southern links before prescribing a route; the longer complete Adesanya way in this package is evidence context, not an already-built district.

Outstanding work includes camera registration, surveyed sections/heights, current transport permissions, drainage calibration, local consultation, rights, assets and authorized native integration. This package advances research and design only; neither renderer consumes it and no game worker was restarted or directed.

# Sector 10 — Boladale–Church Street interface

October 11, 2026. Conditional research/production blueprint. Documentation only; no assets, runtime imports or worker actions.

## 1. Place and intent

We follow northern Seinde Callisto into **Boladale Street**, turn back through **Oshodi Road**, and resolve the **Church Street** junction. This closes a real mapped connection to earlier sectors rather than teleporting into a distant district. The selected south-side streets add a quieter frontage-scale contrast through **Kunle Akinosi Street, Adewale Crescent, Sabiolegbe Street and Ganikale Close**. Names and connections are mapped evidence, not contemporary street surveys. [P01]

The visual anchor is the Boladale/Kunle Akinosi junction: two inspected photographs carry September 6–7, 2024 date-and-coordinate stamps and appear in an October 2, 2024 maintenance post. They show night work, contrasting facade panels, overhead wires and exposed/repairing road surfaces. Near the Oshodi Road return, separately inspected Commons images dated February 11, 2016 show yellow minibuses and people carrying loads. These are two historical observations, not one reconstructed moment or a confirmed present-day motor park. [P02, P03]

Our contrast here is between roadwork-scale material detail, transport frontage and residential branches. No colonial monument, 1970s brutalist block, luxury estate or glass skyline is inserted without site evidence. Larger city contrasts must emerge through geographically connected sectors, not transplanted landmarks.

Evidence labels: **mapped** source geometry/tags; **observed** image-specific detail; **reported** dated account; **proposed** our design; **unresolved** insufficient for final placement. All simulation values below are proposed. See the [source register](sector-10-sources.md).

## 2. Spatial layout and topology

Fresh query: longitude 3.3440–3.3505 E, latitude 6.5540–6.5590 N. The [network](sector-10-network.geojson) retains sixteen complete road ways, including continuations outside that rectangle. It is a selected network, not complete coverage. [P01]

| Control | Longitude E | Latitude N | OSM node |
| --- | --- | --- | --- |
| Seinde / Boladale / Church / unnamed secondary | 3.3452941 | 6.5557881 | 1079849271 |
| Boladale / inner northbound link | 3.3462837 | 6.5557557 | 4395533057 |
| Boladale / Kunle Akinosi; footprint centre | 3.3466968 | 6.5557704 | 1475182789 |
| Boladale source split / unnamed residential | 3.3482733 | 6.5557729 | 9393853880 |
| Boladale / eastern unnamed tertiary | 3.3489836 | 6.5557096 | 1079849281 |
| Eastern unnamed tertiary / Oshodi Road | 3.3490373 | 6.5564930 | 4395533063 |
| Oshodi Road source-way split | 3.3484235 | 6.5564944 | 4395533064 |
| Inner link / Oshodi Road | 3.3462864 | 6.5564731 | 4395533056 |
| Oshodi Road / Church Street | 3.3452437 | 6.5564568 | 1079849430 |
| Kunle Akinosi / Adewale Crescent | 3.3469462 | 6.5552388 | 4395533035 |

### Two connected candidate circuits

**Eastern circuit, 989.1 m:** node 1079849271 east along Boladale 93146072 to 9393853880; forward along one-way Boladale 1343514141 to 1079849281; north on unnamed 441859099 to 4395533063; west on Oshodi 5153761 to 4395533064; west on Oshodi 1547601493 to 1079849430; south on Church 1547601494 back to the starting node. [P01]

**Inner circuit, 378.9 m:** the same start east along Boladale to 4395533057; forward on one-way unnamed 441859097 to 4395533056; west on Oshodi 1547601493 to 1079849430; south on Church to the start. [P01]

The [connection record](sector-10-connections.json) stores full route node sequences, each leg's direction and horizontal length. Neither circuit violates an explicit one-way tag in the selected ways. Neither is an approved vehicle route or verified pedestrian loop: turn restrictions, usable widths, curb access, signs, sidewalk continuity and contemporary operation remain unresolved. Do not infer permission from an absent tag. Rounded leg lengths need not sum exactly to the rounded total. [P01]

Twenty-five pairwise shared-node records occur at thirteen distinct nodes. A four-way source junction contributes six pairs, not six physical intersections. Do not turn source-way splits into junction props. Unnamed ways stay unnamed. [P01]

### Shared world ownership

Ten feature/file overlap records match earlier geometry and source versions: Oshodi with S01/S02/S07/S08, Church with S03/S09, and Boladale/Seinde/unnamed secondary with S09. Assign one future runtime owner per source feature. The repeated records must not double collision, queues, NPCs, lights or sound. This package establishes no runtime owner. [P01]

Kunle Akinosi and two unnamed southern approaches reach expressway-related source ways outside this selected network. Treat those as boundary interfaces, not arbitrary vehicle spawn points or proof that tricycles/motorcycles may enter the expressway. Keep S09's unconnected southern stair approach and reverse-one-way diagnostic trace unresolved.

## 3. Topography, buildings and engine registration

The [footprint sample](sector-10-buildings.geojson) contains 29 closed source rings. Selection uses the arithmetic mean of non-closing vertices within an approximate 65 m of node 1475182789, not full-ring containment or a cadastral buffer. All have building=yes; twelve also carry Microsoft/BuildingFootprints source tags. No current business use, height, entrance, roof shape or storey count is verified. Do not attach the photographed facade or its lettering to the nearest polygon. [P01]

The two image-stamped positions lie approximately 13.0 m and 7.5 m from the mapped junction. That is a consistency check between source claims, not camera registration: bearing, lens, stamp accuracy, facade identity and relative depth remain unresolved. [P02]

Eight fresh nearest-neighbour SRTM queries yield 23,21,26,21,21,23,20,21 m. They are historical coarse raster elevations in EGM96, not road/drain measurements. A 26 m sample beside 21 m samples does not justify a procedural hump or flood basin. No local municipal contour/utility/drain survey was obtained. [P05]

```text
Evidence: WGS84 [longitude, latitude], no altitude
Proposed research projection: EPSG:32631
Shared research origin: longitude 3.3510 E, latitude 6.5560 N
Source metres: (NorthDelta, UpDelta, EastDelta)
Existing adapter: UE(X,Y,Z) = 100 * source(Z,X,Y)
Research UE centimetres: (EastDelta, NorthDelta, UpDelta)
Projected coordinates calculated: false
Engine vertical datum: null
Canonical-world registration: null
```

Before any collision or navigation production, establish the canonical transform and measure carriageway edges, curb heights, channel invert/outfall, doorway thresholds, utility supports, overhead clearance and building envelopes. Preserve the existing game geography. Degrees never enter the metre adapter. The photographs' roadside edge supports investigating drainage; it does not supply a drain network. [P02, P05]

## 4. Procedural traffic and go-slow

**Proposed architecture:** keep directed lane links, turn reservations, stopping pockets, pedestrian surfaces and delivery destinations separate. Source centreline geometry seeds research, not lane counts. Junction admission requires enough downstream storage for the entire vehicle; turning uses wheelbase, mirrors, swept envelope and measured corner geometry.

- Split demand among through-trips, admitted local drop-offs, deliveries and residents. Keep trip purpose, destination and queue identity across streaming; never respawn the same waiting vehicle at both S02 and S10 boundaries.
- For future tuning, evaluate queue occupancy every 0.25–0.5 s. Below 0.5 m/s, vehicles may enter an eligible stopped-interaction state; movement resumes only after doors, people and goods clear the envelope. These are simulation parameters, not measured local driving habits.
- Preserve one-way 1343514141 from 9393853880 to 1079849281, and 441859097 from 4395533057 to 4395533056. No contraflow shortcut for spectacle. [P01]
- Historic Commons views support yellow/black minibus appearance and panel-wear studies, not a Volkswagen-only fleet, verified routes, current stop locations or a fixed bus count. Model-specific wheels, suspension, contact hulls, doors and passenger capacity require separate vehicle references. [P03]
- Keep keke and okada disabled on unverified links until route- and date-specific access is established. Green keke is not a universal livery rule. No current restriction is asserted here.
- Hawkers need evidence of eligible trading space and local operation before placement. Proposed NPC logic: finite stock, scheduled replenishment, a reserved refuge, stop eligibility and sufficient remaining dwell time. Admit no trade if safe return is unavailable; never steer a hawker through a moving lane merely to sell.
- Gala, plantain chips and cold water remain requested product candidates, not verified stock at this junction. Branded packaging, local pricing, seller pitches and product dimensions need evidence/clearance; prototype stock is unbranded and explicitly authored.
- A queue should arise from a traceable cause—finite downstream capacity, boarding, a turn conflict or bounded delivery dwell—not a perpetual citywide congestion multiplier. Side-street residents retain access and quiet periods.

Collision uses separate body and carried-object envelopes; character movement may adapt pace and sideways clearance but cannot phase through mirrors or push people into vehicles. Fine collision and navigation do not disappear when visual LOD changes.

## 5. Materials, architecture and lighting detail

The Boladale photographs show alternating light/dark facade panels, window framing, utility supports/wires, perimeter elements and harsh night point lights. The second shows dark granular material being spread. These observations support a historical material study; material chemistry, construction age and final surface are unresolved. [P02]

OSM labels both Boladale ways unpaved, Church's longer northern section asphalt and its short southern section paving_stones. Oshodi return ways are paving_stones. Keep those tags alongside the 2024 photographs rather than declaring one source current and the other wrong. An edit timestamp is not a site inspection date. [P01, P02]

| Proposed asset family | Authoring rules | Evidence gate |
| --- | --- | --- |
| Facade panel/render kit | Separate panel colour, grout/reveal, roughness, repairs and clean variants; no uniform grime overlay | Register actual facade, dimensions and current condition |
| Road substrate/patch kit | Independent aggregate scale, patch edge, compaction and dampening; rough/light and dark-repair studies stay dated | Confirm current surface per segment; no whole-street asphalt inference |
| Roof/perimeter kit | Independent sheet/other roof candidates, wall caps, gate swing envelopes and drainage edges | Verify each building's roof/material/access; no generic roof assignment from footprint |
| Pole/wire kit | Span-specific attachments, sag and clearances; visual cables distinct from collision | Survey support positions and actual utilities; no arbitrary cable tangle |
| Minibus panels | Yellow/black reference palette, localized scratches, paint layers and repaired-panel variants | Match dated images, then obtain make/current fleet references |
| Wet surfaces | Pooled water distinct from dark wet material; roughness and reflection follow wetness and view angle | Survey local grades; no universal oily film |

No faded campaign poster, telecom billboard, political affiliation or named storefront is generated from a generic Lagos decal sheet. Exact source date, location and reuse rights are required. Generator soot follows an evidenced exhaust and receiving wall; generators behind fences are candidate assets, not observed installations here.

**Proposed LOD:** prioritize curb silhouettes, junction sightlines, doors, poles and cables near the player; simplify facade joints and hardware by projected size; merge distant repeated details only after occlusion review. Candidate bands of 0–25 m detailed, 25–75 m simplified and beyond 75 m massing are tuning starts, not engine budgets. Do not let a closed gate become traversable at distance or remove a water barrier with visual simplification.

Night lighting should preserve different pools of work/frontage/vehicle light, dark interstitial space and restrained bloom. The 2024 photographs are not lux, colour-temperature or electrical-service surveys. Headlight spill and wet reflections respond to actual scene geometry, not painted highlights. [P02]

## 6. Ambient spatial audio

**Proposed:** separate the Oshodi return's moving emitters from Boladale frontage and quieter residential interiors. Vehicle engine load, wheel/surface sound, braking and horns have independent state. Doppler follows relative motion; a stationary gridlocked horn does not sweep in pitch. Use occlusion through walls and around corners rather than one loud city loop.

Candidate shop sources may use licensed/commissioned Afrobeats or Fuji after a specific location and local sound character are verified. Door openings change high-frequency leakage; bass transmission is material-dependent. This package supplies no field recording, shop playlist, measured sound level or music licence.

Generators need evidenced positions plus start, crank, idle, load and shutdown states. Power interruption is a per-building simulation event; it must not start every generator simultaneously or imply a measured outage schedule. Street-light power, shop audio and interior lighting can have different backup arrangements.

Directional conductor calls must match verified served destinations and vehicle state. No destination is invented from city familiarity. Dialogue retains intelligibility at conversational distance, while a small emitter budget prioritizes near, relevant sources. Provide subtitle/source cues and dynamic-range options without making the ambience disappear.

## 7. NPC routines and cultural review

**Proposed priorities:** immediate safety/access → committed work/journey → transaction → social contact/rest. Residents, drivers, passengers, tradespeople, vendors and visitors have individual schedules, budgets, mobility, pace and patience. Women who trade are not uniformly aggressive; negotiation style varies by person and situation.

An eligible vendor tree is: `check stock and refuge → select stopped customer → reserve interaction position → offer → agree quantity/payment → exchange once → clear envelope → return/restock`. Refusal ends the approach; blocked space, moving traffic or insufficient dwell cancels before exchange. A passenger's tree checks departure timing before shopping. No universal coercion or harassment loop.

Candidate interaction radii: 1.5–2.5 m face-to-face trade; 3–6 m attention call if sightline and ambient sound allow; larger calls need separately designed projection. These are design distances, not ethnographic measurements.

“How far?” supports a greeting/check-in, “Abeg” a request/please, and “No wahala” reassurance/no problem under the carried language reference. Exact register, intonation, code-switching, spelling preferences and local performance remain for fluent review. Do not combine the phrases into random comic barks. [P06]

No tech-bro enclave is evidenced here. Agbero/dues-collection actors require a documented motor park, role, operating period and consultation; a 2020 repair post mentioning a union leader does not establish such a park or current practice. Do not turn occupation into ethnicity, violence or a mandatory player toll. [P04]

Local consultation, consented recordings and contemporary observation remain missing. People pictured in research are not likeness templates. The world needs waiting, friendship, careful work, rest and routine as well as urgency.

## 8. Weather, particles and water

**Proposed, uncalibrated:** use separate humidity, aerosol, wind, rain, surface moisture and dust-deposition controls. Harmattan art direction reduces distant contrast through particulate scattering and can warm low-angle sunlight; it is not a permanent orange filter or proof of the season in an undated photograph. Keep near-ground dust lifts localized to traffic/material disturbance, with settling and wind transport.

Rain has independent onset, intensity, roof impacts, runoff and residual wetness. Heavy downpour does not flood every street instantly. A local water solver needs measured storage and connected flows: volume change equals rainfall plus inflow minus outflow and infiltration. Unknown outfalls remain unknown, not silently sealed to manufacture a flood.

Use connected local water patches or shallow-water cells once terrain is admitted, not one citywide plane cutting through thresholds. Include road spray, sheltered dry zones and roof-edge drips where geometry supports them. Do not interpret all dark road material in a night repair photo as water or oil. [P02]

Navigation cost responds to water depth, flow, traction, visibility and each actor's mobility. Thresholds require design and validation; coarse SRTM cannot calibrate them. A flooded candidate route is closed only when a verified alternative or safe wait state exists. The circuit geometry is not yet an approved flood detour. [P05]

## 9. Developer Asset Checklist

All production items below remain unbuilt. Evidence preparation is not native integration or visual/gameplay acceptance.

### 3D meshes and architecture

- [ ] S10-R01 — sixteen source road ways; measured lane/curb/junction geometry; shared-feature ownership.
- [ ] S10-F01 — twenty-nine footprint envelopes with current heights, doors, roof forms and facade registration.
- [ ] S10-B01 — Boladale panel/render, window, perimeter and gate kit from registered reference.
- [ ] S10-U01 — surveyed poles, cable attachments, clearances and roadside channel profiles.
- [ ] S10-V01 — independently referenced minibus/body/door/wheel variants; keke/okada admission pending.
- [ ] S10-O01 — unbranded trade container, finite stock and handover props; exact real product references pending.

### Material shaders and textures

- [ ] S10-M01 — exposed road/repair study, current per-segment surface selection withheld.
- [ ] S10-M02 — light/dark facade panels, render, gate and roof candidates with localized wear and clean variants.
- [ ] S10-M03 — yellow/black vehicle paint and panel-specific abrasion from historical references.
- [ ] S10-M04 — independent dry, damp, rain-wet and pooled-water states; no automatic oil sheen or red-dust coating.
- [ ] S10-M05 — aerosol/deposition/traffic dust, night point-light and rain-response controls.
- [ ] S10-X01 — attribution, photographic rights, third-party markings, music, voice and likeness ledger.

### Dynamic systems and spawners

- [ ] S10-G01 — directed links and turn reservations; both candidate loops remain unapproved until access review.
- [ ] S10-T01 — downstream-storage queues, boarding/delivery dwell and cross-sector vehicle identity.
- [ ] S10-N01 — vendor stock, conversational consent, change availability, refuge reservation and cancellation.
- [ ] S10-A01 — moving traffic and directional speech; facade occlusion and conditional generator/music emitters.
- [ ] S10-W01 — measured drainage, connected water storage, wet traction and mobility-aware rerouting.
- [ ] S10-E01 — bounded change-before-departure event with atomic trade and clear-path invariant.
- [ ] S10-Q01 — separate source, asset, native, visual and playable acceptance records.

## 10. Micro-narrative S10-E01 — change before departure

**Fictional design, not a reported event.** Candidate decision anchor: Boladale/Kunle Akinosi node **1475182789**. It requires a contemporary verified stopping pocket, a lawful/eligible trading area and a public pedestrian refuge outside vehicle swept paths and private entrances. None is established by the source node or repair photographs. If unavailable, suppress the event at this anchor.

A waiting passenger agrees to buy one unbranded drink, then discovers that the seller cannot make change with the available till. A second customer waits within the reserved refuge. The vehicle is nearly ready to depart. The player may supply exact change by consent, wait for the exchange, or continue past. The passenger may cancel and board without buying. The scene is about timing, arithmetic and negotiation, not a swindle or a compulsory payment.

State sequence: `validate pocket/refuge and departure margin → reserve participants/stock → quote → discover change shortage → exact-change or cancel branch → atomic exchange/refund → clear space → release dwell reservation`. Proposed transaction window 15–30 s, hard cap 40 s, with departure winning when its safe dwell budget expires. Vehicle movement never begins while anyone remains in its conflict envelope.

The temporary second customer changes the player's avoidance route within the verified refuge; finite boarding dwell can briefly delay a following admitted vehicle. Preserve through-passage and emergency access. No queue spills into the junction, and nobody chases the departing vehicle. Rain, crowded refuge, closure or a stale vehicle reservation cancels before handover or triggers a complete refund.

Persist owner, stock count, money/change inventory, transaction state and departure reservation across streaming. Conserve goods and value; do not debit on animation start and again on completion. Acceptance requires bounded clearance, accessible passage, cancellation with no lost goods/money and consistent vehicle/person/prop collision. No price, denomination, real seller or current stopping practice is asserted. This event is not implemented or play-tested.

## 11. Next adjacent slice and readiness

Continue south from **Boladale/Kunle Akinosi node 1475182789** along **Kunle Akinosi Street 134140049** into **Ganikale Close 496763688** and **Adewale Crescent 441859090**. This deliberately deepens the branch introduced here, with a fresh search and closer residential/access evidence; it does not claim all selected branches are already built.

Remaining gates: contemporary access and surfaces, facade/camera registration, surveyed widths/heights/drainage, canonical transform, field audio and cultural consultation, asset rights, native production and visual/gameplay acceptance. The package is not consumed by either renderer.

# Lagos tree and roadside-drain references

Access date: October 14, 2026. Bounded, read-only research supporting distinct tree meshes and open-drain street-edge assets. This note is not a tree inventory, drainage survey or implementation receipt.

## Evidence boundary

Repository context: read `imagery-evidence-review.md`, including its October 11, 2026 pixel audit. Fresh primary-source reads: University of Lagos checklist records through GBIF and its publisher's EML; Wikimedia Commons photographer-uploaded file metadata and description revision IDs. A web search call returned no usable output; direct read-only PowerShell HTTPS requests succeeded.

Three original photographs were visually inspected through the T3 collaborative browser at a fitted 1280 x 800 viewport: the tree-lined Lagos view, Ibereko channel and Ikeja watercourse/bridge. Other navigations returned image-page titles but blank images or failed snapshots; those are explicitly not fresh pixel evidence. No photographs, thumbnails or screenshots were saved. No inference from a photograph establishes measured dimensions, chemical composition, tree species or present-day conditions.

## 1. Primary botanical repository

**Source:** Onuminya T, Ogundipe O, Igbari A, Kemabonta K, Suberu A (2022), *A checklist of Plants growing in the Lagos Metropolis, Lagos State, Nigeria*. Publisher: University of Lagos. DOI: `10.15468/e9qfps`.

- Dataset page: https://www.gbif.org/dataset/9154e5da-4aa0-4c45-9fe1-4f4aba4dcab5
- Fresh dataset metadata: https://api.gbif.org/v1/dataset/9154e5da-4aa0-4c45-9fe1-4f4aba4dcab5
- Fresh source-name query: https://api.gbif.org/v1/species/search?datasetKey=9154e5da-4aa0-4c45-9fe1-4f4aba4dcab5&limit=300
- Fresh publisher EML: https://ipt-nigeria.gbif.fr/eml.do?r=achecklistofplantsgrowinginthelagosmetropolis
- Publisher archive endpoint, discovered in metadata but **not downloaded**: https://ipt-nigeria.gbif.fr/archive.do?r=achecklistofplantsgrowinginthelagosmetropolis

Inspected metadata identifies a checklist, published September 13, 2022, with temporal coverage June 6-30, 2010. It describes trees growing in the Lagos metropolis. Its stated bounding rectangle is longitude 2.736-4.367, latitude 6.377-6.686; that is a dataset coverage envelope, not a coordinate for any tree. GBIF metadata reports 196 name usages and zero occurrence records. Do not interpret name-usage count as 196 species or use this checklist to geolocate roadside trees. The inspected EML supplied no methods section establishing a street-tree sampling protocol.

**Reuse:** publisher EML and GBIF metadata both declare CC BY 4.0, https://creativecommons.org/licenses/by/4.0/legalcode . Attribute the authors, title, publisher and DOI when reusing dataset information; identify adaptations. This license is for the dataset, not an entitlement to unrelated botanical photographs or textures.

### Checklist-backed candidates, not street distributions

These exact source names and source keys appeared in the dataset-filtered query. Candidate mesh shapes in the last column are **authored modeling briefs**, not botanical identifications of the photographs below. Current nomenclature and botanical geometry need separate reference review before species-specific assets are approved.

| Source scientific name | Source key | Proposed distinct silhouette |
| --- | --- | --- |
| `Terminalia catappa L.` | `200209029` | Broadleaf tree with visibly separated horizontal crown tiers; irregular mature and compact young versions. |
| `Mangifera indica L.` | `200209057` | Dense, rounded broadleaf crown; several unequal crown lobes rather than a single sphere. |
| `Azadirachta indica A.Juss.` | `200209071` | Airier, branching broadleaf crown with small-leaf clusters and readable gaps. |
| `Delonix regia (Bojer ex Hook.) Raf.` | `200208908` | Wide spreading shade crown with exposed branch structure; flowering variant held for botanical review. |
| `Cocos nucifera L.` | `200208876` | Leaning palm trunk and irregular crown of long fronds; distinct from ornamental upright palms. |
| `Roystonea regia (Kunth) O.F.Cook` | `200208878` | Upright ornamental palm, differentiated trunk/crown profile; not a rescaled coconut mesh. |
| `Elaeis guineensis Jacq.` | `200208880` | Dense palm crown and different lower-trunk profile; not interchangeable with either preceding palm. |
| `Polyalthia longifolia (Sonn.) Thwaites` | `200209016` | Narrow vertical ornamental silhouette; retain source name without silently treating it as current accepted nomenclature. |
| `Ficus thonningii Blume` | `200208978` | Large irregular broadleaf shade-tree candidate; roots and branching held for species-specific review. |

A fresh individual record, https://api.gbif.org/v1/species/200209029 , confirms `Terminalia catappa L.` has `origin=SOURCE` in this dataset. It is not a modern inventory of Freeman, Market, Griffith, Willoughby or Kadara streets. None of the candidates has verified placement, abundance, age, condition or native status at those streets in this pass. Do not turn source-listed exotic or disputed names into an unreviewed automatic palette.

## 2. Successfully inspected photographic evidence

### T1. Contrasting roadside tree forms, source-attributed Lagos

- Description page: https://commons.wikimedia.org/wiki/File:Beautiful_street_of_Lagos_with_trees.jpg
- Original inspected: https://upload.wikimedia.org/wikipedia/commons/4/48/Beautiful_street_of_Lagos_with_trees.jpg
- Commons page `77379493`, description revision `1083232767`, revision time September 9, 2025, 14:30:48 UTC. Original 3264 x 2448; SHA-1 `13a537aa9995625e4b9ad655aede1fd203ec4d72`.
- Source-declared capture January 12, 2019; uploaded March 16, 2019, 18:14:23 UTC. Author **Factual Evolution Media**, own-work declaration; **CC BY-SA 4.0**, https://creativecommons.org/licenses/by-sa/4.0/ .
- **Pixels:** large foreground trunks fork into widely spreading branches, relatively sparse fine foliage and visible canopy gaps. Behind a mesh fence is a repeated row of smaller, narrow, upright leafy trees. A billboard and elevated roadway appear beyond. The foreground canopy and narrow planted row provide genuinely different silhouettes in one frame.
- **Limits:** no species identified. The title associates the view with Lagos, but no exact street/camera coordinate was established. Do not assign this to Ebute Metta or identify the elevated roadway from appearance. Sparse foliage in this dated frame does not prove a citywide seasonal leaf-loss rule.
- **Asset implication:** make broad irregular shade trees and narrow upright ornamental trees separate meshes with different branching and leaf-card organization; do not merely change the scale or tint of one crown.

### D1. Sloped open lined channel, source-attributed Ibereko

- Description page: https://commons.wikimedia.org/wiki/File:A_drainage_at_Ibereko,_Badagry,_Lagos_State,_Nigeria.jpg
- Original inspected: https://upload.wikimedia.org/wikipedia/commons/d/d5/A_drainage_at_Ibereko%2C_Badagry%2C_Lagos_State%2C_Nigeria.jpg
- Commons page `129648613`, description revision `992081004`; original 2976 x 3968; SHA-1 `a3711565fb04de5329626d92559a1e9431b8487e`.
- Source-declared capture March 16, 2023, 15:11:20, timezone unestablished; uploaded March 16, 2023, 15:22:40 UTC. Author **Macdanpets**, own-work declaration; **CC BY-SA 4.0**.
- **Pixels:** long open channel with pale, concrete-looking sloped sides, visible panel/joint boundaries, localized cracks, a shallow-looking water stream, deposits and green growth in the invert/along margins. Dense vegetation borders the channel; large leaves and palm-like crowns appear behind.
- **Limits:** this is not evidence of a narrow sidewalk gutter or access slab. No road, entrance crossing or measured cross-section is established by this frame. Concrete-looking is a visual material description, not a specification; no tree species, water depth, water quality or flow capacity established. Source location is Ibereko/Badagry, not the Ebute Metta slice.
- **Asset implication:** maintain a separate sloped-channel family for wider drainage corridors. Do not stretch a vertical-sided roadside gutter to represent this geometry.

### D2. Larger watercourse with bridge, source-attributed Ikeja

- Description page: https://commons.wikimedia.org/wiki/File:Poor_drainage_system_with_local_bridge_in_Lagos.jpg
- Original inspected: https://upload.wikimedia.org/wikipedia/commons/2/2d/Poor_drainage_system_with_local_bridge_in_Lagos.jpg
- Commons page `129687363`, description revision `1110602712`; original 3456 x 4608; SHA-1 `068aabedfb7eacae60fe26232ad4af4fc7dfcfb3`.
- Source-declared capture March 17, 2023; uploaded March 17, 2023, 22:17:32 UTC. Author **Tunesh247**, own-work declaration; **CC BY-SA 4.0**. Description specifies Ikeja, without an exact camera position.
- **Pixels:** broad open watercourse, vegetated margins, visible floating litter, boundary walls/buildings and a raised bridge with railings at the right background. A small foreground tree has a relatively slender trunk and concentrated leafy upper crown.
- **Limits:** despite its search title, this is not a slab across a narrow sidewalk gutter. Bridge material, dimensions, public access and street identity are not established. The foreground tree is not botanically identified. Litter in this frame does not prescribe all Lagos channels as polluted.
- **Asset implication:** distinguish larger crossing structures from doorway drain slabs. Keep mature, young and pruned tree silhouettes distinct without asserting the pictured tree's species.

## 3. Narrow channel crossings: earlier inspected evidence

**D3. Makoko land-facing scene, historical observation retained from the repository audit.** Fresh metadata was retrieved today, but fresh pixel inspection failed; observations below remain attributed to the successful **October 11, 2026** inspection recorded in `imagery-evidence-review.md`.

- Description page: https://commons.wikimedia.org/wiki/File:Makoko_an_Land_(5208471669).jpg
- Original: https://upload.wikimedia.org/wikipedia/commons/c/cc/Makoko_an_Land_%285208471669%29.jpg
- Commons page `22140207`, description revision `1151026992`; 3264 x 2448; SHA-1 `9cf11b476af09981a3f795813fe81302ff6d0f3a`. Fresh metadata matches the earlier audit's identifiers.
- Source-declared capture November 2, 2010, 11:45, timezone unestablished; uploaded October 17, 2012, 16:07:37 UTC. Photographer **Heinrich-Boll-Stiftung**, via https://www.flickr.com/photos/boellstiftung/5208471669/ . **CC BY-SA 2.0**, https://creativecommons.org/licenses/by-sa/2.0/ . Flickr license review is recorded by Commons; Flickr itself was not reread in this pass.
- **Prior inspected pixels:** land-facing room/shop thresholds beside an open channel crossed by individual boards/slabs, with a raised water point and domestic containers. The earlier audit explicitly leaves exact crossing and channel materials/widths unresolved.
- **Location limit:** imported latitude 6.446658, longitude 3.416061 conflicts with the prior dossier's Makoko anchors; do not register this camera pin in the game or use it as proof of exact street placement.
- **Useful distinction:** historical support for intermittent threshold crossings over an otherwise open channel. It does **not** verify concrete slab composition, load-bearing capacity, public right-of-way or an accessible walking route.

## 4. Metadata-only leads and inspection failures

These primary photographer uploads are useful follow-up targets, not additional visual evidence. Commons declares CC BY-SA 4.0 for each. No present-day or exact-street inference is authorized from captions alone.

| Exact description page | Retrieved metadata | What happened / remaining gap |
| --- | --- | --- |
| https://commons.wikimedia.org/wiki/File:A_walkway_in_IKEJA_GRA_off_PWD_Lagos_Nigeria.jpg | Page `130717390`, revision `992819431`, author Swantata, declared capture April 13, 2023, 09:47:56; SHA-1 `c8bb64d409eecb56d53833a2a7eafe718862fe22`. | Original-image navigation succeeded; two snapshots returned a blank dark viewport. No walkway, tree or cover-slab observation made. |
| https://commons.wikimedia.org/wiki/File:Construction_of_drainage_system_in_Pako_Ikeja_Lagos_state,_Nigeria.jpg | Page `131414542`, author Tunesh247, declared capture May 1, 2023; SHA-1 `c5557539fc3352f1b3db0add1ac1a4e86f8566f6`. | Original and redirected thumbnail navigation succeeded; snapshots failed at the preview client. Geometry and construction method not inspected. |
| https://commons.wikimedia.org/wiki/File:Blocked_drainage_in_Morogbo_Badagry,_Lagos_State,_Nigeria.jpg | Page `129652541`, author Macdanpets, declared capture March 16, 2023, 09:22:41; SHA-1 `f41bac843130bee959677dbb5229518849ac6776`. | Original navigation succeeded; subsequent snapshots failed, including retry in a fresh tab. No roadside/crossing geometry accepted. |
| https://commons.wikimedia.org/wiki/File:Man_cleaning_drainage_in_Agbara,_Lagos_Badagry_Expressway.jpg | Page `129652546`, author Macdanpets, declared capture March 16, 2023, 08:23:10; SHA-1 `b639377c2a9d79bf76aad29653df23bb51cfc3cb`. | Metadata only; no original navigation or pixel inspection. Agbara attribution is not proof of a location inside Lagos State; no boundary registration attempted. |

Fresh Commons searches were `Lagos drainage`, `Lagos gutter`, `Lagos trees street`, file namespace 6, limit 8. Search results included Portuguese Lagos and unrelated lake imagery; those were excluded. Metadata query form: `https://commons.wikimedia.org/w/api.php?action=query&pageids=77379493%7C130717390%7C129648613%7C129687363%7C22140207&prop=imageinfo%7Crevisions&iiprop=url%7Cextmetadata%7Ctimestamp%7Csha1%7Csize&rvprop=ids%7Ctimestamp&format=json`. Additional page IDs in the table were fetched separately. Source-declared capture dates are not independently authenticated camera dates.

## 5. Proposed implementation handoff

The following are authored game-asset proposals, not recovered local dimensions or certified civil designs. This research does not alter any renderer, scene, street cross-section, collision or spawner.

### Trees

- Prioritize six visually non-interchangeable families: spreading shade crown, tiered broadleaf crown, dense rounded crown, narrow ornamental crown, leaning palm and upright ornamental palm. Use the checklist as a candidate pool, not evidence that every street should receive all six.
- Give each family at least two independent branch/trunk meshes before adding young/pruned variants. Rotation and scale variation alone do not solve repeated silhouettes. Generic morphotype IDs remain generic until botanical review verifies trunk, leaf and crown references.
- Vary canopy porosity and branch exposure as well as height. Keep material-driven leaf color variation subordinate to geometry. Do not use orange foliage everywhere to stand in for Harmattan dust or universal seasonal behavior.
- Placement must be parcel/streetscape-authored: soil/root-space, entrances, drain maintenance access, sightlines, wires and canopy clearance. No checklist-derived species weights, exact tree counts or mapped placements supplied.

### Open drains and sidewalk thresholds

- Provide separate open narrow-channel, intermittent-cover, fully covered, sloped larger-channel and absent-drain states. Mix by reviewed street segment; never prescribe one neglected state to an entire neighborhood.
- Proposed narrow roadside kit: straight trough, corner, junction, end/outfall, open inspection interval, entrance slab, driveway crossing and optional grate. Channel interiors must remain visible geometry, not a black strip painted onto a sidewalk.
- Prototype-only narrow-channel authoring ranges: clear width 0.35-0.8 m, depth 0.3-0.7 m. These values are **design inputs**, not measured Lagos dimensions and not a flood model. Wider sloped channels use a different mesh family and require their own reviewed sections.
- Treat a doorway slab as a crossing perpendicular to the channel, distinct from a cover slab continuing along it. Resolve sidewalk position, channel position and property threshold separately. No photograph here establishes a single universal road-drain-sidewalk order.
- Make slab dimensions follow the authored channel span and support geometry; do not scatter floating slabs as decorative stepping stones. Keep an intentionally clear walking corridor and continuous supported alternatives where the game permits public access. Missing cover variants need honest collision and route constraints, not invisible flat walkable surfaces across holes.
- Separate concrete exposure, joints/repairs, sheltered surfaces, wet waterline and localized invert deposits. Support maintained/cleared channels as well as localized litter and vegetation. Do not spawn waste uniformly or infer sewage from water color.
- Water state must reference an authored/surveyed invert and outlet connectivity before affecting routes. Keep decorative wetness separate from hydraulic claims; photographs do not provide slope, flow capacity or flood return periods.

## Rights and delivery

CC BY-SA photograph reuse requires attribution, the applicable license and compliance with adaptation/share-alike terms. Author names above are source metadata; source pages retain exact spelling. Photo-derived texture production is not cleared by this note. Copyright licenses do not establish consent, property/model releases or permission to reproduce identifiable people, marks or private interiors. Original mesh design from general visual observations is preferred over direct photo-texture extraction.

Only this document was created. No other file edits, downloads saved to disk, tests, builds, editor/server starts, scene or renderer exports, commits or further delegated agents. Remaining evidence gap: fresh, geolocated photographs and measured sections showing **narrow concrete roadside gutters with concrete doorway/driveway slabs** at the actual authored slice. Existing historical crossing evidence and newly inspected larger-channel evidence must not be presented as closing that gap.

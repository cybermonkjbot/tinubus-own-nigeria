# Lagos geographic authoring reference

October 11, 2026. The adjacent JSON is an additive authoring manifest. No engine,
Windows bridge, editor, native generator or production renderer was changed.

## Use now

Select `geo20261011:yaba-first-street-detail`, a 150 by 100 metre workspace.
Its source anchor is `[26969,0,-7169]`; UE anchor is
`[-716900,2696900,0]` centimetres. Source bounds are
`[26894,-7219,27044,-7119]` in minX, minZ, maxX, maxZ order.
This is an estimated authoring workspace, not a measured street reconstruction.
Pin a current Yaba street reference before claiming a named street. The
[University of Lagos address](https://staffschool.unilag.edu.ng/contact-us/)
supports Akoka/Yaba identity; the
[Works Ministry scorecard](https://www.fmw.gov.ng/themes/front_end_themes_01/images/download/25030975757.pdf)
supports the Herbert Macaulay/Yaba bridge connection. Neither provides facade
dimensions or parcels.

The manifest includes numeric envelopes for every requested district, a
topology graph and Third Mainland corridor points. Read with standard Python
`json.load`. Additive IDs are not registered canonical jobs. The current
`WorldGeneration.execute` cannot consume them as existing coverage IDs.
Prepare a separate explicit native geometry handoff through the coordinator;
do not rename `city/districts/*` or weaken the canonical source guard.

Keep the initial 150 metre street detailed before expanding its coverage.
Player-height and close facade, road and drainage evidence must establish the
result. Boxes and simplified silhouettes are reference aids, not finished art.
Call of Duty is Joshua's visual target, not an achieved fidelity claim or a
source of assets we may copy.

## Land and water decisions

Yaba, Makoko, Ikeja and Oworonshoki belong to the mainland context. Ikeja stays
inland, using the
[revised Ikeja plan](https://lasbca.lagosstate.gov.ng/wp-content/uploads/2021/07/IKEJA-LAND-USE.dwg_.pdf)
for named areas and corridors. The plan covers 2016-2036 and includes proposed
infrastructure; it is not a current construction inventory.

Makoko needs a land-water transition, not a solid island tile. Its water
community and Makoko/Iwaya collaboration are documented by
[NLE](https://nleworks.com/case/makoko-floating-school/).
Keep structures over water distinct from land. The original floating school
collapsed; its historic drawings do not authorize a currently standing school.
Today's house extent and canoe passages have not been surveyed here.

Lagos Island and Ikoyi keep distinct district identities without an invented
wide sea channel between them. The
[University of Lagos morphology paper](https://ljes.unilag.edu.ng/article/download/1441/1102)
describes the filled or built-over canal and renewed land continuity. That
decision rests on an indexed primary excerpt because the full fetch failed.
The [2015 state plan](https://lasbca.lagosstate.gov.ng/wp-content/uploads/2021/07/Ikoyi-Victoria-Island-MCP.pdf)
supports regional district and road context, not today's remaining drain edges.

Five Cowrie Creek is distinct from the open lagoon and ocean. The
[2015 primary coastal study](https://www.scielo.sa.cr/pdf/rbt/v63n1/a16v63n1.pdf)
describes its western Lagos Harbour connection and eastern lagoon connection
near Ikoyi. The
[LASEPA monitoring account](https://lasepa.gov.ng/blog/officials-of-lasepa-embarked-on-the-monitoring-of-the-33-surface-water-bodies-in-lagos-state-for-the-year-2024-beginning-with-the-five-cowries-creek-ikoyi-which-spans-from-falomo-bridge-to-lekki-ik/)
names Falomo and Lekki-Ikoyi bridges on that waterway. No width, depth or deck
height is established by those references.

Victoria Island faces the Atlantic on its coastal side and the creek toward
Ikoyi. Preserve the Oniru/western Lekki land and road corridor in factual mode.
That continuity is an authoring inference from the state plan and
[coastal study](https://eprints.gouni.edu.ng/5446/1/3.Flood%20risks%20to%20urban%20development%202021c.pdf),
corroborated by the
[May 31, 2026 transport article](https://trafficradio961.ng/traffic-status-report-from-victoria-island-lekki-axis/).
The transport page contains unrelated injected-looking navigation, so it is
not sole evidence. Its historic temporary closure is not current map state.

Lekki is a peninsula. Western Lekki is a bounded authoring subset that continues
east, not a detached real island. The
[official resource directory](https://lasbca.lagosstate.gov.ng/resources/)
names Lekki Peninsula Scheme 1; the
[overall plan](https://lasbca.lagosstate.gov.ng/wp-content/uploads/2021/07/LCMP-UTM_Minna_31N-Geo-2-Overall-Landuse-A0-Sc-25.000-2.pdf)
places peninsula development in lagoon/ocean context. The 2021 geomorphology
paper uses barrier-island terminology for coastal processes. That does not
justify labelling the connected modern authoring region a detached island.

Snake Island is the optional additional harbour island. The
[operator's FAQ](https://nigerdock.com/faqs/)
documents its identity and Ro-Ro ferry connection with Tin Can. Do not invent a
road bridge or turn port-facility acreage into an island outline.
[The field-based Banana Island report](https://ng.boell.org/sites/default/files/uploads/2018/10/lagdel_2017_achieving_mixed_and_integrative_housing-min.pdf)
justifies a reclaimed peninsula subdistrict inside Ikoyi, not another natural
detached island. Tarkwa Bay and Atlas Cove are optional harbour-mouth context
from [NPA pilotage material](https://nigerianports.gov.ng/wp-content/uploads/2019/10/LAGOS-PILOTAGE-DISTRICT-converted.pdf).
Their names alone do not define a standalone island polygon. Proposed islands
on the 2013 plan are excluded from factual existing-land content.

## Bridge evidence

Third Mainland runs between Oworonshoki and Adeniji-Adele interchange on Lagos
Island. Its intermediate Adekunle/Yaba ramps are not its northern endpoint.
The Works scorecard provides those names and an 11.8 km reference length;
the [2020 ministerial speech](https://fmino.gov.ng/full-speech-by-the-hm-of-works-and-housing-mr-babatunde-fashola-san-at-the-press-briefing-on-the-partial-closure-for-repairs-and-restoration-of-the-third-mainland-bridge/)
corroborates length and axial connections. The scorecard's full 34 MB PDF
exceeded the web fetch limit; its inspected evidence is the indexed excerpt.

A read-only [OSM API sample](https://api.openstreetmap.org/api/0.6/map?bbox=3.401,6.501,3.410,6.509)
returned way `93855614` version `12` and opposite-carriageway `134222253`
version `15`, both edited September 18, 2026. The JSON retains five actual
node positions from the first way, each with node version and timestamp.
They are carriageway coordinates, not a surveyed bridge centreline.
The sampled response SHA-256 is
`f036bfc74f4c5fb2a00fc328b2ee490e63870e288d4e3d5494d5f0385e5dc7a1`.
The response was inspected in memory, not saved as a complete source snapshot.
Further full-way queries returned HTTP 429; Overpass failed and a larger
bbox failed. No full centreline or shoreline extraction is claimed.

Endpoint anchors and intermediate gap points remain labelled author estimates.
The numeric corridor is a piecewise-linear reference guide. Do not stretch it
to match 11.8 km, use zero-height samples as deck elevation, or derive ramps
and piers from absent structural data. Retain
[OpenStreetMap attribution and ODbL provenance](https://www.openstreetmap.org/copyright)
with consumed map data; redistribution needs the applicable licensing review.

## Coordinates and fictional option

Source is Y-up metres, x east and z south. The local approximation uses
WGS84 origin longitude `3.4074`, latitude `6.4506`, matching the existing
conversion convention without claiming that the existing fictional city is GIS.
The proposed expansion translation is `[30000,0,0]` source metres. This is a
fictional 30 km composition offset, not a factual displacement of Lagos.
Existing assets receive no translation. The canonical theatre stays
`[-2200,0,1350]`, UE `[135000,-220000,0]`.

Apply `UE(X,Y,Z)=100*source(Z,X,Y)` once. The JSON provides projection constants,
array orders, matrix, metric bounds and bridge coordinates. Cached metric
positions round to one metre for authoring; that is not geographic accuracy.
Height zero is an arbitrary reference plane, not surveyed sea level. The
regional eastern Lekki envelope is not suitable for detailed surveying with
this simple approximation. The official LCMP grid uses Minna UTM Zone 31 North;
its numbers have not been treated as WGS84 or silently converted.

The default preserves real regional continuity. The disabled
`fictional-separated-district-islands` option gives exact additive chunk
translations for Lagos Island, Ikoyi, Victoria Island and western Lekki.
It explicitly changes land continuity, relocates districts and requires newly
authored coastlines. Its Lekki chunk is labelled fictional. It does not change
the factual records, fabricate selection evidence or move saved canonical work.
Do not reuse factual bridge points after relocation; replacement crossings are
intentionally absent pending an actual authored decision. Chunk boxes cannot
become coastlines, and a nominal water-gap parameter is not geography.

## Limits and handoff

Every district bound is an overlapping estimated coverage window. None is an
official boundary, land mask or final coastline. There is no acquired DEM,
bathymetry, current shoreline vector set or current geolocated street-photo
collection. Older maps cannot certify October 2026 reclamation, structures,
water conditions or road operating state. Mainland Central's official PDF was
listed but its 22.9 MB fetch failed; its map content was not inspected.

Pin this manifest's actual bytes before a native handoff. External references
are not already accepted entries in the generator's canonical source lock.
Keep the reference-manifest reconciliation evidence intact and record actual
authority where required. This file contains no human approval, engine
authorization, collision/navigation acceptance, saved Home content or runtime
account admission. No tests, lint, typechecks, builds, compilation, installation,
push or engine operation ran for this handoff.

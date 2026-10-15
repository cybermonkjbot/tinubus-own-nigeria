# Lagos mosaic connector evidence ledger

Research/access date: **October 11, 2026**. Nine bounded endpoint contracts for coordinator review. Documentation only; the six delivered neighborhood dossiers and coordinator-owned README/integration plan remain read-only. No final engine placements, fixed map envelope or canonical-content changes.

## Evidence contract

We distinguish **F** (what a primary/local source explicitly says), **G** (freshly verified OSM shared-node graph segments), **M** (topological interpretation), **P** (fictional composite join or accessible design intent), and **U** (unknown). G certifies database connectivity only, not physical passage, current surface, safe crossing, legal vehicle direction or public access. A ferry way is mapped service geometry, not a hydrographic navigation chart or verified sailing track.

All coordinates are WGS84 **[longitude east, latitude north]**, decimal degrees, **no altitude**. OSM XML supplies separately named lon/lat attributes; bounding boxes below are west longitude, south latitude, east longitude, north latitude. Missing elevation is unknown, not zero. Retain source metres **x east, y up, z south; UE(X,Y,Z) = 100 * source(z,x,y)**, applied once only after coordinator registration. No degrees enter that conversion; no translation, rotation, datum or engine positions are assigned.

### Scope across the delivered twelve slices

We read `AGENTS.md`, the mosaic `README.md` and all six delivered dossiers: `everyday-mainland.md`, `tech-and-mixed-centres.md`, `old-lagos-island.md`, `island-and-coast.md`, `lagoon-neighborhoods.md`, and the previous `port-and-working-streets.md`. This audit connects selected references, not every pair of neighborhoods.

| Delivered references | Treatment in this bounded audit |
| --- | --- |
| University/Moore, Yaba; Makoko Road loop | C01 road approach; C02 unresolved shore transition. |
| Gaskiya College Road, Ijora; Liverpool/Idunganran, Apapa | C03 shared-node road chain; C09 water-network landing remains distinct from Liverpool street corner. |
| Iga/Oroyinyin and Breadfruit/Balogun, Old Island | C04 approach to Awolowo/Ikoyi; C05 Breadfruit-to-VI graph; C07 separate Ebute Ero and CMS receiving pins. |
| Adeola/Eletu, VI; Fola/Kafayat, Lekki | C05 reaches the VI reference; C08 adds Falomo water/bridge context. No audited onward route to Fola, and its small Fola channel bridge is not Falomo or Lekki-Ikoyi. |
| Ipakodo terminal approach | C06 dry approach and boarding gap; C07-C09 mapped/operator-supported water relationships. |
| Olufemi/Olukole, Surulere; Palm/Olorunsogo, Mushin; Toyin/Emina/Amore, Ikeja | Read for shared privacy/access/compression requirements. No cross-dossier route audited here; their existing fictional joins are not upgraded to G. |

## Fresh acquisition and licensing

All **successful** requests listed here were retrieved and inspected October 11, 2026. XML was parsed in memory; only this Markdown is retained. Full ways may extend beyond query rectangles, and map responses can include remote nodes through relation membership. A returned remote POI is not evidence it lies within the query. No photos, imagery pixels, video, field audio, surveys or private interiors were inspected. OSM edit dates and `source=survey` dates are source metadata, not our survey or photographic capture dates.

| Ref | Exact successful map URL | Purpose |
| --- | --- | --- |
| R01 | `https://api.openstreetmap.org/api/0.6/map?bbox=3.374,6.505,3.383,6.517` | Moore/Harvey/Herbert Macaulay approach. |
| R02 | `https://api.openstreetmap.org/api/0.6/map?bbox=3.378,6.496,3.390,6.508` | Joined mainland street graph toward Makoko. |
| R03 | `https://api.openstreetmap.org/api/0.6/map?bbox=3.388,6.497,3.395,6.500` | Makoko loop versus canal nodes; canal branches. |
| R04 | `https://api.openstreetmap.org/api/0.6/map?bbox=3.355,6.439,3.374,6.455` | Apapa roads and elevated approach. |
| R05 | `https://api.openstreetmap.org/api/0.6/map?bbox=3.353,6.455,3.374,6.468` | Ijora/Gaskiya to Apapa graph. |
| R06 | `https://api.openstreetmap.org/api/0.6/map?bbox=3.382,6.450,3.402,6.463` | Old Island reference streets. |
| R07 | `https://api.openstreetmap.org/api/0.6/map?bbox=3.397,6.440,3.405,6.452` | Onikan/Marina/Awolowo and bridge approach layers. |
| R08 | `https://api.openstreetmap.org/api/0.6/map?bbox=3.402,6.425,3.423,6.444` | Ahmadu Bello to Adeola/Eletu. |
| R09 | `https://api.openstreetmap.org/api/0.6/map?bbox=3.425,6.437,3.430,6.443` | Falomo bridge, terminal polygon and ferry ways. |
| R10 | `https://api.openstreetmap.org/api/0.6/map?bbox=3.485,6.601,3.490,6.604` | Ipakodo F270/service/footway and ferry graph. |

Additional successful discovery rectangles `https://api.openstreetmap.org/api/0.6/map?bbox=3.372,6.506,3.393,6.518` and `https://api.openstreetmap.org/api/0.6/map?bbox=3.405,6.436,3.413,6.444` supplied context, not separate route acceptance. A larger southern Yaba query returned HTTP 400; incomplete/slow queries were not used to assert connectivity. Search results were empty; direct primary pages and OSM supplied the admitted evidence.

**Graph method:** build an undirected diagnostic graph from consecutive nodes of retrieved highway ways, then inspect the resulting way changes/shared junctions. C01/C03 excluded construction, proposed, steps and explicitly private ways. C05's retained chain also excluded `access=no`, footways and paths. This is a connectivity search, not shortest-distance routing. One-way restrictions, turn relations, conditional access, sidewalk availability, barriers and vehicle classes were **not** solved. Absent access tags do not grant public permission. C04 retains a longer diagnostic street chain, not an optimized itinerary. No ferry edges are joined to road edges merely by proximity.

**OSM rights:** OpenStreetMap contributors, ODbL 1.0. Rights page read: `https://www.openstreetmap.org/copyright`. XML identifies `http://opendatacommons.org/licenses/odbl/1-0/`; preserve attribution and assess applicable database/produced-work obligations before shipping derivatives. Bing source tags do not license Bing imagery. Raw extracts are not durably archived under this one-file constraint; IDs, versions and transition records below are a bounded evidence ledger, not a complete reproducible offline dataset.

| Ref | Primary/local URL, inspected content and source date | Limits |
| --- | --- | --- |
| T01 | `https://lagferry.gov.ng/our-terminals/`: undated operator page lists Ipakodo -> Five Cowries/Falomo-Ikoyi and Ipakodo -> Ebute Ero/Elegbata -> Marina/CMS. | Text read; no sailing chart, gate location or current departures. Its inconsistent Mile 2 location wording is excluded. Copyright footer 2025, no open media licence established. |
| T02 | `https://lagferry.gov.ng/`: undated operator page lists Ikorodu <-> Liverpool/Apapa, Ikorodu/CMS and Ikorodu/VI/Falomo relationships. | Relationship support, not timetable or availability; terminal names and route arrows do not locate boarding edges. No photo/logo/text asset reuse clearance. |
| T03 | `https://lagferry.gov.ng/safety-and-regulation/`: undated operator guidance describes lifejackets, manifests, capacity control and no night/poor-visibility travel. | Attributed operator guidance only; no current legal advice or guarantee. Do not adopt the page's broad safety superlatives or hours as a real-time bulletin. |
| T04 | `https://nigerianports.gov.ng/security-services/`: displayed modification metadata October 10, 2022; access-control statement refers to ISPS procedures. | Port-controlled facilities excluded from public connector intent. No current security-level or entry-right claim; no operational gate/patrol map. Copyright, reference only. |
| T05 | `https://homef.org/2017/07/26/fishnet-dialogue-at-makoko/`: July 26, 2017 account of July 24 dialogue distinguishes land and water communities and livelihoods. | Historical participant account, not current parcel/shore access evidence. Text read; no imagery, testimony audio or community sub-boundary copied. All rights reserved. |

## Endpoint contracts

### C01: University/Moore, Yaba -> Makoko Road loop mouth

**Classification:** G road chain, M mainland neighborhood connection, P any shortened composite journey. **Endpoints:** node **2271072132 [3.3774551,6.5152498]**, University 207402243 / Moore 217801562; node **6981647402 [3.3895175,6.4978911]**, Makoko 217793801 / path 746004745. University full way freshly read at `https://api.openstreetmap.org/api/0.6/way/207402243/full`, v17, September 8, 2026. R01/R02 verify the following way transitions:

| Way / retrieved version; named label where supplied | Start node [lon,lat] -> next way's shared node [lon,lat] |
| --- | --- |
| 217801562 v6, Moore | 2271072132 [3.3774551,6.5152498] -> 2271072117 [3.3788712,6.5117847] |
| 114912315 v6, Harvey | 2271072117 -> 2176500609 [3.3774715,6.5111851] |
| 534993359 v33, Herbert Macaulay | 2176500609 -> 2176500549 [3.3784823,6.5085716] |
| 217801588 v4, unnamed tertiary | 2176500549 -> 2271072462 [3.3785699,6.5086031] |
| 1221904684 v2, unnamed service | 2271072462 -> 11332308892 [3.3785393,6.5084936] |
| 1221904685 v3, unnamed service | 11332308892 -> 2273196457 [3.3782110,6.5059695] |
| 218010782 v7, unnamed tertiary | 2273196457 -> 2273196536 [3.3783082,6.5058897] |
| 138058059 v17, Oyediran | 2273196536 -> 2270991242 [3.3841522,6.5024514] |
| 217793818 v4, unnamed residential | 2270991242 -> 2270990888 [3.3839123,6.5022869] |
| 138057991 v9, Sariyu | 2270990888 -> 1513671767 [3.3869031,6.4998385] |
| 585460393 v4, unnamed tertiary | 1513671767 -> 5594732021 [3.3868774,6.4994126] |
| 585460394 v3, unnamed roundabout | 5594732021 -> 5594725910 [3.3868584,6.4993527] |
| 745665069 v7, Church | 5594725910 -> 2270990963 [3.3866086,6.4979473] |
| 217793816 v3, unnamed residential | 2270990963 -> 2270991080 [3.3877292,6.4964306] |
| 217793801 v13, Makoko | 2270991080 -> 6981647402 [3.3895175,6.4978911] |

Way edit dates include Moore/Harvey June 14, 2024; Herbert Macaulay October 7, 2024; Oyediran May 12, 2026; Church/Makoko May 7, 2026. Those dates do not establish street condition. Makoko full response re-read: `https://api.openstreetmap.org/api/0.6/way/217793801/full`.

**Omitted fabric:** Harvey/Herbert frontage, unnamed service approaches, Oyediran/Sariyu streets, residential turns and Church/Makoko approach are not vacant distance. Their actual present uses remain U; retain occupied street/yard/service intervals in any P compression. This route reaches the land street, **not** a shore, jetty or Third Mainland endpoint.

**Modes/blockers:** graph has trunk, one-way and service links; no continuous pedestrian route, crossing or bicycle/vehicle permission certified. U kerbs, barriers, frontage rights, drainage grades and accessible widths. P connector intent: ordinary public land walking with protected crossings, a separate reviewed vehicle route, dry return and rest choices; never a campus/private-compound shortcut. All proposed clearances remain design targets, not measured real values. Preserve household/service acoustics before any waterfront sound.

### C02: Makoko loop -> canal -> wider water network

**Classification:** G footpath loop and canal branches separately; **P-only loop-to-shore connection**; U canal-to-main-lagoon navigation. **Endpoint references:** loop mouth **6981647402 [3.3895175,6.4978911]** on 217793801; canal node **6981755494 [3.3914869,6.4984914]** on 407972992. No shared node links the inspected footpath trio to that canal.

Fresh full responses: `https://api.openstreetmap.org/api/0.6/way/746004745/full` v3, `https://api.openstreetmap.org/api/0.6/way/745650052/full` v5, `https://api.openstreetmap.org/api/0.6/way/745487971/full` v4, all edited June 6, 2024; `https://api.openstreetmap.org/api/0.6/way/407972992/full` v22, June 13, 2024.

**G loop:** 6981647402 -> 746004745 -> **6979331163 [3.3898106,6.4978371]** -> 745650052 -> **9546057442 [3.3895587,6.4979424]** -> 745487971 -> **2270990831 [3.3894675,6.4980570]** -> Makoko Road. R03 identifies canals **495302474** and **745995881** also sharing 6981755494 with 407972992. The latter's other endpoint is **2269405526 [3.3945285,6.4948631]**. This is a canal-branch finding, not a complete open-water route; those branch continuations were not traced to a main lagoon boundary.

**Omitted fabric/blockers:** intervening waterfront homes, yards, paths, work spaces and shore edges cannot be declared empty or publicly traversable; exact present arrangement U. T05 supplies land/water community context, not this missing connection. U surviving structures, landing consent, ground/deck heights, tidal range, depth, overhead obstructions, edge protection and small-craft access. `boat=yes` on one canal does not resolve the entire route. No real Makoko-Ipakodo ferry service or canoe corridor is accepted.

**P contract:** a coordinator may author a distinct public shore spur and controlled small-craft landing, with two independent dry public exits and a non-boarding destination. Preserve human-scale homes and privacy; never extend a promenade through residents' yards. The source/destination references are research pins, not final landing positions. Water/roof runoff continuity, sound and collision need separate height treatment; no numeric levels set.

### C03: Gaskiya College Road, Ijora -> Liverpool/Idunganran, Apapa

**Classification:** G database road chain now established, replacing the earlier *untraced* status only in this new ledger; **P** shortened composite adjacency remains fictional. **Endpoints:** **1047196529 [3.3586889,6.4665092]** on Gaskiya 280570894 / residential branch 91339916; **1047195789 [3.3612290,6.4433273]** on Liverpool 144530723 / Idunganran 90288308. R04/R05:

| Way / version | Start -> next shared node [lon,lat] |
| --- | --- |
| 280570894 v8, Gaskiya College | 1047196529 -> 2846609453 [3.3627060,6.4662727] |
| 280570896 v4, Gaskiya College | 2846609453 -> 1047196597 [3.3627968,6.4662969] |
| 90288334 v15, Gaskiya | 1047196597 -> 1472612461 [3.3637088,6.4664391] |
| 1545148118 v1, unnamed primary | 1472612461 -> 1472591503 [3.3638060,6.4657742] |
| 133800605 v4, motorway link | 1472591503 -> 1472591472 [3.3639127,6.4652209] |
| 133800624 v3, bridge/motorway link, layer 1 | 1472591472 -> 1472591527 [3.3640674,6.4632282] |
| 133795062 v11, Apapa Road bridge, layer 1 | 1472591527 -> 6047642615 [3.3654025,6.4556993] |
| 642340877 v2, Apapa Road bridge, layer 2 | 6047642615 -> 1472591498 [3.3661025,6.4542397] |
| 133800601 v7, Apapa Road bridge, layer 2 | 1472591498 -> 1472594421 [3.3666243,6.4530435] |
| 133801174 v11, Apapa Road | 1472594421 -> 5752967076 [3.3672638,6.4512928] |
| 606892598 v4, primary link | 5752967076 -> 5752967075 [3.3672172,6.4511329] |
| 144840475 v12, unnamed tertiary | 5752967075 -> 1583391499 [3.3669379,6.4517400] |
| 144840426 v5, unnamed residential | 1583391499 -> 1475464898 [3.3661251,6.4502477] |
| 152645926 v3, North Avenue | 1475464898 -> 1475464849 [3.3639808,6.4511568] |
| 134170089 v7, Central Avenue | 1475464849 -> 1475464857 [3.3630701,6.4490284] |
| 152645928 v10, Child Avenue | 1475464857 -> 1047195574 [3.3666308,6.4475751] |
| 144530723 v19, Liverpool | 1047195574 -> 1047195789 [3.3612290,6.4433273] |

Gaskiya/approach-link edits include July 27, 2026; Apapa bridge 133795062 October 10, 2026; Liverpool May 12, 2026. The chain is not a present drivable itinerary: diagnostic edges were undirected and contain one-way links. No short, step-free Gaskiya-to-Liverpool foot route has been established.

**Omitted fabric:** Gaskiya approaches, elevated road connections, lower-level streets and North/Central/Child Avenue frontages; houses, services and work yards must not become an all-truck buffer. U exact current frontage uses. **Vertical/access blockers:** layers 1/2 are ordering tags, not metres or surveyed deck heights. U ramps, soffits, safe sidewalks, legal crossings and road admission. Do not flatten bridge and under-road worlds into one junction. T04 keeps terminals/quays outside public path intent.

**P contract:** retain an inhabited street-scale transition and a separately reviewed vehicle crossing/transfer; do not force pedestrians onto motorway bridges. Public dry retreat and at least two accessible departure choices must survive local incidents. No private yard or port-controlled gate is required. Carry road sound through lower streets without applying an industrial loop to homes; real drainage catchments and road levels remain separate U.

### C04: Iga/Oroyinyin, Old Island -> Awolowo Road / Ikoyi approach

**Classification:** G connected street chain to an **approach**, M Island-to-Ikoyi-oriented corridor; P any compressed join. **Endpoints:** **1020822321 [3.3896613,6.4599162]** on Iga 87805658; **2250381494 [3.4041333,6.4435857]** shared by roundabout 215573640 and Awolowo 133753446. Awolowo full way freshly read: `https://api.openstreetmap.org/api/0.6/way/133753446/full`, v10, March 28, 2021. It continues to **5695500276 [3.4061011,6.4442011]**. This is not a new Ikoyi neighborhood slice or a Falomo bridge endpoint.

R06-R08 diagnostic street sequence below preserves all successive way-transition nodes; coordinate table follows in the same order. Long detours in this unweighted graph are not a recommended itinerary.

```text
87805658 v11 -> 129535577 v6 -> 87808308 v9 -> 88270785 v8
-> 166721551 v6 -> 88270775 v11 -> 166721548 v5 -> 530737594 v2
-> 740626972 v5 -> 129556790 v7 -> 129556772 v19 -> 740626971 v4
-> 129586392 v5 -> 133753449 v7 -> 129586397 v4 -> 586656936 v4
-> 129586377 v4 -> 129586390 v5 -> 133753449 v7 -> 215573639 v3
-> 215573640 v3 -> endpoint 2250381494 on 133753446
```

| Successive shared node | [lon,lat] | Labels adjoining the transition where supplied |
| --- | --- | --- |
| 1020782197 | [3.3898884,6.4589069] | Iga / Idumagbo |
| 1020822264 | [3.3900226,6.4589189] | Idumagbo / Isale Agbede |
| 1025377598 | [3.3903947,6.4575330] | Isale Agbede / St John Pachi |
| 5393415813 | [3.3898705,6.4570500] | St John Pachi / Palm Church |
| 1025377514 | [3.3903616,6.4564941] | Palm Church / unnamed way |
| 1429178262 | [3.3919379,6.4551040] | unnamed way / Massey |
| 2844878986 | [3.3940454,6.4531991] | Massey / unnamed way |
| 1025377461 | [3.3939235,6.4526887] | unnamed way / Bamgboshe |
| 1025377285 | [3.3965454,6.4521723] | Bamgboshe / Oil Mill |
| 1429178438 | [3.3964291,6.4514725] | Oil Mill / Igbosere |
| 1025377295 | [3.4026761,6.4497624] | Igbosere segment join |
| 1429440095 | [3.4046242,6.4491941] | Igbosere / unnamed way |
| 111773870 | [3.4050220,6.4480194] | unnamed way / King George V |
| 111761973 | [3.4053305,6.4473227] | King George V / Berkley |
| 1429440034 | [3.4047540,6.4465033] | Berkley / McCarthy |
| 1429440047 | [3.4038519,6.4465640] | McCarthy / Military |
| 1429440065 | [3.4035247,6.4456499] | Military / Ajasa |
| 1429440230 | [3.4051621,6.4450292] | Ajasa / King George V |
| 111751398 | [3.4043171,6.4439986] | King George V / unnamed one-way link |
| 2250381470 | [3.4041904,6.4437677] | link / roundabout |

**Omitted fabric:** the named and unnamed Island streets, homes, commercial/service frontages and Obalende-area approach remain occupied city, not a bridge-length blank strip. Current parcel activity U. **Blockers:** one-way/roundabout direction unresolved; no continuous footway, gradient, traffic-held crossing or passage-right audit. No distinct measured water/deck crossing is certified by this chain. P accessible connector needs its own protected public route and dry alternatives; never run through palace, military, worship or household interiors because a road name appears in the graph. Awolowo endpoint is a street reference, not permission to enter adjacent properties.

### C05: Breadfruit east interface, Old Island -> Adeola/Eletu, VI

**Classification:** G road-only chain excluding explicit `access=no/private` candidates; M bridge-layer interpretation; P any shortened world join. **Endpoints:** **1025307578 [3.3881178,6.4563327]**, Breadfruit 530737584 / Nnamdi Azikiwe 264916646; **1235946207 [3.4213475,6.4295050]**, Adeola 5150268 / Eletu 107554130. R06-R08:

| Way / version | Start -> next shared node [lon,lat] |
| --- | --- |
| 264916646 v9, Nnamdi Azikiwe | 1025307578 -> 1025307880 [3.3897500,6.4539283] |
| 1353974309 v1, Broad | 1025307880 -> 1025307298 [3.3898467,6.4533931] |
| 762582690 v9, Broad | 1025307298 -> 7125410737 [3.3983446,6.4469124] |
| 1353974308 v1, Campbell | 7125410737 -> 5390504954 [3.3984322,6.4470391] |
| 558864243 v1, unnamed residential | 5390504954 -> 5390504950 [3.3995344,6.4470714] |
| 10561261 v10, Tafawa Balewa Square | 5390504950 -> 91273202 [3.4015918,6.4448234] |
| 10561279 v11, Force | 91273202 -> 31701744 [3.4001971,6.4434511] |
| 1067200367 v4, Old Marina | 31701744 -> 2429766017 [3.4004191,6.4432389] |
| 1067200368 v7, Old Marina | 2429766017 -> 1472203766 [3.4005938,6.4430644] |
| 133758773 v8, Old Marina | 1472203766 -> 1472203764 [3.4021514,6.4416665] |
| 133758772 v8, unnamed primary | 1472203764 -> 1090910134 [3.4026058,6.4412747] |
| 93858642 v13, Ahmadu Bello bridge, layer 1 | 1090910134 -> 1090910163 [3.4041342,6.4402163] |
| 133790499 v8, Ahmadu Bello | 1090910163 -> 5588599658 [3.4070371,6.4382180] |
| 133790491 v8, Ahmadu Bello bridge, layer 1 | 5588599658 -> 1472506608 [3.4084922,6.4358232] |
| 133790492 v9, Ahmadu Bello | 1472506608 -> 31674733 [3.4087734,6.4346453] |
| 505413824 v16, Ahmadu Bello | 31674733 -> 1480437222 [3.4089000,6.4300156] |
| 5150268 v35, Adeola Odeku | 1480437222 -> 1235946207 [3.4213475,6.4295050] |

Broad 762582690 and Old Marina 1067200368 were edited October 10, 2026; Adeola August 10, 2026. Bridge 93858642 edited October 8, 2025; 133790491 August 16, 2025. **M:** this Ahmadu Bello alignment supplies the Island/VI bridge corridor reference; the retrieved ways do not themselves name these two pieces Bonny Camp or Independence Bridge, so those exact structure identities are not assigned here. It is not Third Mainland, Falomo or Lekki-Ikoyi.

An initial all-highway diagnostic from Iga found **671991258 `access=no; bus=designated; psv=designated`** and footway/alley shortcuts. Those are not accepted general-public connectors. The retained Breadfruit chain avoids that way; no implication that all other streets are unrestricted follows. Direction/turn relations remain unsolved.

**Omitted fabric/blockers:** Broad/Campbell/TBS/Force/Marina frontages and VI Ahmadu Bello/Adeola service neighborhoods are developed fabric. U current access, carriageway crossings, deck/ramp/soffit heights, protective barriers and walkway continuity. Layers are not heights; `foot=yes` is not an accessibility inspection. P preserve public walking and separate reviewed road modes, dry rest/return choices and local service access. No giant water gap or tiny bridge is invented to fit an envelope; no auditory surf substituted for creek/urban traffic. Drainage cannot be welded across elevated decks at a guessed level.

### C06: Ipakodo F270 neighborhood approach -> terminal footway and boarding boundary

**Classification:** G road/service chain and G footway-to-terminal-point chain **separately**; U gap between them; P public forecourt connection. **Source:** F270/residential junction **1577179182 [3.4873391,6.6026453]** on 144135082 / 448344877. **Receiving references:** service-loop node **9903652937 [3.4862790,6.6021268]**; footway node **12864935853 [3.4865266,6.6017325]**; terminal point **12814051343 [3.4861459,6.6017283]**. Polygon sample **127831615 [3.4866471,6.6014758]** is not a gate, deck or centroid.

**G R10:** 144135082 v12 reaches **5210744505 [3.4865200,6.6030090]**; short F270 way 585042278 v1 connects that node to roundabout 538323252 v2 at **5210744469 [3.4863514,6.6032692]**. Another directed approach 585042279 v1 also shares 5210744505 and the roundabout, at **5210744485 [3.4862329,6.6030949]**. Roundabout shares **5210744479 [3.4861089,6.6031086]** with service 1080483434 v1; that reaches 9903652937 on loop 538323256 v4. No real bidirectional roundabout driving is asserted.

Fresh full responses: `https://api.openstreetmap.org/api/0.6/way/144135082/full`, `https://api.openstreetmap.org/api/0.6/way/538323252/full`, `https://api.openstreetmap.org/api/0.6/way/1080483434/full`, `https://api.openstreetmap.org/api/0.6/way/538323256/full`, `https://api.openstreetmap.org/api/0.6/way/1389859755/full`, `https://api.openstreetmap.org/api/0.6/way/659370442/full`. Service ways last edited July 22, 2022; footway v1 and terminal polygon v3 May 26, 2025. R10 supplies the two short F270 approach ways, both May 2, 2018.

**New G:** footway **1389859755** ends at **12814051343**, a mapped Ikorodu terminal point also used by ferry ways C07-C09. It does **not** share a node with 1080483434/538323256. This upgrades the footway's water-network identity, not its connection to the public road or an accessible boarding ramp. `operational_status=operational` and `survey:date=2025-02-22` on the point remain mapper assertions.

**Omitted fabric/blockers:** residential branch/frontage, controlled service/terminal space and actual shore threshold. U admission gates, pedestrian continuation, ramps, railings, deck/water range, public bypass and service-vehicle conflicts. P forecourt may join them with two independent dry exits, protected waiting/rest space and optional controlled boarding. No ticket purchase, available ferry or private yard becomes the only exit. No invented elevation or measured width. Keep household traffic distinct from berth engines; roof/runoff and landing levels require separate review.

### C07: Ipakodo -> Ebute Ero / Marina-CMS main water relationship

**Classification:** **G mapped ferry way + F operator regional text support**, P any time-compressed transfer. **Endpoints:** **12814051343 [3.4861459,6.6017283]**, Ipakodo; **12814705879 [3.3824785,6.4627403]**, Ebute Ero; **6274484744 [3.3898032,6.4491490]**, Marina/CMS. Ebute Ero and CMS are **different points**, not one interchangeable Island gate.

Fresh full way `https://api.openstreetmap.org/api/0.6/way/1505631586/full`, v5, May 7, 2026: ordered references **6274484744 -> 13774167063 -> 13774167062 -> 13774167061 -> 13774167060 -> 12814705879 -> 13774167059 -> 13774167058 -> 13774167057 -> 13774167056 -> 12814051343**. Tags state `route=ferry`, from Marina/CMS, to Ikorodu/Ipakodo, optional Ebute Ero. T01 lists Ipakodo -> Ebute Ero -> Marina; that operator direction and mapped geometry are relationship support, not verified service patterns. Additional `https://api.openstreetmap.org/api/0.6/way/1505631654/full` v6, May 7, 2026, contains all three pins while its destination wording includes Flour Mills/Apapa; do not silently reinterpret it as a simple direct CMS trip.

Individual point checks: `https://api.openstreetmap.org/api/0.6/node/12814705879` v5, May 7, 2026; `https://api.openstreetmap.org/api/0.6/node/6274484744` v3, April 27, 2026. Geometry segments are generalized source lines, not depth-tested tracks.

**Omitted fabric/blockers:** inhabited shores, working waterfronts, terminal grounds and water journey; after landing, the receiving walk to Iga/Breadfruit remains untraced in this audit. Do not snap Ebute Ero or CMS to a market corner or erase intervening Island streets. U navigable clearance/depth, shoreline/berth levels, accessibility, permissions and actual receiving gates. P keep independent dry arrival/departure routes and non-boarding meeting/rest choices; any compressed boat journey is fictional. No fares, timetables, operator availability or present access guarantee is adopted.

### C08: Ipakodo -> Five Cowries/Falomo; distinguish terminal and bridge

**Classification:** G mapped ferry way + F T01/T02 regional relationship; M terminal identity reconciliation still open; P compressed transfer/onward street link. **Endpoint references:** Ipakodo **12814051343 [3.4861459,6.6017283]**; route destination point **13774168421 [3.4271794,6.4420945]**. Terminal polygon **593494003** supplies a separate boundary sample **5661710418 [3.4272331,6.4433969]**; that is not a boarding position.

Fresh `https://api.openstreetmap.org/api/0.6/way/1505631652/full` v7, July 17, 2026: ordered route includes Ipakodo, optional Alluvia/Afisco node 12814687055, Addax **6274815661 [3.4419567,6.4367766]**, and 13774168421. Tags describe Addax as mandatory; T01's simpler Ipakodo/Falomo wording does not resolve actual stop patterns. Exact companion checks: `https://api.openstreetmap.org/api/0.6/way/593494003/full` v4, February 12, 2019; `https://api.openstreetmap.org/api/0.6/node/13774168421` v4, May 8, 2026. The point says **Five Cowries/Falomo (VI)** while the polygon says **Five Cowries Terminal**, address Falomo Roundabout, T01 says **Falomo-Ikoyi**, and the ferry way's `to` tag says **Five Cowries/Falomo/Ise Water (Ikoyi)**. Keep this naming/location mismatch visible; neither a point label nor polygon sample is a surveyed boarding gate. The point carries both `source_date=2026-04-21` and `survey:date=2025-02-22`; those differing mapper dates establish neither imagery capture nor our own inspection.

**Separate G bridge context:** `https://api.openstreetmap.org/api/0.6/way/1061545449/full` v2, July 2, 2025, connects **3684154039 [3.4254643,6.4396015]** -> **1472142463 [3.4265374,6.4420468]**; `https://api.openstreetmap.org/api/0.6/way/10740237/full` v13, same edit date, continues to **95636539 [3.4280355,6.4462146]**. Both are named **Falomo Bridge**, primary, one-way, bridge layer 1. R09 also maps the opposite bridge alignment 5150262; no continuous terminal-to-bridge foot route was accepted.

Water way `https://api.openstreetmap.org/api/0.6/way/129586395/full`, v7, June 20, 2026, names **Five Cowries Creek**; reference node **1429440231 [3.4277021,6.4409984]** is near this bridge corridor. Crossing geometry is not a road/water junction. It does not supply deck/water elevation or navigable clearance.

**Omitted fabric/blockers:** intermediate water/stop relationships, Falomo terminal grounds, Ikoyi/VI street approaches and the onward route to Adeola/Eletu. Do not make this terminal an instant VI service-street exit. U terminal identity, ramp/deck/tide range, public entry, protected bridge walking and separate water/road levels. P retain controlled boarding and a dry public alternative on each shore, with quiet household/service frontage between landing and busy roads. Falomo is not Third Mainland or Lekki-Ikoyi; no endpoint substitution is authorized.

### C09: Ipakodo -> Liverpool/Apapa water landing, not port entry

**Classification:** G mapped ferry way + F T02 operator regional support; P any compressed water transfer and landing-to-street join. **Endpoints:** Ipakodo **12814051343 [3.4861459,6.6017283]**; Liverpool landing point **12814706438 [3.3591485,6.4390560]**. The Apapa dossier's street junction is **1047195789 [3.3612290,6.4433273]**; it is **not** this landing.

Fresh `https://api.openstreetmap.org/api/0.6/way/1505631653/full`, v4, May 7, 2026, has `route=ferry`, from Ikorodu/Ipakodo, to Liverpool; ordered geometry begins at 12814706438 and ends at 12814051343, so node order alone is not the service direction. Individual landing check `https://api.openstreetmap.org/api/0.6/node/12814706438`, v7, May 7, 2026, records Liverpool/Apapa and mapper survey metadata. T02 lists both Ikorodu -> Liverpool and Liverpool -> Ikorodu. Their existence as listed relationships does not establish departures on the access date.

**Omitted fabric/blockers:** lagoon/channel travel, other working waterfronts, shore/service access and the untraced land route from landing to Liverpool/Idunganran. Do not use a terminal fence or port interior as the missing public connection; T04 access controls remain explicit. U actual landing, gangway slope, water/deck range, channel depth/clearance, public shore route, crossing permissions and current access. P separate passenger arrival from cargo/service circulation, with ordinary occupied Apapa frontage, dry non-boarding destinations and two public departure choices. No rust-only equipment palette or all-industrial connector; no engine placement/shore level assigned.

## Coordinator decisions and unresolved blockers

1. **Accept graph evidence separately from playable routing.** C01/C03/C04/C05 show database chains; one-way travel, turn restrictions, rights, crossings and step-free paths need separate evidence. C03's elevated motorway sequence is a particularly unsuitable default pedestrian assumption.
2. **Keep all omitted neighborhoods inhabited.** The named/unnamed route intervals above are the omission ledger's initial inventory. Present storefront/home use is unknown, not empty. Preserve ordinary compounds, work yards, service entrances, rest and quieter streets in fictional composition; compress journey length, not people/buildings/vehicles.
3. **Resolve shore interfaces before water integration.** C02 has no verified loop-to-landing graph; C06 has a public-road/service-to-terminal-footway gap. C07-C09 use mapped ferry points, not surveyed ramps/decks. All vertical datum, tide range, depth, bridge clearance and shoreline-access evidence remains U.
4. **Reconcile Five Cowries identities without moving evidence pins.** The old polygon, newer VI-labeled point and operator's Ikoyi terminology differ. No silent merger, snapped shore, assumed gate or terminal-to-Adeola shortcut.
5. **Do not adopt ferry operations metadata as current instructions.** Some OSM routes contain March-September conditions and late-evening times; T03 says no night/poor-visibility operations. October 11 falls outside those tagged seasonal windows, but no service conclusion follows from potentially stale/inconsistent tags. No fare, schedule, availability, route-access or legal guarantee is supplied.
6. **Author credible accessible choices as P only.** Public connector intent needs continuous supported footing, protected crossings, resting/turning pockets, redundant visible/non-audio cues and independent dry departures. A provisional 1.8-2.0 m clear route and 1.5 m turning pocket are design values, not measurements or compliance certification. No private courtyard, paid boat, sacred/private institution or port gate is a mandatory bypass. Weather/boarding closure must not disconnect a whole district.

No current appearance is inferred from surface, building, `source=survey` or operational tags. No fresh imagery capture date is available. No measured widths, architectural dates, grades, acoustic levels, shoreline or bridge heights were obtained. Water graph lines do not prove hydraulic/drainage continuity; street/household and berth audio should transition through authored barriers, not neighborhood stereotypes.

**Delivery:** nine endpoint contracts, fresh shared-node and ferry-way evidence, primary/operator relationship checks and explicit fictional/unknown boundaries. Only `connector-evidence-ledger.md` is authored. No neighborhood asset lists recreated, no tests/builds/lint/typechecks/server starts, scene generation, native/runtime/renderer/mobile/config edits, purchases, public messages, subagents or commits. Source review is not playable, visual, operational or release acceptance.

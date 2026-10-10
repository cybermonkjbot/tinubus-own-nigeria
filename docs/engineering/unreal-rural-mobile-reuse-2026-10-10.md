# Unreal rural-expedition mobile reuse

Research date: October 10, 2026.

## Evidence and scope

Our [installed-source evidence](cribtest11-native-furniture-api-2026-10-10.md) identifies UE 5.8.3 files on Cribtest11, changelist `58210709`, branch `++UE5+Release-5.8`. It does not establish engine startup. This pass uses that record, not a fresh machine inspection. Epic's versioned sources below describe UE 5.8, except the explicitly historical UE 5.5 lighting source. They do not independently certify patch-specific behavior.

This note covers environment and presentation decisions for our rural-expedition direction. It does not redesign the parent's native AI squad-order implementation. Recommendations below are design judgments, not measured device results.

## Author roads; keep runtime routes separate

Landscape Splines author roads, trails and fences, deform spline meshes, and modify terrain through the Landscape editing workflow. The 5.8 documentation specifies a Spline edit layer before spline changes affect the heightmap. This is evidence for terrain authoring, not a documented packaged-game terrain-generation API. [1]

Use authored Landscape Splines for persistent roads and paths. Do not equate a drawn road spline with a navigable route, collision readiness or a squad movement plan. If an expedition needs changing routes, select or calculate routes over available traversable space independently of terrain authoring. A runtime line or path mesh is presentation, not proof that the terrain, collision and navigation were regenerated. Runtime terrain modification remains unconfirmed here; do not make it a dependency of the first rural slice.

## Streaming and baked lighting need separate decisions

World Partition streams grid cells around streaming sources. Spatially loaded actors also depend on their Data Layer being enabled; non-spatial actors can remain loaded whenever their layer is enabled. Runtime Data Layers support event-driven world changes. `Loaded` means resident but not visible; `Activated` means loaded and visible. Requesting a state must not be treated as completed streaming. [2][3]

Use spatial streaming for travel and Runtime Data Layers for mission variants. Do not put the entire countryside in an always-loaded layer. Epic warns that loading too many layer assets together, or assigning widely used assets to many runtime layers, can degrade streaming. These systems do not establish a phone memory budget. [3]

Correct the blanket claim that World Partition cannot use baked lighting. UE 5.5 introduced experimental support through CPU Lightmass, including per-cell lightmaps and independently streamed volumetric lightmaps. Its documented opt-in included `r.AllowStaticLightingInWorldPartitionMaps=1`. That is historical evidence, not a verified 5.8.3 setup prescription. [4]

The reviewed 5.8 pages do not settle that feature's production status or the exact mobile/static-lighting/Data-Layer combination. Keep compatibility unresolved. Recommend a bounded, conventionally streamed baked level as the fallback, and do not promise that switching mission geometry also switches valid baked illumination. Mobile baked-lighting support alone does not certify the World Partition baking workflow. [2][3][4][7]

## Foliage needs geometry and pixel budgets

Reuse native instancing for repeated vegetation. Epic's current ISM documentation supports per-instance LOD; the old claim that only HISM supports it is obsolete. HISM can benefit thousands of stationary instances through a static hierarchy, but groups instances for LOD. ISM performs per-instance culling and LOD on the GPU, which can cost more on low-powered platforms. Static Mesh Foliage painting creates HISMs. Neither component is a universal winner. [5]

Recommend authored non-Nanite LODs and explicit cull distances by vegetation class. Keep grass shorter-range than landmark trees, and preserve silhouettes needed for navigation. Treat masked pixel coverage separately from instance count. Instancing does not justify dense overlapping leaf cards. Epic documents worse masked overdraw for traditional billboards and warns that expanding their sprites worsens it. [5][6]

Distant impostors are baked approximations, not free geometry substitutes. Epic's impostor material blends three captured views on desktop but falls back to one frame sample on mobile. Traditional billboards have fixed-view errors; upper-hemisphere impostors omit below-object views. Reserve these for distant vegetation whose viewing angles fit the capture. Do not use them as nearby cover silhouettes or assume desktop transitions represent phone output. [6]

## Choose the mobile renderer deliberately

Recommend Mobile Forward with precomputed environment lighting as the initial target. Epic's 5.8 matrix supports baked lighting, baked GI and reflection captures on Mobile Forward. It excludes Nanite, Lumen GI/reflections, Virtual Shadow Maps and hardware ray-tracing features from the listed mobile paths. A desktop-renderer preview does not establish mobile support. [7]

Do not copy older blanket exclusions either. The 5.8 matrix conditionally supports volumetric fog, screen-space reflections and point-light shadows. Conditional support is not a device-wide promise or a performance budget. Keep these outside the baseline until the target-device constraints are established. Recommend limited dynamic lighting for moving characters and specific gameplay needs, with baked terrain and settlement lighting. Changing the sun or large occluders must not be presented as automatically updating baked GI. [7]

## Spatialize events without revealing hidden actors

Native attenuation provides distance falloff, spatial panning and optional trace-based occlusion with filtering and volume reduction. Binaural spatialization depends on the selected plugin and suits headphones; standard panning suits loudspeakers. Occlusion uses simple collision by default because complex collision costs more. These are audio-rendering controls, not a gameplay authorization system. [8]

Our presentation rule should admit actual listener-relevant audible events, plus explicitly authorized radio, mission or accessibility events. Use an event's permitted origin and information precision. A remembered footstep must not become a continuously updated marker following its hidden source actor. Suppress sound-derived hints when the corresponding event is inaudible under the game's hearing rules, unless an explicit accessibility mode authorizes the equivalent cue. Never infer audibility or permission from actor existence, proximity or a successful play request alone.

No code, assets or configuration changed. No engine launch, validation commands, installs, remote changes or commits ran. Device performance and the unresolved lighting combination remain unverified.

## Primary sources

1. [Epic, Landscape Splines, UE 5.8](https://dev.epicgames.com/documentation/en-us/unreal-engine/landscape-splines-in-unreal-engine?application_version=5.8).
2. [Epic, World Partition, UE 5.8](https://dev.epicgames.com/documentation/en-us/unreal-engine/world-partition-in-unreal-engine?application_version=5.8).
3. [Epic, World Partition Data Layers, UE 5.8](https://dev.epicgames.com/documentation/en-us/unreal-engine/world-partition---data-layers-in-unreal-engine?application_version=5.8).
4. [Epic, UE 5.5 release notes, Static Lighting](https://dev.epicgames.com/documentation/en-us/unreal-engine/unreal-engine-5-5-release-notes).
5. [Epic, Instanced Static Mesh Component, UE 5.8](https://dev.epicgames.com/documentation/en-us/unreal-engine/instanced-static-mesh-component-in-unreal-engine?application_version=5.8).
6. [Epic, Impostor Baker, UE 5.8](https://dev.epicgames.com/documentation/en-us/unreal-engine/impostor-baker-plugin-in-unreal-engine?application_version=5.8).
7. [Epic, Mobile rendering feature matrix, UE 5.8](https://dev.epicgames.com/documentation/en-us/unreal-engine/rendering-features-reference?application_version=5.8).
8. [Epic, Sound Attenuation, UE 5.8](https://dev.epicgames.com/documentation/en-us/unreal-engine/sound-attenuation-in-unreal-engine?application_version=5.8).

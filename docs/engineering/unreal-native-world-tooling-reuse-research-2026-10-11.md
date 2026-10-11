# Unreal-native world tooling reuse assessment

October 11, 2026. Bounded source research for the authorized Unreal port. This note assigns responsibilities to existing mechanisms, not to implementation owners. No engine, MCP, remote-machine, generation, import, conversion or validation operation ran. No other owner's dirty source was inspected.

## Conclusion

Keep exact Baddie recipes as the inputs. Let Unreal own mesh assets, actor placement, instancing and level streaming. Use splines and Geometry Script where they preserve the authored result, not as a reason to rebuild already-defined geometry. PCG is optional for approved scatter or explicit data-driven placement. It should not redesign the city, interiors or saved rooms. Retain a thin identity, coordinate, approval and evidence adapter around those native operations. These are recommendations inferred from the source and Epic documentation, not verified installed capabilities. [R1][R3][E1][E2][E3][E4]

## Version and evidence boundary

- Recorded observation: the existing source review reports Cribtest11 UE 5.8.3 build metadata inspected at `2026-10-11T03:27:42.0012522Z`. It records PCG and experimental PCG Primitives manifests, plus an Editor-target restriction in the inspected local project copy. The remote readback did not include that restriction. This assessment did not repeat those inspections. [R2]
- Documentation observation: Epic's consulted guides and API pages identify UE 5.8. They do not establish patch-level UE 5.8.3 compatibility, local module loading, target support or callable MCP tools. [E1][E5][E11]
- Source observation: only generator commit `929a81b4779ffa07d2317c75eba1206516f9cb3b` was read, enough to classify its work. Its embedded canonical source pin is `e978763b74acfb0369b65b879af6a3a8c90b110b`. The existing review records 34 prepared recipes, 101 missing-input jobs and zero generated maps/packages. Those are historical preparation results, not a fresh execution inventory. Its outstanding corrections remain with their owners. [R2][R3]

The parent's newer pinned record establishes these installed-source facts, not live availability. [R4]

| Inspected item | Recorded fact | SHA-256 |
| --- | --- | --- |
| `PCGToolset.uplugin` | VersionName `1.0`, experimental, `EditorOnly=true`, Editor module and target allowlist | `089b1fe3bf6686adfa4b2970d52d568a9fb38b749053968de5b9b4319692639e` |
| `PCGInterops/PCGPythonInterop/PCGPythonInterop.uplugin` | VersionName `0.1`, beta, `PCGPythonInteropEditor` module of type Editor | `9f0f786ab2ef07d980ca35fab940485e1a2d078616e01fe7b2d7672a35efe278` |
| `PCGToolset.h` | `AICallable` declarations include `GetGraphStructure`, `GetGraphSchema`, `ListNativeNodes`, `GetNativeNodeSchema`, mutations and `ExecuteGraphInstance` | `3edc350253d2a1002ef6ae42e56c6496e4383e4dc7b3ae8f010b87fb11aab641` |

The successful Windows listener inspection at `2026-10-11T03:43:42.9404435Z` found no port 8000 listener. Live tool schemas remain unavailable in this evidence. Source declarations prove neither registered tools nor execution. The record's unrelated crypto details were excluded. [R4]

## What the pinned adapter does

Source facts, not proof that the native calls succeed:

- It translates canonical parcels, authored parts and road points into explicit transforms or triangle meshes. Its coordinate mapping is `UE(X,Y,Z) = 100 * source(Z,X,Y)`. Home variants preserve source-specific extension and rotation rules. [R3, geometry.py:1-44; recipes.py:173-250]
- It imports explicit geometry through Unreal's asset tools into StaticMesh assets, creates Materials, groups repeated parts into native HISM components, and merges same-finish explicit geometry into bounded cells. Journals retain canonical IDs with instance indices or triangle ranges. The 240-metre grouping is custom authoring organization, not World Partition. [R3, native_generator.py:214-298,319-379]
- It creates non-partitioned native World assets or duplicates approved templates. Composition uses `LevelStreamingDynamic`; children begin unloaded and invisible. Hashes, approved inputs, owned paths and journals wrap the editor operations. Generation does not admit a player or activate gameplay. [R3, README.md; native_generator.py:381-443]

This is a deterministic recipe translator and guarded editor adapter, not a competing runtime renderer. Generic geometry construction is the main candidate for selective native reuse. Replacing every operation with a PCG graph would not remove the source contracts or approval requirements. That is an inference, not a proposed rewrite. [R3][E1][E3]

## Native mechanisms and recommended responsibility

The middle column records documented facts. The last column is the Baddie-specific inference.

| Responsibility | Epic documentation | Recommendation for this port |
| --- | --- | --- |
| Exact placement and repetition | ISM renders multiple instances of one StaticMesh with per-instance transforms. HISM adds a hierarchy suited to large static populations; it is not automatically the best option for every population. [E2] | Use native actors/components and existing HISM placement for fixed scenery. Consider ISM versus HISM only against the actual target and content. Preserve explicit IDs and transforms regardless of batching. No new instancing framework. |
| Roads, paths and repeated linear objects | Spline Components represent paths; Spline Mesh Components deform an individual mesh between endpoints and tangents. [E4] | Reuse splines for authored paths and supported linear assemblies. Do not replace the current explicit road ribbon with default spline interpolation. Preserve control points, width, elevation, tangent/corner behavior and resulting geometry before substituting it. |
| Authored shapes and assemblies | Geometry Script exposes mesh operations through `UDynamicMesh`; Epic labels it Beta. Level Instances reuse actor assemblies, while Packed Level Blueprints optimize static mesh assemblies. [E3][E6] | Prefer existing StaticMesh assets. Use native geometry operations only where they reproduce required openings, silhouettes and transforms. Use Level Instances for reusable approved shells or scenery assemblies, not saved account furniture. Keep explicit meshes when a native generator would change the recipe. |
| Streaming | Native level streaming loads and unloads levels. World Partition uses a persistent world divided into streaming cells. Epic states that Level Instances outside a World Partition world do not automatically provide a streaming strategy. [E6][E7] | Retain the current bounded native streamed-level choice. Level Instances are an assembly mechanism, not an admission system. World Partition is an alternative only after an approved content decision, not a replacement implied by PCG partitioning or custom cell bins. |
| Foliage | Foliage Mode distinguishes instanced Static Mesh Foliage from Actor Foliage; actor foliage carries ordinary actor costs. PCG supports procedural point processing and spawning. [E1][E8] | Use native foliage/instancing for approved vegetation. PCG may distribute non-semantic vegetation inside approved areas and exclusions. Preserve exact authored plants and trees where their identity or placement belongs to the recipe. No custom foliage renderer. |
| Procedural placement | PCG graphs operate on spatial data and metadata, with mesh spawning and runtime generation facilities. Attribute-based mesh selection exists. [E1][E9] | PCG need not be stochastic. Exact supplied points and asset attributes can be a placement route if identity and fidelity survive. A graph is unnecessary when direct native placement already meets the recipe. |

Matching bounding boxes alone would not establish equivalence. Openings, source indices, material assignment, orientation and semantic mappings also matter. Any optimization that reorders instances or merges geometry must preserve the canonical mapping independently of generated actor names. [R2][R3]

## PCG Primitives, runtime and MCP limits

Documented facts:

- Epic labels PCG Primitives experimental. It is a world-building library of subgraphs, actions and examples, not merely basic geometric shapes. City Sample PCG demonstrates assemblies and building/road grammars. These are reuse references, not equivalent Baddie content or an approved sample dependency. [E5][R3]
- PCG documents editor and runtime generation modes. Geometry Script supports both, but its `Create New Static Mesh Asset From Mesh` node is editor-only. Unreal Python is editor scripting, not packaged gameplay execution. The pinned generator's asset import/save operations and the installed PCGToolset/Python interop therefore belong to editor authoring. This does not establish runtime support for every PCG Primitives graph dependency. [E3][E9][E10][R3][R4]
- Epic labels Unreal MCP experimental and describes editor tools exposed through toolsets. Discovery returns tools/descriptions; it does not establish the result of an operation. [E11]

Inference for this port: prefer an explicitly authorized editor/content pass that produces saved native assets. Do not add PCG Primitives, Python or MCP as shipping runtime dependencies. The editor-only PCGToolset and Python interop facts do not make PCG or PCG Primitives universally editor-only. Individual 5.8.3 graph/node dependencies, cook and target limits still need inspection. [R2][R4][E5]

MCP discovery, compatibility and capability proof are separate. An advertised tool cannot prove successful asset creation, persisted instancing or preserved recipe IDs. Even a successful read-only query proves only that query. A later authorized content operation must verify its resulting assets and saved evidence. No currently installed callable capability is claimed. [E11][R4]

## The adapter that must remain

Retain these responsibilities within the existing adapter, without adding a service or framework:

- Resolve approved canonical source, asset hashes and exact destination/layout/seed/revision identities. Preserve parcel and fixture IDs, original source indices, cameras, entrances, spawns and travel endpoints. Coordinate conversion and authored-versus-explicit-zero Home semantics remain exact. [R2][R3]
- Map those identities to native asset references and instance/feature records. Native mesh or graph identities do not replace Baddie's saved identities. Keep saved furniture, account room selection and gameplay ownership outside scenery generation. [R1][R3]
- Enforce the named human authorization, source/generator identity, target editor/project, exact jobs and operation permissions. Preserve immutable receipts, complete dependency hashes, ownership checks and explicit recovery decisions. The existing correction review, not this note, owns defects in those checks. [R2][R3]
- Leave access, movement admission, mission readiness and utility power with the existing runtime/backend owners. A map, generated point or streaming registration cannot grant them. [R1][R3]

Do not replace authored parcel positions, building variants, road shapes, openings, fixture indices, spawn/camera poses, room layouts or canonical seeds with stochastic PCG. A repeatable random seed does not prove the same authored result. Only approved non-semantic decoration may vary, within supplied constraints. [R2][R3][E1]

## Required authorized content pass and unresolved checks

This research authorizes none of the following. A named content pass still needs approval for mesh/material authoring or import, shader/derived-data processing, map saves, initialized-template duplication and streamed composition. It must supply the missing approved Home shells/openings, metric rural terrain and initialized Landscape content, derived road/scenery inputs and dependency evidence. Experimental nodes do not supply those missing inputs. Collision, navigation, lighting, material fidelity, rendered/player-height checks and target performance remain separate acceptance work; builds and releases remain separately authorized. [R2][R3]

Unresolved installed-source and acceptance checks:

1. Complete PCG, PCG Primitives and Geometry Script dependency/target compatibility for selected operations. R4 settles the inspected PCGToolset and Python interop descriptors, not loaded availability or every graph dependency. [R2][R4][E3][E5]
2. Which inspected functions/nodes are editor-only, which can run in a cooked target, and whether their actual signatures support the required mesh, instance, spline, foliage and level operations. Do not infer absence from an unsuccessful candidate-path lookup. [R2]
3. Live schemas remain unavailable at the recorded endpoint. The source-declared discovery functions identify future inspection candidates; no restart or launch follows from this note. Identity preservation and package persistence require later authorized evidence. [R4][E11]
4. Compatibility of any proposed assembly/streaming change with the current non-partitioned maps and intended mobile/static-lighting requirements. No World Partition conversion or PCG migration is selected by this assessment. [R3][E6][E7]

## Sources

Repository sources:

- R1. `/Users/joshua/Documents/ChatGPT/baddie.ng/docs/engineering/standalone-companion-unreal-port.md`, "Unreal-first implementation", read October 11, 2026.
- R2. `/Users/joshua/Documents/ChatGPT/baddie.ng/docs/engineering/unreal-world-generation-source-review-2026-10-11.md`, read October 11, 2026. Prior installed observations are attributed to this record, not re-observed.
- R3. Git commit `929a81b4779ffa07d2317c75eba1206516f9cb3b`, files under `/Users/joshua/Documents/ChatGPT/baddie.ng/apps/companion-unreal/Scripts/WorldGeneration/`. Read `README.md`, `geometry.py` and bounded excerpts of `recipes.py` and `native_generator.py` using `git show`. Line references name committed blobs, not dirty working files. The later review in R2 supersedes the pinned README's historical unresolved manifest-discrepancy statement.
- R4. Git commit `3f9ccc453d9b635c679f4af1e96b86c51faba56c`, `/Users/joshua/Documents/ChatGPT/baddie.ng/output/cribtest11-native-capability-2026-10-11/parent-source-inspection.json`. Read only the relevant listener, PCG descriptor/declaration and acceptance fields from the committed record. No remote calls.

Primary Epic references consulted October 11, 2026. URLs identify documentation, not verified installed API contracts.

```text
E1. PCG overview and node reference
https://dev.epicgames.com/documentation/unreal-engine/procedural-content-generation-overview?lang=en-US
https://dev.epicgames.com/documentation/unreal-engine/procedural-content-generation-framework-node-reference-in-unreal-engine?lang=en-US
E2. Instanced Static Mesh Component
https://dev.epicgames.com/documentation/unreal-engine/instanced-static-mesh-component-in-unreal-engine?lang=en-US
E3. Geometry Scripting user guide and reference
https://dev.epicgames.com/documentation/unreal-engine/geometry-scripting-users-guide-in-unreal-engine?lang=en-US
https://dev.epicgames.com/documentation/unreal-engine/geometry-scripting-reference-in-unreal-engine?lang=en-US
E4. Blueprint Spline Components overview
https://dev.epicgames.com/documentation/unreal-engine/blueprint-spline-components-overview-in-unreal-engine?lang=en-US
E5. PCG Primitives plugin index; City Sample PCG
https://dev.epicgames.com/documentation/unreal-engine/API/PluginIndex/PCGPrimitives?lang=en-US
https://dev.epicgames.com/documentation/unreal-engine/city-sample-pcg-for-unreal-engine?lang=en-US
E6. Level Instancing
https://dev.epicgames.com/documentation/unreal-engine/level-instancing-in-unreal-engine?lang=en-US
E7. Level Streaming overview; World Partition
https://dev.epicgames.com/documentation/unreal-engine/level-streaming-overview-in-unreal-engine?lang=en-US
https://dev.epicgames.com/documentation/unreal-engine/world-partition-in-unreal-engine?lang=en-US
E8. Foliage Mode
https://dev.epicgames.com/documentation/unreal-engine/foliage-mode-in-unreal-engine?lang=en-US
E9. PCG generation modes; attribute-based mesh selector
https://dev.epicgames.com/documentation/unreal-engine/using-pcg-generation-modes-in-unreal-engine?lang=en-US
https://dev.epicgames.com/documentation/unreal-engine/API/Plugins/PCG/UPCGMeshSelectorByAttribute?lang=en-US
E10. Scripting the Unreal Editor using Python
https://dev.epicgames.com/documentation/unreal-engine/scripting-the-unreal-editor-using-python?lang=en-US
E11. Unreal MCP
https://dev.epicgames.com/documentation/unreal-engine/unreal-mcp-in-unreal-editor?lang=en-US
https://dev.epicgames.com/documentation/unreal-engine/working-with-pcg-and-llms-using-unreal-mcp-in-unreal-engine?lang=en-US
```

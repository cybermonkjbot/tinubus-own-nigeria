# Native PCG recovery evidence

We inspected live editor PID 13584 and the expected Town Centre map before recovery. The sole dirty package was the owned unsaved PCG_MarketRoadKerbs34 graph with exactly the expected Create Points and Static Mesh Spawner nodes. No PCG actor existed; old Market Road kerbs were intact.

The original failure at PCGMeshSelectorWeightedEntry(descriptor=descriptor, weight=1) was TypeError: call() takes at most 1 argument (2 given). Recovery used the reflected default constructor and explicit descriptor/weight editor properties, completing the same graph in place. Generation finished with 1,141 instances and no generation in progress.

The first finish verified all transforms, mesh, material and BlockAll collision, removed the old owned batch and saved both maps. Receipt creation then failed because package_files(GRAPH, True) looked for a .umap instead of the actual .uasset. This was a post-save receipt error, not generation failure. Recovery verifies the old batch is absent, maps are clean and generated content still matches before writing the corrected receipt. No graph rerun, old-geometry recreation, map reset or editor restart was used.

The existing safe_level_change.load_saved helper rejected the Connected map namespace before attempting a map switch; it is scoped to the earlier Realism namespace. After releasing all inspected authoring globals containing Unreal Object wrappers and clearing standard exception references, we used the LevelEditorSubsystem directly on the exact saved Connected map. Reload succeeded without restart. Saved native readback verified 1,141 PCG instances, 6,378 support instances, unchanged source positions, zero maximum kerb location error and no dirty packages or active game world.

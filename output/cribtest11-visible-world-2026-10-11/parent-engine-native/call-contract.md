# Static scenery instancing handoff

October 11, 2026. `scenery_instances.py` is importable without Unreal or mutation.
`plan_instances(rows)` is pure; `author_instances(...)` is for the sole Windows
Unreal writer, inside its checked editor. This worker made no engine calls.

## Exact input

Each row has exactly these keys. References are existing full object paths.
`materials` covers every mesh slot in slot order. `custom_contract` lists ordered
semantic names; `custom_data` has the same number of finite floats.

```python
row = {
    "id": "yaba-frontage:window:001", "cell": "yaba-block-A:cell-0",
    "mesh": actual_mesh.get_path_name(),
    "materials": [actual_material.get_path_name()],
    "collision": "QUERY_AND_PHYSICS", "collision_profile": "BlockAll",
    "mobility": "STATIC", "cast_shadow": True,
    "mode": "ism", "nanite_qualified": False,
    "custom_contract": [], "custom_data": [],
    "location": [100.0, 200.0, 300.0],
    "rotation": [0.0, 0.0, 0.0, 1.0], "scale": [1.0, 1.0, 1.0],
}
plan = scenery_instances.plan_instances([row])
```

Transforms are already UE world centimetres, full unit quaternion XYZW and
positive XYZ scale. No basis, centimetre or axis conversion happens here.
The worker's measured FBX mapping for this import is
`Blender(x,y,z) -> UE(-100*y,-100*x,100*z)`. Apply it in that importer only,
including its rotation/scale basis handling. Do not pass raw Blender transforms
or use the canonical geographic transform to reinterpret these FBX instances.
Bake reflected geometry rather than sending negative scale.

The caller chooses `cell` to retain spatial granularity. Grouping includes cell,
mesh, ordered materials, collision/profile, mobility, shadow flag, mode and
custom-data semantics. Per-instance custom values and transforms do not split
groups. Source IDs sort within each group and map to actual returned indices.
The helper rejects duplicates and non-finite inputs before native mutation.

## Native call

```python
request = {
    "expected_pid": freshly_verified_editor_pid,
    "level_path": checked_current_level.get_path_name(),
    "level_owner": actual_world_owner_metadata,
    "level_identity": actual_world_identity_sha256,
    "owner": "cribtest11-scenery-instancing-20261011",
    "batch_id": "actual-unique-delivery-id",
    "save_level": False,
    "level_layout_evidence": "nonpartitioned-nonexternalized:" + actual_inspection_evidence,
}
journal = scenery_instances.author_instances(
    rows, request, owned_anchor_actor,
    {mesh.get_path_name(): mesh for mesh in already_loaded_meshes},
    {material.get_path_name(): material for material in already_loaded_materials},
)
```

The anchor must be in the current persistent level and carry `level_owner` as
an actor tag. The world must retain `Baddie.WorldGeneration.Owner` and
`Baddie.WorldGeneration.Identity`. This helper never claims an existing world
or retags an existing anchor. It checks exact level/world identity, PID, no PIE,
clean packages and every loaded mesh/material before creating anything.
Save unrelated existing edits separately first. Provide real inspected
nonpartitioned/nonexternalized evidence; Python layout queries were not verified
in this handoff. Partitioned, external-actor and streamed child authoring are
outside this bounded helper.

Only `save_level=True` requests saving that exact checked owned current level.
Default false leaves persistent actor-owned subobjects authored but unsaved.
The helper never saves mesh/material assets, other levels or project settings.
Its return is a journal dictionary, not a disk journal file. The caller saves
that evidence in its existing owned journal location and adds actual map/sidecar
hashes after saving. `saved-needs-reload-readback` reports the save API result,
not proof of component persistence across reload. Reload verification belongs
to the sole writer's authorized pass, not this source-only task.

Use a unique batch ID and do not replay a created or saved batch. There is no
global actor lookup, deduplication scan, asset deletion or overwrite mechanism.
Failure destroys only retained actors created in that call, including their
new subobjects. Unconfirmed destruction or a failed save returns
`partial-failure`. It never resaves a rollback or claims disk rollback. A clean
in-memory rollback can still leave the level dirty; resolve it explicitly.

Collision and shadows are caller decisions. Collision options are
`NO_COLLISION`, `QUERY_ONLY`, `PHYSICS_ONLY`, `QUERY_AND_PHYSICS`; the profile
must already exist. Profile responses, imported collision geometry and material
custom-data consumption need caller inspection. No physics geometry is created.
Qualified Nanite input requires ISM and enabled mesh Nanite settings. This does
not certify material/RHI support, Nanite residency or frame-time improvement.
HISM is optional static fallback, not an automatic optimization choice.
No cull-distance, renderer, gameplay, account, utility or runtime-admission
setting changes. Mesh topology, UVs and imported detail remain unchanged.

## API basis and remaining readback

The helper follows `NativeAuthor.component` and `instantiate_parts` in
`apps/companion-unreal/Scripts/WorldGeneration/native_generator.py`, using
`SubobjectDataSubsystem.add_new_subobject` on a new StaticMeshActor instead of
constructing a transient unattached component. The existing source lock records
installed UE 5.8.3 headers for ISM, HISM, subobjects and editor subsystems.
This Mac does not contain those installed Windows headers; no fresh installed
API read or live component-registration claim is made.

Epic's public Python documentation was read over HTTP on October 11, 2026:

- [ISM](https://dev.epicgames.com/documentation/en-us/unreal-engine/python-api/class/InstancedStaticMeshComponent) documents `add_instance`, `set_num_custom_data_floats`, `set_custom_data_value` and `get_instance_count`.
- [Subobjects](https://dev.epicgames.com/documentation/en-us/unreal-engine/python-api/class/SubobjectDataSubsystem) documents persistent actor subobject creation.
- [Level editor](https://dev.epicgames.com/documentation/en-us/unreal-engine/python-api/class/LevelEditorSubsystem) documents `get_current_level` and `save_current_level`.
- [Actor](https://dev.epicgames.com/documentation/en-us/unreal-engine/python-api/class/Actor) and [editor actors](https://dev.epicgames.com/documentation/en-us/unreal-engine/python-api/class/EditorActorSubsystem) document level membership and destruction.
- [Nanite settings](https://dev.epicgames.com/documentation/en-us/unreal-engine/python-api/class/MeshNaniteSettings) document `enabled`; the helper reads it, never changes it.

Public pages are not proof of the installed version's callable surface.
Unavailable APIs fail rather than being replaced with guessed calls.
`ActorComponent.is_registered` was not documented, so it is not invented here.
Native component ownership and instance count are read back; registration,
transforms, custom data, collision responses and post-save reload persistence
remain unverified live. No tests, builds, compilation, typechecks or lint ran.

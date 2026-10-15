"""Source-only handoff. Input transforms are already UE cm, quaternion XYZW.

No import-time Unreal calls. Native subobject registration/persistence still needs
sole-writer readback. Never apply the measured FBX basis conversion again here.
"""

import hashlib
import json
import math
import os
import re


def _digest(value):
    return hashlib.sha256(json.dumps(value, sort_keys=True, separators=(",", ":"), allow_nan=False).encode()).hexdigest()


def _text(value):
    if not isinstance(value, str) or not value or "\x00" in value:
        raise ValueError("Expected nonempty text")
    return value


def _numbers(value, count):
    if not isinstance(value, (list, tuple)) or len(value) != count:
        raise ValueError("Incorrect numeric array length")
    if any(type(v) not in (int, float) or not math.isfinite(v) for v in value):
        raise ValueError("Non-finite or nonnumeric input")
    return list(value)


def plan_instances(rows):
    """Pure plan: stable IDs, explicit spatial cells and complete batch contracts."""
    if not isinstance(rows, (list, tuple)) or not rows:
        raise ValueError("Expected a nonempty bounded batch")
    groups, seen = {}, set()
    for row in rows:
        fields = {"id", "cell", "mesh", "materials", "collision", "collision_profile", "mobility",
                  "cast_shadow", "mode", "nanite_qualified", "custom_contract", "custom_data", "location", "rotation", "scale"}
        if not isinstance(row, dict) or set(row) != fields:
            raise ValueError("Row must match the exact call contract")
        identifier = _text(row["id"])
        if identifier in seen:
            raise ValueError("Duplicate source ID: " + identifier)
        seen.add(identifier)
        for key in ("cell", "mesh", "collision_profile"):
            _text(row[key])
        for key in ("materials", "custom_contract"):
            if not isinstance(row[key], list):
                raise ValueError("Expected list: " + key)
            for value in row[key]:
                _text(value)
        if len(set(row["custom_contract"])) != len(row["custom_contract"]):
            raise ValueError("Duplicate custom-data semantic")
        if row["collision"] not in ("NO_COLLISION", "QUERY_ONLY", "PHYSICS_ONLY", "QUERY_AND_PHYSICS"):
            raise ValueError("Unsupported collision mode")
        if row["mobility"] != "STATIC" or row["mode"] not in ("ism", "hism"):
            raise ValueError("Only static authored ISM/HISM scenery is supported")
        if any(type(row[key]) is not bool for key in ("cast_shadow", "nanite_qualified")):
            raise ValueError("Flags must be explicit booleans")
        if row["nanite_qualified"] and row["mode"] != "ism":
            raise ValueError("Caller-qualified Nanite scenery requires ISM")
        location = _numbers(row["location"], 3)
        rotation = _numbers(row["rotation"], 4)
        scale = _numbers(row["scale"], 3)
        custom = _numbers(row["custom_data"], len(row["custom_contract"]))
        if abs(sum(v * v for v in rotation) - 1) > 1e-5 or any(v <= 0 for v in scale):
            raise ValueError("Require unit XYZW quaternion and positive scale; bake mirrors into mesh")
        contract = {key: row[key] for key in fields - {"id", "location", "rotation", "scale", "custom_data"}}
        key = _digest(contract)
        group = groups.setdefault(key, {"key": key, "contract": contract, "instances": []})
        group["instances"].append({"id": identifier, "location": location, "rotation": rotation, "scale": scale, "custom_data": custom})
    result = []
    for key in sorted(groups):
        group = groups[key]
        group["instances"].sort(key=lambda item: item["id"])
        result.append(group)
    return json.loads(json.dumps({"groups": result, "instance_count": len(seen)}, allow_nan=False))


def author_instances(rows, request, anchor_actor, mesh_refs, material_refs):
    """Create only new scenery actors in a checked owned persistent level.

    Caller passes already-loaded references and owns serialization/journalling.
    Failures return rollback evidence; they never delete source assets or worlds.
    """
    plan = plan_instances(rows)
    required = {"expected_pid", "level_path", "level_owner", "level_identity", "owner", "batch_id", "save_level", "level_layout_evidence"}
    if not isinstance(request, dict) or set(request) != required:
        raise ValueError("Request must match the exact call contract")
    if type(request["expected_pid"]) is not int or request["expected_pid"] != os.getpid():
        raise ValueError("Editor PID mismatch")
    for key in required - {"expected_pid", "save_level"}:
        _text(request[key])
    if type(request["save_level"]) is not bool or not re.fullmatch(r"[a-f0-9]{64}", request["level_identity"]):
        raise ValueError("Invalid save flag or level identity")
    if not re.fullmatch(r"/Game/BaddieGeneratedWorld/[A-Za-z0-9_/]+\.[A-Za-z0-9_]+:PersistentLevel", request["level_path"]):
        raise ValueError("Target must be an exact owned persistent-level object path")
    import unreal as u
    assets = u.get_editor_subsystem(u.EditorAssetSubsystem)
    actors = u.get_editor_subsystem(u.EditorActorSubsystem)
    levels = u.get_editor_subsystem(u.LevelEditorSubsystem)
    editor = u.get_editor_subsystem(u.UnrealEditorSubsystem)
    subobjects = u.get_engine_subsystem(u.SubobjectDataSubsystem)
    lib = u.SubobjectDataBlueprintFunctionLibrary
    level, world = levels.get_current_level(), editor.get_editor_world()
    if level is None or world is None or not isinstance(anchor_actor, u.Actor):
        raise ValueError("Missing live level/world/anchor")
    if not isinstance(mesh_refs, dict) or not isinstance(material_refs, dict):
        raise ValueError("Loaded reference dictionaries are required")
    def checked_level():
        if (os.getpid() != request["expected_pid"] or editor.get_game_world() is not None
                or levels.get_current_level() != level or editor.get_editor_world() != world
                or level.get_path_name() != request["level_path"] or level.get_outer() != world
                or anchor_actor.get_level() != level or anchor_actor.is_actor_being_destroyed()
                or request["level_owner"] not in [str(t) for t in anchor_actor.get_editor_property("tags")]
                or assets.get_metadata_tag(world, "Baddie.WorldGeneration.Owner") != request["level_owner"]
                or assets.get_metadata_tag(world, "Baddie.WorldGeneration.Identity") != request["level_identity"]):
            raise ValueError("Target level, owner, anchor, world identity or PIE state changed")
    checked_level()
    # Python does not expose a verified external-actor/layout query in this handoff.
    # Sole writer must supply actual inspected layout evidence, not an invented flag.
    prefix = "nonpartitioned-nonexternalized:"
    if not request["level_layout_evidence"].startswith(prefix) or not request["level_layout_evidence"][len(prefix):].strip():
        raise ValueError("Require writer's inspected nonpartitioned/nonexternalized level evidence")
    if u.EditorLoadingAndSavingUtils.get_dirty_map_packages() or u.EditorLoadingAndSavingUtils.get_dirty_content_packages():
        raise ValueError("Save/discard unrelated edits separately before this batch")
    for group in plan["groups"]:
        spec = group["contract"]
        mesh = mesh_refs[spec["mesh"]]
        if not isinstance(mesh, u.StaticMesh) or mesh.get_path_name() != spec["mesh"]:
            raise ValueError("Loaded mesh reference mismatch")
        if len(mesh.get_editor_property("static_materials")) != len(spec["materials"]):
            raise ValueError("Require complete ordered material-slot bindings")
        for path in spec["materials"]:
            material = material_refs[path]
            if not isinstance(material, u.MaterialInterface) or material.get_path_name() != path:
                raise ValueError("Loaded material reference mismatch")
        if spec["nanite_qualified"] and not mesh.get_editor_property("nanite_settings").enabled:
            raise ValueError("Caller-qualified Nanite mesh has Nanite disabled")
        cls = u.InstancedStaticMeshComponent if spec["mode"] == "ism" else u.HierarchicalInstancedStaticMeshComponent
        getattr(u.CollisionEnabled, spec["collision"])
        for method in ("add_instance", "set_num_custom_data_floats", "set_custom_data_value", "get_instance_count"):
            if not hasattr(cls, method):
                raise RuntimeError("Unavailable installed component API: " + method)
    journal = {"state": "authoring", "owner": request["owner"], "batch_id": request["batch_id"],
               "plan_sha256": _digest(plan), "level_path": request["level_path"], "pid": os.getpid(),
               "groups": [{"key": g["key"], **g["contract"]} for g in plan["groups"]],
               "instances": [], "saved_level": None, "existing_assets_modified": [], "runtime_authority": False}
    created = []
    try:
        for group in plan["groups"]:
            checked_level()
            spec = group["contract"]
            actor = actors.spawn_actor_from_class(u.StaticMeshActor, u.Vector(0, 0, 0), u.Rotator(0, 0, 0), transient=False)
            if actor is None:
                raise RuntimeError("New scenery actor spawn failed")
            created.append(actor)
            actor.set_editor_property("tags", [u.Name(request["owner"]), u.Name("Baddie.SceneryBatch=" + request["batch_id"]), u.Name("Baddie.RuntimeAuthority=False")])
            actor.set_actor_label("Scenery_" + group["key"][:20])
            if actor.get_level() != level:
                raise RuntimeError("New actor spawned into another level")
            root = actor.get_component_by_class(u.StaticMeshComponent)
            root.set_editor_property("component_tags", [u.Name(request["owner"])])
            root.set_mobility(u.ComponentMobility.STATIC)
            handles = subobjects.k2_gather_subobject_data_for_instance(actor)
            if not handles:
                raise RuntimeError("No persistent actor subobject handle")
            cls = u.InstancedStaticMeshComponent if spec["mode"] == "ism" else u.HierarchicalInstancedStaticMeshComponent
            handle, reason = subobjects.add_new_subobject(u.AddNewSubobjectParams(parent_handle=handles[0], new_class=cls, blueprint_context=None))
            if not lib.is_handle_valid(handle):
                raise RuntimeError("Persistent component creation failed: " + str(reason))
            component = lib.get_associated_object(lib.get_data(handle))
            if not isinstance(component, cls) or component.get_owner() != actor:
                raise RuntimeError("New component owner/type mismatch")
            component.set_editor_property("component_tags", [u.Name(request["owner"]), u.Name("Baddie.Cell=" + spec["cell"])])
            component.set_mobility(u.ComponentMobility.STATIC)
            component.set_static_mesh(mesh_refs[spec["mesh"]])
            for slot, path in enumerate(spec["materials"]):
                component.set_material(slot, material_refs[path])
            component.set_collision_profile_name(u.Name(spec["collision_profile"]))
            component.set_collision_enabled(getattr(u.CollisionEnabled, spec["collision"]))
            component.set_editor_property("cast_shadow", spec["cast_shadow"])
            if (component.get_editor_property("static_mesh") != mesh_refs[spec["mesh"]]
                    or str(component.get_collision_profile_name()) != spec["collision_profile"]
                    or component.get_collision_enabled() != getattr(u.CollisionEnabled, spec["collision"])):
                raise RuntimeError("Mesh/collision contract readback mismatch")
            component.set_num_custom_data_floats(len(spec["custom_contract"]))
            for expected_index, item in enumerate(group["instances"]):
                transform = u.Transform(location=u.Vector(*item["location"]), rotation=u.Quat(*item["rotation"]), scale=u.Vector(*item["scale"]))
                index = component.add_instance(transform, world_space=True)
                if index != expected_index:
                    raise RuntimeError("Unexpected source-to-instance ordering")
                for offset, value in enumerate(item["custom_data"]):
                    if not component.set_custom_data_value(index, offset, value, mark_render_state_dirty=True):
                        raise RuntimeError("Per-instance custom-data assignment failed")
                journal["instances"].append({"source_id": item["id"], "cell": spec["cell"], "group": group["key"], "actor": actor.get_path_name(), "component": component.get_path_name(), "instance_index": index})
            if component.get_instance_count() != len(group["instances"]):
                raise RuntimeError("Instance-count readback mismatch")
        checked_level()
        journal["state"] = "created-unsaved"
        if request["save_level"]:
            journal["state"] = "save-attempted"
            if not levels.save_current_level():
                raise RuntimeError("Explicit current-level save failed; disk state requires inspection")
            journal.update(state="saved-needs-reload-readback", saved_level=world.get_path_name())
        return journal
    except Exception as error:
        save_attempted = journal["state"] == "save-attempted"
        survivors = []
        for actor in reversed(created):
            try:
                if not actors.destroy_actor(actor):
                    survivors.append(actor.get_path_name())
            except Exception:
                survivors.append("new-actor-destruction-unconfirmed")
        journal.update(state="partial-failure" if survivors or save_attempted else "rolled-back-unsaved",
                       error=str(error), surviving_new_actors=survivors, disk_state_unconfirmed=save_attempted)
        return journal

"""Camera-only combined frame from the preserved v3 first-scene checkpoint."""
import argparse
from datetime import datetime, timezone
import hashlib
import json
import math
import os
from pathlib import Path
import runpy
import sys
import time
import traceback

import bpy
from bpy_extras.object_utils import world_to_camera_view
from mathutils import Vector

JOB = "lagos-first-scene-camera-v1"
ROOT = Path("C:/dev/cribtest-game/output/cribtest11-visible-world-2026-10-11/street-props")
SOURCE = ROOT / "first-scene-v3/LagosFirstScene.blend"
SOURCE_HASH = "dc21105829a56780000bc01043c2a0c0e1adc932ebe113085f75a3d3cbd7e3bb"
DIRECTORY = ROOT / "camera-delivery-v1"


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def write_json(path, record):
    temporary = path.with_name(path.name + ".tmp")
    temporary.write_text(json.dumps(record, indent=2) + "\n", encoding="utf-8")
    temporary.replace(path)


def progress(step, **values):
    record = {"job": JOB, "pid": os.getpid(), "step": step,
              "updated_at": datetime.now(timezone.utc).isoformat(), **values}
    write_json(DIRECTORY / "progress.json", record)
    print(json.dumps(record), flush=True)


def instance_bounds(graph, parent_name):
    points = []
    count = 0
    for item in graph.object_instances:
        if item.is_instance and item.parent and item.parent.name == parent_name and item.object.type in {"MESH", "FONT"}:
            count += 1
            points.extend(item.matrix_world @ Vector(corner) for corner in item.object.bound_box)
    if not points:
        raise RuntimeError("No evaluated geometry for " + parent_name)
    minimum = [min(point[axis] for point in points) for axis in range(3)]
    maximum = [max(point[axis] for point in points) for axis in range(3)]
    return {"object": parent_name, "evaluated_parts": count,
            "blender_world_bounds_metres": [minimum, maximum],
            "source_world_bounds_metres": [[p[0], p[2], p[1]] for p in (minimum, maximum)]}, points


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--owned-background-job", required=True)
    parser.add_argument("--probe-script", required=True)
    args = parser.parse_args(sys.argv[sys.argv.index("--") + 1:])
    if not bpy.app.background or os.getpid() == 3224:
        raise RuntimeError("Refusing live/protected Blender session")
    if args.owned_background_job != JOB or os.environ.get("CRIBTEST_BLENDER_OWNER_JOB") != JOB:
        raise RuntimeError("Camera delivery ownership mismatch")
    if Path(bpy.data.filepath).resolve() != SOURCE.resolve() or digest(SOURCE) != SOURCE_HASH:
        raise RuntimeError("Preserved v3 checkpoint identity mismatch")
    frame = DIRECTORY / "03-Theatre-Danfo-Kiosk.png"
    saved = DIRECTORY / "LagosFirstScene-CameraDelivery.blend"
    if frame.exists() or saved.exists():
        raise FileExistsError("Camera delivery exists; preserve previous outputs")
    try:
        scene = bpy.data.scenes["Lagos First Scene - National Theatre"]
        bpy.context.window.scene = scene
        bpy.context.view_layer.update()
        progress("inspecting_saved_v3_instance_bounds", source=str(SOURCE), source_sha256=SOURCE_HASH)
        probe = runpy.run_path(args.probe_script)["inspect_scene"]
        write_json(DIRECTORY / "scene-probe-reopened-v3.json", probe(limit=12))
        graph = bpy.context.evaluated_depsgraph_get()
        danfo, danfo_points = instance_bounds(graph, "Owned dressing Danfo 0")
        kiosk, kiosk_points = instance_bounds(graph, "Owned dressing Kiosk 1")
        placement_before = {obj.name: list(sum((tuple(row) for row in obj.matrix_world), ()))
                            for obj in scene.objects if obj.get("placement_kind")}
        base_provenance = json.loads((SOURCE.parent / "provenance-manifest.json").read_text(encoding="utf-8"))
        source_transit = Path(base_provenance["source_files"]["lagos-connected-transit-v1.json"]["path"])
        transit = json.loads(source_transit.read_text(encoding="utf-8"))["transit"]
        stop = next(record for record in transit["stops"] if record["id"] == "brt-stop:theatre-ring-drive:1:340")
        data = bpy.data.cameras.new("03-Theatre-Danfo-Kiosk")
        camera = bpy.data.objects.new(data.name, data)
        scene.collection.children["Cameras and light"].objects.link(camera)
        # The camera is north of both source poses, so both props lie between
        # it and the theatre. This is an editor-only eye, not a world placement.
        eye_source = [-2220.0, 2.0, 1594.0]
        target_source = [-2175.0, 14.0, 1360.0]
        camera.location = (eye_source[0], eye_source[2], eye_source[1])
        camera.rotation_euler = (Vector((target_source[0], target_source[2], target_source[1])) - camera.location).to_track_quat("-Z", "Y").to_euler()
        data.lens = 28
        data.clip_start, data.clip_end = 0.1, 2500
        camera["authoring_job"] = JOB
        camera["placement_kind"] = "editor-only camera; no canonical entity or runtime state"
        camera["source_eye_json"] = json.dumps(eye_source)
        camera["source_target_json"] = json.dumps(target_source)
        scene.camera = camera
        scene.render.engine = "CYCLES"
        scene.cycles.device, scene.cycles.samples = "CPU", 16
        scene.cycles.use_denoising = True
        scene.render.threads_mode, scene.render.threads = "FIXED", 2
        scene.render.resolution_x, scene.render.resolution_y = 960, 540
        scene.render.resolution_percentage = 100
        scene.render.image_settings.file_format = "PNG"
        scene.render.image_settings.color_mode, scene.render.image_settings.color_depth = "RGB", "8"
        temporary = frame.with_name(frame.stem + ".partial.png")
        scene.render.filepath = str(temporary)
        bpy.context.view_layer.update()
        projected = {}
        for name, points in (("Danfo", danfo_points), ("Kiosk", kiosk_points)):
            projected[name] = [list(world_to_camera_view(scene, camera, point)) for point in points]
            if not all(point[2] > 0 for point in projected[name]):
                raise RuntimeError(name + " is not fully in front of the camera")
        bpy.ops.wm.save_as_mainfile(filepath=str(saved))
        # Reopen the camera deliverable, not merely the source scene, before render.
        bpy.ops.wm.open_mainfile(filepath=str(saved))
        scene = bpy.data.scenes["Lagos First Scene - National Theatre"]
        bpy.context.window.scene = scene
        camera = scene.objects["03-Theatre-Danfo-Kiosk"]
        scene.camera = camera
        bpy.context.view_layer.update()
        for name, original in placement_before.items():
            current = list(sum((tuple(row) for row in scene.objects[name].matrix_world), ()))
            if current != original:
                raise RuntimeError("Prop pose changed in camera-only save: " + name)
        diagnostic = probe(limit=12)
        write_json(DIRECTORY / "scene-probe-camera-delivery.json", diagnostic)
        if diagnostic["assets"]["missing_count"]:
            raise RuntimeError("Reopened camera delivery has missing assets")
        manifest = {"job": JOB, "pid": os.getpid(), "source_blend": str(SOURCE),
                    "source_blend_sha256": SOURCE_HASH, "script_sha256": digest(Path(__file__)),
                    "saved_blend": str(saved), "saved_blend_sha256": digest(saved),
                    "stop": {"id": stop["id"], "berth": stop["berth"], "shelter": stop["shelter"]},
                    "evaluated_bounds": [danfo, kiosk], "prop_poses_unchanged": True,
                    "camera": {"name": camera.name, "source_eye": eye_source,
                               "source_target": target_source, "lens_mm": camera.data.lens,
                               "canonical_id": None, "placement": "editor-only roadside presentation eye"},
                    "projected_prop_bounds": {name: [[min(p[i] for p in points) for i in range(3)],
                                                          [max(p[i] for p in points) for i in range(3)]]
                                              for name, points in projected.items()},
                    "render": {"engine": "CYCLES", "device": "CPU", "threads": 2,
                               "samples": 16, "resolution": [960, 540]},
                    "frame": None, "visual_acceptance": "pending actual frame review", "errors": []}
        # Camera datablocks from before reopening are invalid; use the reopened one.
        manifest["camera"]["lens_mm"] = camera.data.lens
        write_json(DIRECTORY / "camera-provenance.json", manifest)
        progress("rendering_combined_camera", saved_blend=str(saved), bounds=[danfo, kiosk],
                 prop_poses_unchanged=True, source_eye=eye_source)
        started = time.monotonic()
        bpy.ops.render.render(write_still=True, scene=scene.name)
        seconds = time.monotonic() - started
        image = bpy.data.images.load(str(temporary), check_existing=False)
        try:
            if list(image.size) != [960, 540] or not image.has_data:
                raise RuntimeError("Combined PNG did not decode at requested dimensions")
        finally:
            bpy.data.images.remove(image)
        temporary.replace(frame)
        manifest["frame"] = {"path": str(frame), "sha256": digest(frame), "seconds": seconds,
                               "bytes": frame.stat().st_size, "dimensions": [960, 540]}
        write_json(DIRECTORY / "camera-provenance.json", manifest)
        if digest(SOURCE) != SOURCE_HASH:
            raise RuntimeError("Preserved source checkpoint changed")
        progress("combined_frame_delivered_pending_visual_review", frame=manifest["frame"],
                 paths=[str(saved), str(frame), str(DIRECTORY / "camera-provenance.json")],
                 hashes={str(saved): digest(saved), str(frame): digest(frame)}, errors=[])
    except Exception:
        progress("failed", error=traceback.format_exc())
        raise


if __name__ == "__main__":
    main()

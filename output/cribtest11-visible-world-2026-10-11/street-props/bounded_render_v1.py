"""Inspect the saved v1 asset library and render only its two assigned cameras."""
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
from mathutils import Vector

JOB = "lagos-street-props-v1"
EXPECTED_SCRIPT_HASH = "c25297c16a6fb24752d9fb13fb651d5be7683c92a530e3a4758365061b20e877"
EXPECTED_OUTPUT = Path("C:/dev/cribtest-game/output/cribtest11-visible-world-2026-10-11/street-props/v1")


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def atomic_json(path, value):
    temporary = path.with_name(path.name + ".tmp")
    temporary.write_text(json.dumps(value, indent=2) + "\n", encoding="utf-8")
    temporary.replace(path)


def progress(path, step, **changes):
    value = json.loads(path.read_text(encoding="utf-8"))
    value.update(updated_at=datetime.now(timezone.utc).isoformat(), pid=os.getpid(),
                 render_pid=os.getpid(), status="rendering", step=step)
    value.update(changes)
    atomic_json(path, value)
    print(json.dumps({"pid": os.getpid(), "step": step}), flush=True)


def inspect_assets(scene, manifest):
    graph = bpy.context.evaluated_depsgraph_get()
    records = []
    for record in manifest["assets"]:
        root = scene.objects.get(record["root"])
        collection = bpy.data.collections.get(record["collection"])
        if root is None or collection is None or root.get("authoring_job") != JOB:
            raise RuntimeError("Saved asset identity mismatch: " + record["asset"])
        inverse = root.matrix_world.inverted()
        minimum = [math.inf] * 3
        maximum = [-math.inf] * 3
        parts = [obj for obj in collection.objects if obj.type in {"MESH", "FONT"}]
        if len(parts) != record["editable_parts"]:
            raise RuntimeError("Saved part mapping mismatch: " + record["asset"])
        for obj in parts:
            evaluated = obj.evaluated_get(graph)
            for corner in evaluated.bound_box:
                point = inverse @ evaluated.matrix_world @ Vector(corner)
                if not all(math.isfinite(value) for value in point):
                    raise RuntimeError("Nonfinite bound: " + obj.name)
                for axis in range(3):
                    minimum[axis] = min(minimum[axis], point[axis])
                    maximum[axis] = max(maximum[axis], point[axis])
        fbx = EXPECTED_OUTPUT / record["fbx"]
        if digest(fbx) != record["fbx_sha256"]:
            raise RuntimeError("Export hash mismatch: " + fbx.name)
        suspended = record["asset"] == "WireSpan8m"
        records.append({"asset": record["asset"], "root": root.name,
                        "editable_parts": len(parts), "root_scale": list(root.scale),
                        "bounds_local_metres": [minimum, maximum],
                        "dimensions_metres": [maximum[i] - minimum[i] for i in range(3)],
                        "contact": record["contact"], "suspended": suspended,
                        "base_within_1cm_of_local_ground": None if suspended else abs(minimum[2]) <= 0.01,
                        "fbx": record["fbx"], "fbx_sha256": record["fbx_sha256"],
                        "export_mesh_identity": "SM_" + record["asset"]})
    return records


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output-dir", required=True)
    parser.add_argument("--owned-background-job", required=True)
    parser.add_argument("--probe-script", required=True)
    args = parser.parse_args(sys.argv[sys.argv.index("--") + 1:])
    directory = Path(args.output_dir).resolve()
    checkpoint = directory / "LagosStreetProps.blend"
    progress_path = directory.parent / "progress.json"
    if directory != EXPECTED_OUTPUT.resolve():
        raise RuntimeError("Output must be the assigned owned v1 directory")
    if not bpy.app.background or os.getpid() == 3224:
        raise RuntimeError("Refusing a live or protected Blender process")
    if args.owned_background_job != JOB or os.environ.get("CRIBTEST_BLENDER_OWNER_JOB") != JOB:
        raise RuntimeError("Missing owned-job handshake")
    if Path(bpy.data.filepath).resolve() != checkpoint.resolve():
        raise RuntimeError("Only the saved v1 deliverable may be inspected")
    if digest(directory.parent / "lagos_street_props.py") != EXPECTED_SCRIPT_HASH:
        raise RuntimeError("Frozen source changed")
    try:
        progress(progress_path, "saved_deliverable_reopened_for_asset_inspection")
        manifest = json.loads((directory / "props-manifest.json").read_text(encoding="utf-8"))
        scene = bpy.data.scenes["LagosStreetProps"]
        bpy.context.window.scene = scene
        if scene.unit_settings.system != "METRIC" or scene.unit_settings.scale_length != 1.0:
            raise RuntimeError("Saved metre unit contract changed")
        bpy.context.view_layer.update()
        probe = runpy.run_path(args.probe_script)["inspect_scene"]
        atomic_json(directory / "scene-probe-reopened.json", probe(limit=12))
        assets = inspect_assets(scene, manifest)
        atomic_json(directory / "asset-inspection.json", {
            "pid": os.getpid(), "saved_blend": str(checkpoint), "assets": assets,
            "external_caches": "No simulation, tiled/sequence images or libraries authored by frozen v1.",
            "export_review": "FBX hashes and source identity mapping checked; Unreal translation belongs to parent.",
            "uv_note": "Editable meshes have planar metre UVs; frozen v1 merged FBX export does not copy UV layers."
        })
        scene.render.engine = "CYCLES"
        scene.cycles.device = "CPU"
        scene.cycles.samples = 16
        scene.cycles.use_denoising = True
        scene.render.threads_mode = "FIXED"
        scene.render.threads = 2
        scene.render.resolution_x = 960
        scene.render.resolution_y = 540
        scene.render.resolution_percentage = 100
        scene.render.image_settings.file_format = "PNG"
        scene.render.image_settings.color_mode = "RGB"
        scene.render.image_settings.color_depth = "8"
        scene.render.use_file_extension = True
        configuration = {
            "job": JOB, "pid": os.getpid(), "blender_version": bpy.app.version_string,
            "blender_build": bpy.app.build_hash.decode(errors="replace"),
            "saved_blend": str(checkpoint), "saved_blend_sha256": digest(checkpoint),
            "frozen_script_sha256": EXPECTED_SCRIPT_HASH,
            "render_script_sha256": digest(Path(__file__)),
            "probe_script_sha256": digest(Path(args.probe_script)),
            "engine": "CYCLES", "device": "CPU", "threads": 2, "samples": 16,
            "resolution": [960, 540], "resolution_percentage": 100,
            "denoising": True, "max_bounces": scene.cycles.max_bounces,
            "color": {"view_transform": scene.view_settings.view_transform,
                      "look": scene.view_settings.look, "exposure": scene.view_settings.exposure,
                      "display": scene.display_settings.display_device},
            "checkpoint_policy": "Saved v1 geometry unchanged; bounded render settings recorded in this companion manifest.",
            "cameras": ["DanfoView", "LibraryView"], "frames": [], "errors": [],
            "visual_review": "pending actual image inspection and parent review"
        }
        config_path = directory / "bounded-render-manifest.json"
        if config_path.exists():
            raise FileExistsError("Render manifest exists; preserve previous delivery")
        atomic_json(config_path, configuration)
        atomic_json(directory / "scene-probe-bounded.json", probe(limit=12))
        for name in configuration["cameras"]:
            if (directory / "STOP").exists():
                raise RuntimeError("Owned STOP marker present between frames")
            target = directory / (name + ".png")
            temporary = directory / (name + ".partial.png")
            if target.exists() or temporary.exists():
                raise FileExistsError("Frame already exists: " + target.name)
            view = scene.objects.get(name)
            if view is None or view.type != "CAMERA":
                raise RuntimeError("Saved camera missing: " + name)
            scene.camera = view
            scene.render.filepath = str(temporary)
            progress(progress_path, "rendering_" + name, render_configuration=str(config_path),
                     technical_acceptance="saved_scene_reopened_and_bounded_asset_probe_complete")
            started = time.perf_counter()
            bpy.ops.render.render(write_still=True, scene=scene.name)
            seconds = time.perf_counter() - started
            image = bpy.data.images.load(str(temporary), check_existing=False)
            try:
                dimensions = list(image.size)
                if dimensions != [960, 540] or not image.has_data:
                    raise RuntimeError("Rendered PNG dimensions or decoded data invalid")
            finally:
                bpy.data.images.remove(image)
            temporary.replace(target)
            configuration["frames"].append({"camera": name, "path": str(target),
                                             "sha256": digest(target), "bytes": target.stat().st_size,
                                             "dimensions": dimensions, "seconds": seconds,
                                             "camera_location": list(view.location), "lens_mm": view.data.lens})
            atomic_json(config_path, configuration)
            progress(progress_path, "saved_" + name, rendered_frames=configuration["frames"])
        if digest(checkpoint) != configuration["saved_blend_sha256"]:
            raise RuntimeError("Saved v1 checkpoint changed during rendering")
        paths = [checkpoint, directory / "props-manifest.json", config_path,
                 directory / "asset-inspection.json", directory / "scene-probe-reopened.json",
                 directory / "scene-probe-bounded.json"]
        paths += [directory / record["fbx"] for record in manifest["assets"]]
        paths += [Path(record["path"]) for record in configuration["frames"]]
        progress(progress_path, "assets_and_two_frames_delivered", status="render_complete_pending_parent_review",
                 paths=[str(path) for path in paths], hashes={str(path): digest(path) for path in paths},
                 technical_acceptance="saved_scene_reopened_assets_mapped_fbx_hashes_checked_two_actual_pngs_decoded",
                 visual_acceptance="pending_actual_image_inspection_and_parent_review", errors=[])
    except Exception:
        error = traceback.format_exc()
        progress(progress_path, "owned_inspection_or_render_error", status="failed", errors=[error])
        raise


if __name__ == "__main__":
    main()

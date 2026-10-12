"""Capture the prepared town-centre streetpass views through the live editor.

This keeps the bridge and native viewport capture serial: first prepare an
exact viewport through the PID-bound Python bridge, then capture one PNG
through the existing loopback MCP tool.
"""
import argparse
import base64
from datetime import datetime, timezone
import hashlib
import importlib.util
import json
from pathlib import Path
import struct
import subprocess
import sys
import time

sys.dont_write_bytecode = True
ROOT = Path("C:/dev/cribtest-game")
SCRIPTS = ROOT / "apps/companion-unreal/Scripts"
ENGINE = "C:/Program Files/Epic Games/UE_5.8/Engine"
DEFAULT_OUT_DIR = ROOT / "output/cribtest11-visible-world-2026-10-11/previews/town-centre-streetpass01-refine01-daylight"
DEFAULT_RECEIPT = ROOT / "output/cribtest11-visible-world-2026-10-11/town-centre-street-pass-02-refine01.json"
OUT_DIR = DEFAULT_OUT_DIR
RECEIPT = DEFAULT_RECEIPT
sys.path.insert(0, str(SCRIPTS))
from mcp_client import UnrealMcp

EXTRA_CAMERAS = {
    "market-road-source-centre": {
        "id": "market-road-source-centre",
        "locationCm": [-7870.0, -25900.0, 165.0],
        "rotation": {"pitch": 2.0, "yaw": 54.4, "roll": 0.0},
        "fov": 68.0,
    },
    "market-road-frontage-a": {
        "id": "market-road-frontage-a",
        "locationCm": [-9800.0, -24800.0, 165.0],
        "rotation": {"pitch": 2.0, "yaw": 56.0, "roll": 0.0},
        "fov": 68.0,
    },
    "market-road-frontage-b": {
        "id": "market-road-frontage-b",
        "locationCm": [-12100.0, -26300.0, 165.0],
        "rotation": {"pitch": 2.0, "yaw": 34.0, "roll": 0.0},
        "fov": 68.0,
    },
    "civic-terrace-open-a": {
        "id": "civic-terrace-open-a",
        "locationCm": [-31600.0, 2200.0, 165.0],
        "rotation": {"pitch": 2.0, "yaw": 350.0, "roll": 0.0},
        "fov": 68.0,
    },
    "civic-terrace-open-b": {
        "id": "civic-terrace-open-b",
        "locationCm": [-26700.0, -5200.0, 165.0],
        "rotation": {"pitch": 2.0, "yaw": 18.0, "roll": 0.0},
        "fov": 68.0,
    },
}

BASE_CAMERAS = {
    "market-road-junction": {
        "id": "market-road-junction",
        "locationCm": [-18200.0, -23600.0, 165.0],
        "rotation": {"pitch": 2.5, "yaw": 34.0, "roll": 0.0},
        "fov": 68.0,
    },
    "library-lane-approach": {
        "id": "library-lane-approach",
        "locationCm": [-76000.0, -25800.0, 165.0],
        "rotation": {"pitch": 2.0, "yaw": 55.0, "roll": 0.0},
        "fov": 70.0,
    },
    "civic-street-approach": {
        "id": "civic-street-approach",
        "locationCm": [-28400.0, -2600.0, 165.0],
        "rotation": {"pitch": 2.0, "yaw": 8.0, "roll": 0.0},
        "fov": 68.0,
    },
}


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def bridge(expected_pid, code):
    request = json.dumps({"expected_pid": expected_pid, "code": code})
    process = subprocess.run(
        [
            sys.executable,
            "-B",
            str(SCRIPTS / "python_bridge.py"),
            "--engine",
            ENGINE,
            "--project-dir",
            str(ROOT / "apps/companion-unreal"),
        ],
        input=request,
        capture_output=True,
        text=True,
        check=True,
    )
    payload = json.loads(process.stdout)
    result = payload["result"]
    assert result["success"], result
    infos = []
    for entry in result.get("output", []):
        text = entry.get("output", "").strip()
        if entry.get("type") == "Info" and text.startswith("{"):
            infos.append(json.loads(text))
    return infos


def load_receipt():
    receipt = json.loads(RECEIPT.read_text(encoding="utf-8"))
    assert receipt["mapPackage"].endswith("L_town_centre_neighbourhood_20261011_streetpass01")
    return receipt


def camera_specs():
    receipt = load_receipt()
    base_cameras = receipt.get("cameras")
    if not base_cameras:
        base_cameras = list(BASE_CAMERAS.values())
    specs = {camera["id"]: camera for camera in base_cameras}
    specs.update(EXTRA_CAMERAS)
    return specs


def same_pose(actual, camera):
    location = actual.get("location")
    rotation = actual.get("rotation")
    fov = actual.get("fov")
    assert location is not None and rotation is not None and fov is not None
    for value, expected in zip(location, camera["locationCm"]):
        assert abs(value - expected) < 0.05, (location, camera["locationCm"])
    for axis in ("pitch", "yaw", "roll"):
        delta = (rotation[axis] - camera["rotation"][axis] + 180.0) % 360.0 - 180.0
        assert abs(delta) < 0.001, (rotation, camera["rotation"])
    assert abs(fov - camera["fov"]) < 0.01


def prepare_camera(expected_pid, camera_id):
    cameras = camera_specs()
    camera = cameras[camera_id]
    receipt_cameras = load_receipt().get("cameras") or []
    if camera_id in {camera["id"] for camera in receipt_cameras}:
        code = f"""
import json
import importlib.util
from pathlib import Path
path = Path(r"C:/dev/cribtest-game/apps/companion-unreal/Scripts/WorldGeneration/Connected/town_centre_street_pass.py")
spec = importlib.util.spec_from_file_location("town_centre_street_pass", path)
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)
module.prepare_camera({camera_id!r})
import unreal
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
key = levels.get_active_viewport_config_key()
location, rotation = levels.get_level_viewport_camera_info(key)
print(json.dumps({{
    "world": world.get_path_name().split(".")[0],
    "frame": unreal.SystemLibrary.get_frame_count(),
    "location": [location.x, location.y, location.z],
    "rotation": {{"pitch": rotation.pitch, "yaw": rotation.yaw, "roll": rotation.roll}},
    "fov": levels.get_level_viewport_fov(key)
}}))
"""
    else:
        code = f"""
import json
import unreal
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
key = levels.get_active_viewport_config_key()
levels.set_level_viewport_camera_info(
    unreal.Vector(*{camera["locationCm"]!r}),
    unreal.Rotator(
        pitch={camera["rotation"]["pitch"]!r},
        yaw={camera["rotation"]["yaw"]!r},
        roll={camera["rotation"]["roll"]!r},
    ),
    key,
)
levels.set_level_viewport_fov({camera["fov"]!r}, key)
levels.editor_set_game_view(True, key)
levels.editor_set_viewport_realtime(True)
levels.editor_invalidate_viewports()
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
location, rotation = levels.get_level_viewport_camera_info(key)
print(json.dumps({{
    "world": world.get_path_name().split(".")[0],
    "frame": unreal.SystemLibrary.get_frame_count(),
    "location": [location.x, location.y, location.z],
    "rotation": {{"pitch": rotation.pitch, "yaw": rotation.yaw, "roll": rotation.roll}},
    "fov": levels.get_level_viewport_fov(key)
}}))
"""
    infos = bridge(expected_pid, code)
    assert infos, "No bridge info returned"
    return infos[-1]


def probe(expected_pid):
    code = """
import json
import unreal
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
key = levels.get_active_viewport_config_key()
location, rotation = levels.get_level_viewport_camera_info(key)
print(json.dumps({
    "world": world.get_path_name().split(".")[0],
    "frame": unreal.SystemLibrary.get_frame_count(),
    "location": [location.x, location.y, location.z],
    "rotation": {"pitch": rotation.pitch, "yaw": rotation.yaw, "roll": rotation.roll},
    "fov": levels.get_level_viewport_fov(key)
}))
"""
    infos = bridge(expected_pid, code)
    assert infos, "No probe info returned"
    return infos[-1]


def capture_view(expected_pid, camera_id, label):
    cameras = camera_specs()
    camera = cameras[camera_id]
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    image_path = OUT_DIR / f"{label}.png"
    record_path = OUT_DIR / f"{label}.capture.json"
    assert not image_path.exists(), image_path
    assert not record_path.exists(), record_path

    first = prepare_camera(expected_pid, camera_id)
    assert first["world"] == load_receipt()["mapPackage"]
    same_pose(first, camera)
    deadline = time.monotonic() + 45.0
    latest = first
    while latest["frame"] - first["frame"] < 8:
        time.sleep(0.25)
        latest = probe(expected_pid)
        assert latest["world"] == first["world"]
        same_pose(latest, camera)
        assert time.monotonic() < deadline, f"Viewport redraw did not advance for {camera_id}"

    client = UnrealMcp("http://127.0.0.1:8000/mcp")
    try:
        client.initialize()
        result = client.request(
            "tools/call",
            {
                "name": "call_tool",
                "arguments": {
                    "toolset_name": "EditorToolset.EditorAppToolset",
                    "tool_name": "CaptureViewport",
                    "arguments": {
                        "captureTransform": None,
                        "annotations": None,
                        "bShowUI": False,
                    },
                },
            },
        )
        assert not result.get("isError"), result
        text = next(item["text"] for item in result["content"] if item["type"] == "text")
        capture = json.loads(text)["returnValue"]
        assert capture["image"]["mimeType"] == "image/png"
        native_pose = {
            "location": [capture["cameraLocation"][axis] for axis in ("x", "y", "z")],
            "rotation": {axis: capture["cameraRotation"][axis] for axis in ("pitch", "yaw", "roll")},
            "fov": capture["cameraFOV"],
        }
        same_pose(native_pose, camera)
        raw = base64.b64decode(capture["image"]["data"], validate=True)
        assert raw[:8] == b"\x89PNG\r\n\x1a\n"
        width, height = struct.unpack(">II", raw[16:24])
        with image_path.open("xb") as stream:
            stream.write(raw)
        capture.pop("image")
        record = {
            "schemaVersion": 1,
            "recordedAt": datetime.now(timezone.utc).isoformat(),
            "pid": expected_pid,
            "mapPackage": load_receipt()["mapPackage"],
            "camera": camera_id,
            "cameraConfig": camera,
            "image": image_path.relative_to(ROOT).as_posix(),
            "imageSha256": hashlib.sha256(raw).hexdigest(),
            "width": width,
            "height": height,
            "warmupNativeFrames": latest["frame"] - first["frame"],
            "nativeCapture": capture,
            "softwareRendering": True,
            "nullRHI": False,
            "receipt": RECEIPT.relative_to(ROOT).as_posix(),
            "receiptSha256": sha(RECEIPT),
            "scriptSha256": sha(__file__),
        }
        with record_path.open("x", encoding="utf-8", newline="\n") as stream:
            json.dump(record, stream, indent=2)
            stream.write("\n")
        return record
    finally:
        client.close()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--expected-pid", required=True, type=int)
    parser.add_argument("--camera", action="append", dest="cameras")
    parser.add_argument("--receipt")
    parser.add_argument("--outdir")
    args = parser.parse_args()
    global RECEIPT, OUT_DIR
    RECEIPT = Path(args.receipt).resolve() if args.receipt else DEFAULT_RECEIPT
    OUT_DIR = Path(args.outdir).resolve() if args.outdir else DEFAULT_OUT_DIR
    requested = args.cameras or [camera["id"] for camera in load_receipt()["cameras"]]
    results = [capture_view(args.expected_pid, camera_id, camera_id) for camera_id in requested]
    print(json.dumps(results, indent=2))


if __name__ == "__main__":
    main()

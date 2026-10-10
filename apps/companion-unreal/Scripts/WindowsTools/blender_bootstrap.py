"""Initialize only the dedicated Windows asset workbench and Lab MCP add-on."""

import json
import os
from pathlib import Path
import sys

import addon_utils
import bpy


root = Path(os.environ["BADDIE_BLENDER_TOOLS"])
workbench = Path(os.environ["BADDIE_BLENDER_WORKBENCH"])
sys.path.insert(0, str(root / "provider" / "addon"))
addon = addon_utils.enable("blender_mcp_addon", default_set=True, persistent=True)
if addon is None:
    raise RuntimeError("Blender Lab MCP add-on could not be enabled")
preferences = bpy.context.preferences.addons["blender_mcp_addon"].preferences
preferences.host = "127.0.0.1"
preferences.port = 9876
preferences.use_autostart = False

if bpy.data.filepath:
    if Path(bpy.data.filepath).resolve() != workbench.resolve():
        raise RuntimeError("Refusing to claim another Blender scene")
elif workbench.exists():
    bpy.ops.wm.open_mainfile(filepath=str(workbench))
else:
    workbench.parent.mkdir(parents=True, exist_ok=True)
    bpy.context.scene.name = "BaddieAssetWorkbench"
    bpy.context.scene.unit_settings.system = "METRIC"
    bpy.context.scene.unit_settings.scale_length = 1.0
    bpy.context.scene.render.engine = "CYCLES"
    bpy.context.scene.cycles.device = "CPU"
    bpy.ops.wm.save_as_mainfile(filepath=str(workbench))

print(json.dumps({
    "pid": os.getpid(),
    "file": bpy.data.filepath,
    "version": bpy.app.version_string,
    "scene": bpy.context.scene.name,
    "background": bpy.app.background,
    "bridge": "127.0.0.1:9876",
}))

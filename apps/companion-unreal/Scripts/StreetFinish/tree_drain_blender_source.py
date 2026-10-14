"""Create an editable source-library .blend in an isolated background worker.

Asset authoring only: no render, bake, live Blender mutation, native import,
validation suite or app export. Arguments: -- content-addressed-source-directory.
"""
import hashlib
import json
from pathlib import Path
import sys

import bpy


def author():
    if bpy.data.filepath or not bpy.app.background:
        raise RuntimeError("Requires an isolated factory-startup asset-authoring process")
    source = Path(sys.argv[sys.argv.index("--") + 1]).resolve()
    manifest = json.loads((source / "manifest.json").read_text())
    data = (source / "geometry.json").read_bytes()
    if hashlib.sha256(data).hexdigest() != manifest["geometrySha256"]:
        raise ValueError("Pinned geometry changed")
    output = source / "TreeDrainKit_Source.blend"
    if output.exists():
        raise RuntimeError("Refusing to overwrite editable source library")
    packet = json.loads(data)
    scene = bpy.data.scenes.new("TreeDrainKit_Source")
    bpy.context.window.scene = scene
    scene.unit_settings.system = "METRIC"
    materials = {}
    for name, recipe in packet["materials"].items():
        material = bpy.data.materials.new("TD_" + name)
        material.diffuse_color = (*recipe["color"], 1)
        material.use_nodes = True
        shader = material.node_tree.nodes.get("Principled BSDF")
        shader.inputs["Base Color"].default_value = (*recipe["color"], 1)
        shader.inputs["Roughness"].default_value = recipe["roughness"]
        shader.inputs["Metallic"].default_value = recipe["metallic"]
        materials[name] = material
    for index, asset in enumerate(packet["assets"]):
        collection = bpy.data.collections.new(asset["id"])
        scene.collection.children.link(collection)
        for level, geometry in enumerate(asset["lods"]):
            name = asset["id"] + "_LOD" + str(level)
            mesh = bpy.data.meshes.new(name)
            mesh.from_pydata([[v / 100 for v in p] for p in geometry["verticesCm"]], [], geometry["triangles"])
            mesh.update()
            used = sorted(set(geometry["faceMaterialIds"]))
            for material_name in used:
                mesh.materials.append(materials[material_name])
            uv = mesh.uv_layers.new(name="UVMap")
            for polygon, material_name, coords in zip(mesh.polygons, geometry["faceMaterialIds"], geometry["triangleUVsMetres"]):
                polygon.material_index = used.index(material_name)
                polygon.use_smooth = asset["kind"] == "tree" and material_name == "bark"
                for loop, coord in zip(polygon.loop_indices, coords):
                    uv.data[loop].uv = coord
            obj = bpy.data.objects.new(name, mesh)
            collection.objects.link(obj)
            obj.location = ((index % 4) * 14, (index // 4) * 14, 0)
            obj.hide_render = level > 0
            obj.hide_set(level > 0)
            obj["sourceGeometrySha256"] = manifest["geometrySha256"]
            obj["assetPivot"] = "Tree ground/root or drain rim at local Z=0; grid offset is library display only"
            obj["speciesVerified"] = False
    scene["sourceGeometrySha256"] = manifest["geometrySha256"]
    scene["status"] = "Original editable asset library; not rendered acceptance or surveyed Lagos geometry"
    bpy.ops.wm.save_as_mainfile(filepath=str(output), check_existing=False)
    print(json.dumps({"path": str(output), "assets": len(packet["assets"]), "rendered": False}), flush=True)


if __name__ == "__main__":
    author()

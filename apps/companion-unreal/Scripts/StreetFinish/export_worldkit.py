"""Bounded Blender conversion; never edits the original world-kit library.

Run in a dedicated --background --factory-startup Blender process, not a
user's workbench. Arguments: -- SOURCE.blend-or-RECIPE_DIRECTORY DESTINATION.
No render or bake. Recipe reconstruction is labeled separately from archived v06.
"""
import ast
import hashlib
import json
import os
from pathlib import Path
import sys

import bpy
from mathutils import Matrix


def reconstruct(source, names):
    """Replay only named, retained asset recipes; do not run their full-library mains."""
    sys.path.insert(0, str(source))
    import author_world_kit as kit
    import refine_world_kit_v04 as refine
    palette = [
        ('iron', (.034, .044, .042), .44, .5),
        ('steel', (.43, .46, .47), .3, .9),
        ('galvanised', (.48, .51, .49), .47, .78),
        ('wood', (.26, .12, .052), .52, 0),
        ('bark', (.19, .09, .044), .91, 0),
        ('leaf', (.08, .21, .066), .69, 0),
        ('leaf-light', (.16, .29, .07), .73, 0),
        ('sage', (.19, .29, .23), .46, 0),
        ('ivory', (.87, .84, .74), .79, 0),
    ]
    for key, color, roughness, metallic in palette:
        kit.M[key] = kit.finish(key, color, roughness, metallic)
    kit.vegetation()
    parsed = ast.parse((source / 'author_world_kit.py').read_text())
    for function, asset_id in [('architecture', 'corrugated-awning'),
                               ('furniture', 'street-bench'),
                               ('cooking_and_market', 'vendor-canopy')]:
        body = next(n.body for n in parsed.body if isinstance(n, ast.FunctionDef) and n.name == function)
        chunks, current = [], None
        for node in body:
            start = (isinstance(node, ast.Assign) and isinstance(node.value, ast.Call)
                     and isinstance(node.value.func, ast.Name) and node.value.func.id == 'Asset')
            if start:
                current = [] if ast.literal_eval(node.value.args[0]) == asset_id else None
                if current is not None:
                    chunks.append(current)
            if current is not None:
                current.append(node)
        if len(chunks) != 1:
            raise RuntimeError('Could not isolate retained recipe ' + asset_id)
        exec(compile(ast.Module(body=chunks[0], type_ignores=[]), str(source / 'author_world_kit.py'), 'exec'), kit.__dict__)
    for asset in kit.ASSETS:
        refine.parent_new(asset)
    refine.vegetation()
    for asset in kit.ASSETS:
        refine.parent_new(asset)
        low = refine.bounds(asset)['min'][2]
        for obj in asset.collection.objects:
            if obj.type == 'MESH':
                refine.move_geometry(obj, (0, 0, -low))
    return [bpy.data.collections['Asset | ' + name] for name in names]


def main():
    source, destination = map(Path, sys.argv[sys.argv.index('--') + 1:])
    if bpy.data.filepath or not bpy.app.background:
        raise RuntimeError('Requires a new, isolated background conversion process')
    names = ['broadleaf-tree', 'palm-tree', 'shrub-cluster',
             'corrugated-awning', 'street-bench', 'vendor-canopy']
    expected = 'b33ed8f9404057a7d260f199abb05b7e51fcca325ac1524a018d4f5c9d076f8d'
    recipes = source.is_dir()
    if not recipes and hashlib.sha256(source.read_bytes()).hexdigest() != expected:
        raise RuntimeError('World-kit source hash changed')
    if (destination / 'manifest.json').exists():
        raise RuntimeError('Refusing to overwrite a completed conversion')
    destination.mkdir(parents=True, exist_ok=True)
    scene = bpy.data.scenes.new('StreetFinish_20261014_Conversion')
    bpy.context.window.scene = scene
    scene.unit_settings.system = 'METRIC'
    scene.unit_settings.scale_length = 1.0
    if recipes:
        collections = reconstruct(source, names)
    else:
        with bpy.data.libraries.load(str(source), link=False) as (available, loaded):
            wanted = ['Asset | ' + name for name in names]
            if not set(wanted).issubset(available.collections):
                raise RuntimeError('Missing source asset collections')
            loaded.collections = wanted
        collections = loaded.collections
        for collection in collections:
            scene.collection.children.link(collection)
    bpy.context.view_layer.update()
    report = {'pid': os.getpid(), 'blender': bpy.app.version_string,
              'scene': scene.name, 'source': str(source),
              'source_sha256': None if recipes else expected,
              'source_kind': 'bounded retained-recipe reconstruction; not archived v06' if recipes else 'archived v06',
              'units': 'metres', 'forward': '-Y', 'up': 'Z',
              'materials': {}, 'assets': [], 'renders': 'none'}
    for name, collection in zip(names, collections):
        root = next(o for o in collection.all_objects if o.name == 'asset:' + name)
        inverse = root.matrix_world.inverted()
        depsgraph = bpy.context.evaluated_depsgraph_get()
        verts, faces, face_materials, slots, uv_faces = [], [], [], [], []
        for obj in collection.all_objects:
            if obj.type not in {'MESH', 'CURVE'}:
                continue
            evaluated = obj.evaluated_get(depsgraph)
            mesh = evaluated.to_mesh(preserve_all_data_layers=True, depsgraph=depsgraph)
            if mesh is None:
                continue
            transform = inverse @ obj.matrix_world
            offset = len(verts)
            verts.extend(tuple(transform @ v.co) for v in mesh.vertices)
            local_slots = []
            for mat in mesh.materials:
                if mat not in slots:
                    slots.append(mat)
                local_slots.append(slots.index(mat))
            uv = mesh.uv_layers.active
            for poly in mesh.polygons:
                faces.append(tuple(offset + i for i in poly.vertices))
                face_materials.append(local_slots[poly.material_index] if local_slots else 0)
                uv_faces.append([tuple(uv.data[i].uv) for i in poly.loop_indices] if uv else [(0, 0)] * len(poly.vertices))
            evaluated.to_mesh_clear()
        mesh = bpy.data.meshes.new('SM_WK_' + name.replace('-', '_'))
        mesh.from_pydata(verts, [], faces)
        mesh.update()
        for mat in slots:
            mesh.materials.append(mat)
            shader = next((n for n in mat.node_tree.nodes if n.type == 'BSDF_PRINCIPLED'), None) if mat.use_nodes else None
            record = {'name': mat.name, 'base_color': list(mat.diffuse_color),
                      'roughness': mat.roughness, 'metallic': mat.metallic,
                      'base_color_linked': False}
            if shader:
                record.update(base_color=list(shader.inputs['Base Color'].default_value),
                              roughness=shader.inputs['Roughness'].default_value,
                              metallic=shader.inputs['Metallic'].default_value,
                              base_color_linked=shader.inputs['Base Color'].is_linked)
                if shader.inputs['Base Color'].is_linked:
                    node = shader.inputs['Base Color'].links[0].from_node
                    if node.type == 'VALTORGB':
                        colors = [list(e.color) for e in node.color_ramp.elements]
                        record['base_color'] = [sum(c[i] for c in colors) / len(colors) for i in range(4)]
                        record['approximation'] = 'Mean of authored procedural color ramp; no texture bake'
                    else:
                        record['linked_node'] = node.type
            report['materials'][mat.name] = record
        uv = mesh.uv_layers.new(name='UVMap')
        for poly, material, coords in zip(mesh.polygons, face_materials, uv_faces):
            poly.material_index = material
            poly.use_smooth = True
            for loop, coord in zip(poly.loop_indices, coords):
                uv.data[loop].uv = coord
        obj = bpy.data.objects.new(mesh.name, mesh)
        scene.collection.objects.link(obj)
        obj.matrix_world = Matrix.Identity(4)
        bpy.ops.object.select_all(action='DESELECT')
        obj.select_set(True)
        bpy.context.view_layer.objects.active = obj
        bounds = {'min': [min(v[i] for v in verts) for i in range(3)],
                  'max': [max(v[i] for v in verts) for i in range(3)]}
        lods = []
        for level, ratio in enumerate([1.0, .5, .2]):
            modifier = None
            if level:
                modifier = obj.modifiers.new('Delivery_LOD', 'DECIMATE')
                modifier.ratio = ratio
            bpy.context.view_layer.update()
            evaluated = obj.evaluated_get(bpy.context.evaluated_depsgraph_get())
            counted = evaluated.to_mesh()
            counted.calc_loop_triangles()
            triangles = len(counted.loop_triangles)
            evaluated.to_mesh_clear()
            path = destination / f'{mesh.name}_LOD{level}.fbx'
            bpy.ops.export_scene.fbx(filepath=str(path), use_selection=True,
                                     object_types={'MESH'}, apply_unit_scale=True,
                                     apply_scale_options='FBX_SCALE_UNITS',
                                     axis_forward='-Y', axis_up='Z',
                                     bake_anim=False, use_mesh_modifiers=True,
                                     mesh_smooth_type='FACE', path_mode='AUTO')
            lods.append({'file': path.name, 'triangles': triangles,
                         'sha256': hashlib.sha256(path.read_bytes()).hexdigest()})
            if modifier:
                obj.modifiers.remove(modifier)
        report['assets'].append({'id': mesh.name, 'source_id': name,
                                 'materials': [mat.name for mat in slots],
                                 'bounds': bounds, 'lods': lods})
        print(json.dumps(report['assets'][-1]), flush=True)
    bpy.data.libraries.write(str(destination / 'StreetFinish_Converted.blend'),
                             {scene}, path_remap='RELATIVE', fake_user=True, compress=True)
    (destination / 'manifest.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({'complete': True, 'pid': os.getpid(), 'assets': len(report['assets'])}), flush=True)


if __name__ == '__main__':
    main()

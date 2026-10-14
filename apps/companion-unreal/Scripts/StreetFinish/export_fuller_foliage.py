"""Native-only foliage variants, reconstructed in an isolated Blender worker.

Arguments match export_worldkit.py. No shared/mobile source or accepted native
mesh is overwritten. Adds leaf-bearing shoots to the retained branch tips;
individual leaf size stays 14-20 cm. No renders, bakes or application builds.
"""
import math
from pathlib import Path
import random
import runpy
import sys

import bpy
from mathutils import Vector

sys.path.insert(0, str(Path(__file__).parent))
import export_worldkit


def fuller_foliage():
    import author_world_kit as kit
    import refine_world_kit_v04 as refine
    rng = random.Random(10142026)
    for identifier, tip_name, shoots, pairs, radius in [
        ('broadleaf-tree', 'fine branch', 14, 7, .74),
        ('shrub-cluster', 'woody stem', 6, 4, .22),
    ]:
        asset = refine.author(identifier)
        tips = [sum((v.co for v in obj.data.vertices[-10:]), Vector()) / 10
                for obj in refine.parts(asset, tip_name)]
        if not tips:
            raise RuntimeError('Retained branch tips missing for ' + identifier)
        vertices, faces, finishes = [], [], []

        def triangle(a, b, c, finish):
            index = len(vertices)
            vertices.extend([tuple(a), tuple(b), tuple(c)])
            faces.append((index, index + 1, index + 2))
            finishes.append(finish)

        def leaf(start, direction, length, width, finish):
            side = direction.cross(Vector((0, 0, 1))).normalized()
            tip = start + direction * length
            left = start + direction * length * .48 + side * width
            right = start + direction * length * .48 - side * width
            ridge = start + direction * length * .50 + Vector((0, 0, .012))
            for a, b, c in [(start, left, ridge), (left, tip, ridge),
                            (tip, right, ridge), (right, start, ridge)]:
                triangle(a, b, c, finish)

        for tip_index, base in enumerate(tips):
            for shoot in range(shoots):
                angle = shoot * math.tau / shoots + rng.uniform(-.22, .22)
                reach = radius * rng.uniform(.65, 1.1)
                end = base + Vector((math.cos(angle) * reach,
                                     math.sin(angle) * reach, rng.uniform(-.16, .44)))
                # Each shoot physically starts at an existing branch tip.
                direction = (end - base).normalized()
                side = direction.cross(Vector((0, 0, 1))).normalized() * .003
                up = direction.cross(side).normalized() * .003
                for radial in [side, up]:
                    triangle(base - radial, base + radial, end, 0)
                for pair in range(pairs):
                    t = .20 + pair * .76 / max(1, pairs - 1)
                    origin = base.lerp(end, t)
                    for sign in [-1, 1]:
                        heading = angle + sign * rng.uniform(.7, 1.4)
                        direction_leaf = Vector((math.cos(heading), math.sin(heading), rng.uniform(-.45, .55))).normalized()
                        leaf(origin, direction_leaf, rng.uniform(.14, .20),
                             rng.uniform(.031, .044), 2 if (tip_index + pair + shoot) % 5 == 0 else 1)
        mesh = bpy.data.meshes.new(identifier + '_CrownShoots')
        mesh.from_pydata(vertices, [], faces)
        mesh.update()
        for material in ['bark', 'leaf', 'leaf-light']:
            mesh.materials.append(kit.M[material])
        for polygon, finish in zip(mesh.polygons, finishes):
            polygon.material_index = finish
            polygon.use_smooth = True
        obj = bpy.data.objects.new(identifier + ' | connected crown shoots', mesh)
        asset.collection.objects.link(obj)
        obj.parent = asset.root
        obj['seed'] = 10142026
        obj['purpose'] = 'Fuller native street foliage; retained trunk and leaf scale'


if __name__ == '__main__':
    if bpy.data.filepath or not bpy.app.background:
        raise RuntimeError('Use a new isolated background worker')
    arguments = sys.argv[sys.argv.index('--') + 1:]
    if len(arguments) != 2 or not Path(arguments[0]).is_dir():
        raise RuntimeError('Fuller foliage requires the retained recipe directory and a new destination')
    probe = Path(__file__).with_name('probe_scene.py')
    if not probe.exists():
        raise RuntimeError('Stage the read-only Blender production scene probe first')
    print(runpy.run_path(str(probe))['inspect_scene'](limit=5), flush=True)
    export_worldkit.main(
        names=['broadleaf-tree', 'shrub-cluster'], refine_callback=fuller_foliage,
        prefix='SM_SF2_', source_kind='native fuller foliage variant from retained recipes; not archived v06')

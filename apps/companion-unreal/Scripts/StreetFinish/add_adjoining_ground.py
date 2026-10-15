"""Fitted garden/apron and verge surfaces for the additive adjoining buildings.

All ground is decorative flat support treatment. Roads, existing entrances,
other parcels and accepted architecture remain exclusions, not regenerated.
"""
import json
import math
from pathlib import Path
import random
import runpy

import unreal as u

AD = runpy.run_path(str(Path(__file__).with_name('add_adjoining.py')))
BASE, ROOT, OUTPUT = AD['BASE'], AD['ROOT'], AD['OUTPUT']
SURFACE_PREFIX = 'Adjoining ground: '
PLANTING_LABEL = 'Adjoining planting'
PLANTING_GRAPH = 'PCG_Adjoining_GroundPlanting'
MESH_PREFIX = 'SM_AD_'
IMPORT_SOURCE = 'adjoining-20261014-ground-source'
FOLDER = 'StreetFinish_20261014/Adjoining'
VERGES = ['TC_Ground22_unity-avenue_' + str(i) + '_support' for i in range(10, 14)] + [
    'TC_Ground22_theatre-street_' + str(i) + '_support' for i in range(3)]


def area(poly):
    return sum(a[0]*b[1]-b[0]*a[1] for a, b in zip(poly, poly[1:]+poly[:1])) / 2


def rectangle(x0, x1, y0, y1):
    return [(x0, y0), (x1, y0), (x1, y1), (x0, y1)]


def transform(poly, origin, yaw):
    a = math.radians(yaw)
    return [(origin[0]+x*math.cos(a)-y*math.sin(a), origin[1]+x*math.sin(a)+y*math.cos(a)) for x, y in poly]


def box_poly(box, padding=0):
    ex, ey = box['extents'][:2]
    return transform(rectangle(-ex-padding, ex+padding, -ey-padding, ey+padding), box['centre'], box['yaw'])


def bounds(poly):
    return min(p[0] for p in poly), max(p[0] for p in poly), min(p[1] for p in poly), max(p[1] for p in poly)


def overlaps(a, b):
    return a[0] < b[1] and a[1] > b[0] and a[2] < b[3] and a[3] > b[2]


def halfplane(poly, a, b, positive):
    def side(p):
        return (b[0]-a[0])*(p[1]-a[1])-(b[1]-a[1])*(p[0]-a[0])
    result = []
    for p, q in zip(poly, poly[1:]+poly[:1]):
        sp, sq = side(p), side(q)
        pin, qin = (sp >= -1e-7, sq >= -1e-7) if positive else (sp <= 1e-7, sq <= 1e-7)
        if pin:
            result.append(p)
        if pin != qin and abs(sp-sq) > 1e-10:
            t = sp/(sp-sq)
            result.append((p[0]+(q[0]-p[0])*t, p[1]+(q[1]-p[1])*t))
    clean = []
    for p in result:
        if not clean or math.dist(clean[-1], p) > .02:
            clean.append(p)
    if len(clean) > 1 and math.dist(clean[0], clean[-1]) <= .02:
        clean.pop()
    return clean


def cut(pieces, cutter):
    cutter = cutter if area(cutter) > 0 else list(reversed(cutter))
    cb, output = bounds(cutter), []
    for piece in pieces:
        if not overlaps(bounds(piece), cb):
            output.append(piece)
            continue
        remaining = piece
        for a, b in zip(cutter, cutter[1:]+cutter[:1]):
            if len(remaining) < 3:
                break
            outside = halfplane(remaining, a, b, False)
            if len(outside) >= 3 and area(outside) > 10:
                output.append(outside)
            remaining = halfplane(remaining, a, b, True)
    return output


def inside(point, polygon, margin=0):
    for a, b in zip(polygon, polygon[1:]+polygon[:1]):
        length = math.dist(a, b)
        if length > .001 and ((b[0]-a[0])*(point[1]-a[1])-(b[1]-a[1])*(point[0]-a[0])) / length < margin:
            return False
    return True


def prepare():
    AD['guard'](False)
    if (OUTPUT / 'ground-prepared.json').exists():
        raise RuntimeError('Ground kit is complete; preserve its saved assets')
    survey = json.loads((OUTPUT / 'survey.json').read_text())
    if AD['BIOME']['fingerprint'](AD['actors']()) != survey['protectedFingerprint']:
        raise RuntimeError('Protected source changed')
    ground = survey['protectedGround']
    plots, surfaces = [], []
    for p in survey['plans']:
        source = p['sourceParcel']
        half = source['width']*50 + source['garden']*100
        front = (p['depthCm']-source['depth']*100)/2 - source['garden']*100
        poly = transform(rectangle(front, p['depthCm']-front, -half, half), p['originCm'], p['yaw'])
        plots.append(poly)
        pieces = [poly]
        shell = transform(rectangle(-130, p['depthCm']+40, -p['widthCm']/2-40, p['widthCm']/2+40), p['originCm'], p['yaw'])
        cutters = [shell] + [box_poly(b, 16) for b in ground if b['label'].endswith(('_support', '_entrance')) or (
            b['label'].endswith('_footprint') and p['parcelId'] not in b['label'])]
        for cutter in cutters:
            pieces = cut(pieces, cutter)
        surfaces.append({'id': p['parcelId']+'-yard', 'kind': 'yard', 'centre': p['sourceCentreCm'], 'pieces': pieces,
                         'z': 1.0, 'material': AD['COURT']['BASE']+'/Materials/M_CompactedLaterite'})
    selected = [b for b in ground if b['label'] in VERGES]
    if len(selected) != len(VERGES):
        raise RuntimeError('Road support selection changed')
    previous = []
    for b in selected:
        poly = box_poly(b, 1600)
        pieces = [poly]
        # All original road shoulders/entrances/footprints and new gardens remain clear.
        for cutter in [box_poly(v, 20) for v in ground] + plots + previous:
            pieces = cut(pieces, cutter)
        previous.append(poly)
        surfaces.append({'id': b['label'].removeprefix('TC_Ground22_')+'-verge', 'kind': 'verge',
                         'centre': b['centre'][:2], 'pieces': pieces, 'z': -4.8,
                         'material': AD['COURT']['BASE']+'/Materials/M_ParkMeadow'})
    kit = AD['native_kit']()
    for surface in surfaces:
        vertices, faces = [], []
        origin = surface['centre']
        for piece in surface['pieces']:
            # Centroid fan keeps every piece convex with explicit native +Z winding.
            centre = [sum(p[i] for p in piece)/len(piece) for i in (0, 1)]
            for a, b in zip(piece, piece[1:]+piece[:1]):
                tri = [centre, a, b]
                if area(tri) <= 2:
                    continue
                start = len(vertices)
                vertices.extend([[p[0]-origin[0], p[1]-origin[1], surface['z']] for p in tri])
                faces.append([start, start+1, start+2])
        if len(faces) < 3:
            raise RuntimeError('Fitted surface was completely excluded')
        package = BASE + '/Meshes/' + MESH_PREFIX + surface['id'].replace('-', '_')
        mesh, asset = kit.FABRIC.IMPORT.import_section(package, 0, 0, 0, 0,
            supplied_geometry=(vertices, faces), source_directory=IMPORT_SOURCE,
            vertical_uv=False, recover=True)
        if mesh.get_num_triangles(0) != len(faces):
            raise RuntimeError('Ground topology mismatch: ' + surface['id'])
        mesh.set_material(0, u.load_asset(surface['material']))
        u.EditorAssetLibrary.set_metadata_tag(mesh, 'AdjoiningOwner', AD['OWNER'])
        if not u.EditorAssetLibrary.save_loaded_asset(mesh):
            raise RuntimeError('Could not save ground mesh')
        surface.update(mesh=package, asset=asset, areaM2=sum(area(p) for p in surface['pieces'])/10000)
    AD['write']('ground-prepared.json', {'surfaces': surfaces})
    print(json.dumps({'surfaces': len(surfaces), 'areaM2': sum(s['areaM2'] for s in surfaces)}))


def install(resume_surfaces=False):
    AD['guard'](False)
    existing = {a.get_actor_label(): a for a in AD['actors']()
                if a.get_actor_label().startswith((SURFACE_PREFIX, PLANTING_LABEL))}
    if existing and not resume_surfaces:
        raise RuntimeError('Adjoining ground already exists')
    data = json.loads((OUTPUT / 'ground-prepared.json').read_text())
    survey = json.loads((OUTPUT / 'survey.json').read_text())
    kit = AD['native_kit']()
    kit.BASE.make_level_current(AD['LAYER'])
    rows = []
    if resume_surfaces and set(existing) != {SURFACE_PREFIX + s['id'] for s in data['surfaces']}:
        raise RuntimeError('Only the exact incomplete sixteen-surface installation can resume')
    for surface in data['surfaces']:
        if resume_surfaces:
            actor = existing[SURFACE_PREFIX + surface['id']]
            if actor.static_mesh_component.static_mesh.get_path_name().split('.')[0] != surface['mesh'] or math.dist(
                    AD['STREET']['vec'](actor.get_actor_location()), [*surface['centre'], 0]) > .01:
                raise RuntimeError('Incomplete ground was edited; preserve it')
            continue
        actor = u.get_editor_subsystem(u.EditorActorSubsystem).spawn_actor_from_class(u.StaticMeshActor, u.Vector(*surface['centre'], 0))
        actor.set_actor_label(SURFACE_PREFIX + surface['id'])
        actor.set_folder_path(FOLDER + '/Ground')
        actor.tags = [AD['OWNER'], 'Baddie.NoRuntimeAuthority']
        c = actor.static_mesh_component
        c.set_static_mesh(u.load_asset(surface['mesh']))
        c.set_collision_profile_name('NoCollision')
        c.set_collision_enabled(u.CollisionEnabled.NO_COLLISION)
        c.set_editor_properties({'can_ever_affect_navigation': False, 'cast_shadow': False})
    def put(mesh, point, yaw, scale=(1, 1, 1), source='verge'):
        rows.append({'mesh': ROOT + '/' + mesh, 'location': list(point), 'yaw': yaw,
                     'scale': list(scale), 'cull_cm': [22000, 30000], 'source': source})
    rng = random.Random(101409)
    rejected = []
    for surface in data['surfaces']:
        if surface['kind'] != 'verge':
            continue
        pieces = surface['pieces']
        if not pieces:
            continue
        x0, x1, y0, y1 = bounds([p for piece in pieces for p in piece])
        accepted = []
        target = int(surface['areaM2']/30)
        for _ in range(target*35):
            if len(accepted) >= target:
                break
            p = (rng.uniform(x0, x1), rng.uniform(y0, y1))
            if not any(inside(p, piece, 85) for piece in pieces) or any(math.dist(p, q) < 280 for q in accepted):
                continue
            try:
                support = AD['STREET']['ground'](u.Vector(*p, 0))
            except RuntimeError as error:
                if not str(error).startswith('No horizontal support at'):
                    raise
                rejected.append({'point': p, 'reason': 'No horizontal support'})
                continue
            if abs(support+5) > .2:
                rejected.append({'point': p, 'reason': 'Different support elevation', 'z': support})
                continue
            accepted.append(p)
            i, angle = len(accepted), rng.uniform(0, 360)
            if i % 13 == 0:
                put('Foliage2/Meshes/SM_SF2_shrub_cluster', (*p, -4.8), angle, (.9, .9, .85))
            else:
                put('EyeLevel/Meshes/SM_EL_GrassClump' if i % 3 else 'EyeLevel/Meshes/SM_EL_Groundcover',
                    (*p, -4.8), angle, (1.2, 1.2, 1.15))
    for p in survey['plans']:
        pid, half = p['parcelId'], p['widthCm']/2
        for side in (-1, 1):
            point = kit.BASE.transform_local(p['originCm'], p['yaw'], [-110, side*(half-85), 1])
            put('EyeLevel/Meshes/SM_EL_FernPot', point, p['yaw'], source=pid)
        # Closed source door and shop shutter approaches remain unobstructed.
        if p['sourceParcel']['garden']:
            yard = next(s for s in data['surfaces'] if s['id'] == pid+'-yard')
            for side in (-1, 1):
                for x in (.3*p['depthCm'], .7*p['depthCm']):
                    loc = kit.BASE.transform_local(p['originCm'], p['yaw'], [x, side*(p['sourceParcel']['width']*50+150), 1])
                    if not any(inside(loc, piece, 90) for piece in yard['pieces']):
                        continue
                    put('Forecourts/Meshes/SM_FC_BedPlanter', loc, p['yaw'], source=pid)
                    for offset in (-65, 65):
                        leaf = kit.BASE.transform_local(loc, p['yaw'], [offset, 0, 15])
                        # transform_local includes local Z, so keep the soil contact explicit.
                        leaf[2] = 16
                        put('EyeLevel/Meshes/SM_EL_GrassClump', leaf, rng.uniform(0, 360), (1.15, 1.15, 1.0), pid)
    actor = AD['STREET']['make_graph'](PLANTING_GRAPH, rows,
        scope='Fixed, seed-derived verge points and source-garden beds; CPU editor scenery', label=PLANTING_LABEL)
    actor.tags = list(actor.tags) + [AD['OWNER']]
    actor.set_folder_path(FOLDER)
    AD['write']('ground-installed.json', {'surfaces': [{'id': s['id'], 'areaM2': s['areaM2'], 'mesh': s['mesh']} for s in data['surfaces']],
        'placements': rows, 'rejectedPlantingPoints': rejected})
    actor.pcg_component.generate(True)
    print(json.dumps({'surfaces': len(data['surfaces']), 'plantingInstances': len(rows), 'generation': 'pending'}))


def audit(name='ground-readback.json'):
    AD['guard'](False)
    prepared = json.loads((OUTPUT / 'ground-prepared.json').read_text())
    installed = json.loads((OUTPUT / 'ground-installed.json').read_text())
    scene = {a.get_actor_label(): a for a in AD['actors']()}
    for surface in prepared['surfaces']:
        actor = scene[SURFACE_PREFIX + surface['id']]
        c = actor.static_mesh_component
        if c.static_mesh.get_path_name().split('.')[0] != surface['mesh'] or c.get_material(0).get_path_name().split('.')[0] != surface['material']:
            raise RuntimeError('Ground mesh/material mismatch')
        if math.dist(AD['STREET']['vec'](actor.get_actor_location()), [*surface['centre'], 0]) > .01:
            raise RuntimeError('Ground moved')
        if c.get_collision_enabled() != u.CollisionEnabled.NO_COLLISION or c.get_editor_property('can_ever_affect_navigation'):
            raise RuntimeError('Ground acquired collision/navigation authority')
    planting = scene[PLANTING_LABEL]
    if planting.pcg_component.get_editor_property('generation_in_progress') or not planting.pcg_component.generated:
        raise RuntimeError('Ground planting generation incomplete')
    actual = []
    for c in planting.get_components_by_class(u.InstancedStaticMeshComponent):
        if c.get_collision_enabled() != u.CollisionEnabled.NO_COLLISION or c.get_editor_property('can_ever_affect_navigation'):
            raise RuntimeError('Planting acquired collision/navigation authority')
        if any(not c.get_material(i) or 'DefaultMaterial' in c.get_material(i).get_name() for i in range(c.get_num_materials())):
            raise RuntimeError('Missing planting material')
        for i in range(c.get_instance_count()):
            t = c.get_instance_transform(i, world_space=True)
            actual.append({'mesh': c.static_mesh.get_path_name().split('.')[0],
                           'location': AD['STREET']['vec'](t.translation), 'scale': AD['STREET']['vec'](t.scale3d),
                           'yaw': t.rotation.rotator().yaw})
    if len(actual) != len(installed['placements']):
        raise RuntimeError('Ground planting count mismatch')
    for row in installed['placements']:
        matches = [p for p in actual if p['mesh'] == row['mesh'] and math.dist(p['location'], row['location']) < .05
                   and abs((p['yaw']-row['yaw']+180) % 360-180) < .001]
        if len(matches) != 1 or math.dist(matches[0]['scale'], row['scale']) > .001:
            raise RuntimeError('Ground planting transform mismatch')
        actual.remove(matches[0])
    result = {'surfaces': len(prepared['surfaces']), 'areaM2': sum(s['areaM2'] for s in prepared['surfaces']),
              'plantingInstances': len(installed['placements']), 'noCollision': True, 'noNavigation': True,
              'roadAndEntranceMasks': 'Source support rectangles subtracted before import', 'flatGroundOnly': True}
    AD['write'](name, result, replace=True)
    print(json.dumps(result))

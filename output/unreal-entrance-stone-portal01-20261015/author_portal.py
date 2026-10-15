"""One owned main-thread Blender job. No old inputs, network, modifiers or threads."""
import bpy
import bmesh
import json
import os
import socket
import time
import traceback
from pathlib import Path
from datetime import datetime, timezone
from mathutils import Vector

ROOT = Path(__file__).resolve().parent
JOB = 'entrance-stone-portal01'
TOL = 0.000002
AREA_TOL = 0.000002
NAMES = ['LeftPier', 'RightPier', 'Canopy', 'Threshold', 'PresentationFoundationOnly']
PARAMS = dict(pier_corner=0.002, canopy_corner=0.002, threshold_corner=0.001,
              pier_top=2.92, canopy_bottom=2.92, threshold_top=0.03, foundation_top=0.0)
EXPECTED_MATERIAL = dict(LeftPier='PortalStone', RightPier='PortalStone', Canopy='PortalStone',
                         Threshold='ThresholdStone', PresentationFoundationOnly='FoundationStone')
VIEWS = {
    'front': ((6.2, -8.5, 5.0), (0, 0, 1.45), 48),
    'reverse': ((-5.7, 7.6, -0.9), (0, 0, 1.45), 46),
    'grazing': ((-2.5, -1.55, 3.28), (-1.72, 0, 2.92), 62),
    'player165': ((0, -8.2, 1.65), (0, 0, 1.56), 40),
    'foot-detail': ((-2.4, -1.7, 0.46), (-1.62, 0, 0.06), 62),
}


def receipt(name, value):
    with (ROOT / name).open('x', encoding='utf-8', newline='\n') as handle:
        json.dump(value, handle, sort_keys=True, indent=2, allow_nan=False)


def identity():
    scene = bpy.context.scene
    return dict(host=socket.gethostname(), pid=os.getpid(), file=bpy.data.filepath,
                scene=scene.name, engine=scene.render.engine, version=bpy.app.version_string,
                job=JOB, utc=datetime.now(timezone.utc).isoformat())


def require(ok, obj, field, sample):
    if not ok:
        context = dict(object=obj, field=field, sample=sample, tolerance_m=TOL)
        receipt('guard-failure.json', context)
        raise RuntimeError(json.dumps(context))


def close(actual, expected, obj, field):
    require(abs(actual - expected) <= TOL, obj, field,
            dict(actual=actual, expected=expected, distance=abs(actual - expected)))


def linear(c):
    return c / 12.92 if c <= 0.04045 else ((c + 0.055) / 1.055) ** 2.4


def material(name, colour, roughness):
    m = bpy.data.materials.new(name)
    m.use_nodes = True
    rgb = tuple(linear(int(colour[i:i+2], 16) / 255) for i in (1, 3, 5))
    m.diffuse_color = (*rgb, 1)
    node = m.node_tree.nodes.get('Principled BSDF')
    node.inputs['Base Color'].default_value = (*rgb, 1)
    node.inputs['Roughness'].default_value = roughness
    node.inputs['Metallic'].default_value = 0
    return m


def extrude(name, profile, low, high, axis, mat):
    # Profiles are CCW; cyclic XY/Z and YZ/X mappings both preserve orientation.
    n = len(profile)
    verts = [(u, v, t) if axis == 'Z' else (t, u, v)
             for t in (low, high) for u, v in profile]
    faces = [tuple(reversed(range(n))), tuple(range(n, 2*n))]
    faces += [(i, (i+1) % n, (i+1) % n+n, i+n) for i in range(n)]
    mesh = bpy.data.meshes.new(name + 'Mesh')
    mesh.from_pydata(verts, [], faces)
    mesh.materials.append(mat)
    for face in mesh.polygons:
        face.material_index = 0
        face.use_smooth = False
    mesh.update()
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.scene.collection.objects.link(obj)
    return obj


def rectangle(a, b, c, d):
    return [(a,c), (b,c), (b,d), (a,d)]


def top_profile(y0, y1, z0, z1, corner):
    return [(y0,z0), (y1,z0), (y1,z1-corner), (y1-corner,z1),
            (y0+corner,z1), (y0,z1-corner)]


def construct(params, replace=False):
    if replace:
        # Only these five owned objects/meshes are replaced on an explicit correction.
        for name in NAMES:
            obj = bpy.data.objects[name]
            mesh = obj.data
            bpy.data.objects.remove(obj, do_unlink=True)
            bpy.data.meshes.remove(mesh)
    c = params['pier_corner']
    for name, centre in [('LeftPier', -1.75), ('RightPier', 1.75)]:
        a, b, lo, hi = centre-.11, centre+.11, -.41, .41
        profile = [(a+c,lo), (b-c,lo), (b,lo+c), (b,hi-c),
                   (b-c,hi), (a+c,hi), (a,hi-c), (a,lo+c)]
        extrude(name, profile, 0, params['pier_top'], 'Z', bpy.data.materials['PortalStone'])
    extrude('Canopy', top_profile(-.47,.43,params['canopy_bottom'],3.14,params['canopy_corner']),
            -1.825,1.825,'X',bpy.data.materials['PortalStone'])
    extrude('Threshold', top_profile(-.42,.44,0,params['threshold_top'],params['threshold_corner']),
            -1.63,1.63,'X',bpy.data.materials['ThresholdStone'])
    extrude('PresentationFoundationOnly', rectangle(-.8,.8,params['foundation_top']-.08,params['foundation_top']),
            -2.15,2.15,'X',bpy.data.materials['FoundationStone'])
    bpy.context.view_layer.update()


def diagnostic():
    result = {}
    for name in NAMES:
        obj = bpy.data.objects[name]
        slots = [slot.material.name if slot.material is not None else None for slot in obj.material_slots]
        result[name] = dict(slots=[dict(index=i, material=m) for i,m in enumerate(slots)],
                            faces=[dict(face=p.index, material_index=p.material_index) for p in obj.data.polygons])
    return result


def guard_materials(diag):
    for name, data in diag.items():
        require(len(data['slots']) == 1, name, 'owned_slot_count', data)
        for slot in data['slots']:
            require(slot['material'] == EXPECTED_MATERIAL[name], name, 'intended_nonnull_slot', slot)
        for face in data['faces']:
            index = face['material_index']
            require(index == 0 and data['slots'][index]['material'] == EXPECTED_MATERIAL[name],
                    name, 'rendered_face_material', dict(face=face, slots=data['slots']))


def evaluated(name):
    obj = bpy.data.objects[name].evaluated_get(bpy.context.evaluated_depsgraph_get())
    mesh = obj.to_mesh()
    points = [obj.matrix_world @ v.co for v in mesh.vertices]
    normal_matrix = obj.matrix_world.to_3x3().inverted().transposed()
    faces = [dict(id=p.index, indices=list(p.vertices), points=[list(points[i]) for i in p.vertices],
                  normal=list((normal_matrix @ p.normal).normalized()), material=p.material_index,
                  smooth=p.use_smooth) for p in mesh.polygons]
    bounds = [[min(v[i] for v in points), max(v[i] for v in points)] for i in range(3)]
    bm = bmesh.new()
    bm.from_mesh(mesh)
    bm.verts.ensure_lookup_table()
    bad_edges = [e.index for e in bm.edges if not e.is_manifold or not e.is_contiguous]
    remaining = set(bm.verts)
    components = 0
    while remaining:
        components += 1
        stack = [remaining.pop()]
        while stack:
            v = stack.pop()
            for edge in v.link_edges:
                other = edge.other_vert(v)
                if other in remaining:
                    remaining.remove(other)
                    stack.append(other)
    volume = bm.calc_volume(signed=True)
    bm.free()
    canonical = [tuple(sorted(tuple(p) for p in f['points'])) for f in faces]
    result = dict(vertices=[list(v) for v in points], faces=faces, bounds=bounds,
                  matrix=[list(row) for row in obj.matrix_world], invalid_edges=bad_edges,
                  components=components, volume=volume, exact_duplicate_faces=len(canonical)-len(set(canonical)))
    obj.to_mesh_clear()
    return result


def signed_area(poly):
    return sum(a[0]*b[1]-b[0]*a[1] for a,b in zip(poly,poly[1:]+poly[:1])) / 2


def ccw(poly):
    return poly if signed_area(poly) > 0 else list(reversed(poly))


def clip_convex(subject, boundary):
    # Sutherland-Hodgman on actual transformed face XY vertices, CCW interiors left.
    out = ccw(subject)
    boundary = ccw(boundary)
    for a,b in zip(boundary,boundary[1:]+boundary[:1]):
        source, out = out, []
        if not source:
            break
        def distance(p):
            return (b[0]-a[0])*(p[1]-a[1])-(b[1]-a[1])*(p[0]-a[0])
        previous = source[-1]
        dp = distance(previous)
        for point in source:
            dc = distance(point)
            if (dc >= 0) != (dp >= 0):
                fraction = dp / (dp-dc)
                out.append([previous[i]+fraction*(point[i]-previous[i]) for i in (0,1)])
            if dc >= 0:
                out.append(list(point))
            previous, dp = point, dc
    return out


def plane_face(data, up):
    candidates = [f for f in data['faces'] if f['normal'][2]*(1 if up else -1) > 1-1e-6]
    require(len(candidates) == 1, 'planar_contact_selection', 'single_horizontal_face',
            dict(up=up, candidates=candidates))
    return candidates[0]


def contact(upper, lower, data):
    bottom, top = plane_face(data[upper], False), plane_face(data[lower], True)
    zs = [p[2] for p in bottom['points']] + [p[2] for p in top['points']]
    gap = bottom['points'][0][2]-top['points'][0][2]
    normal_dot = Vector(bottom['normal']).dot(Vector(top['normal']))
    patch = clip_convex([p[:2] for p in bottom['points']], [p[:2] for p in top['points']])
    area = abs(signed_area(patch)) if len(patch) >= 3 else 0
    require(max(zs)-min(zs) <= TOL and abs(gap) <= TOL and normal_dot < -.999999 and area > 0,
            upper+'/'+lower,'opposing_contact',dict(gap=gap, normal_dot=normal_dot, patch=patch, area=area,
                                                    bottom_face=bottom, top_face=top))
    triangles = [[patch[0],patch[i],patch[i+1]] for i in range(1,len(patch)-1)]
    triangle_areas = [signed_area(triangle) for triangle in triangles]
    require(signed_area(patch)>0 and all(a>=0 for a in triangle_areas)
            and abs(sum(triangle_areas)-area)<AREA_TOL,
            upper+'/'+lower,'ordered_ccw_contact_triangulation',
            dict(patch=patch,triangle_areas=triangle_areas,area=area))
    return dict(upper=upper,lower=lower,signed_gap=gap,normal_dot=normal_dot,
                bottom_face=bottom,top_face=top,patch_xy=patch,area_m2=area,
                patch_signed_area=signed_area(patch),fan_triangles=triangles,triangle_areas=triangle_areas)


def ray(name, origin, direction):
    obj = bpy.data.objects[name].evaluated_get(bpy.context.evaluated_depsgraph_get())
    inverse = obj.matrix_world.inverted()
    direction = Vector(direction).normalized()
    hit, loc, norm, face = obj.ray_cast(inverse @ Vector(origin),
                                      (inverse.to_3x3() @ direction).normalized(), distance=20)
    return dict(object=name, origin=list(origin), direction=list(direction), hit=hit, face=face,
                position=list(obj.matrix_world @ loc) if hit else None,
                normal=list((obj.matrix_world.to_3x3().inverted().transposed() @ norm).normalized()) if hit else None,
                matrix=[list(row) for row in obj.matrix_world])


def pair(name, a, b, direction):
    first = ray(name,a,direction)
    second = ray(name,b,tuple(-v for v in direction))
    require(first['hit'] and second['hit'],name,'paired_surface_hits',dict(first=first,second=second))
    dot = Vector(first['normal']).dot(Vector(second['normal']))
    require(dot < -.999999,name,'opposing_flat_normals',dict(first=first,second=second,dot=dot))
    return dict(first=first,second=second,opposing_normal_dot=dot,
                distance=(Vector(first['position'])-Vector(second['position'])).length,
                sampled_flat_only=True)


def measurements(tag):
    data = {name:evaluated(name) for name in NAMES}
    for name,d in data.items():
        require(not d['invalid_edges'] and d['components']==1 and d['volume']>0 and d['exact_duplicate_faces']==0,
                name,'closed_wound_single_positive_solid',d)
    expected = {
        'LeftPier':[[-1.86,-1.64],[-.41,.41],[0,2.92]],
        'RightPier':[[1.64,1.86],[-.41,.41],[0,2.92]],
        'Canopy':[[-1.825,1.825],[-.47,.43],[2.92,3.14]],
        'Threshold':[[-1.63,1.63],[-.42,.44],[0,.03]],
        'PresentationFoundationOnly':[[-2.15,2.15],[-.8,.8],[-.08,0]],
    }
    for name, bounds in expected.items():
        for axis in range(3):
            for end in range(2):
                close(data[name]['bounds'][axis][end],bounds[axis][end],name,f'bounds_axis{axis}_end{end}')
    sections = {
        'pier_width':pair('LeftPier',(-2.1,0,1.4),(-1.4,0,1.4),(1,0,0)),
        'pier_depth':pair('LeftPier',(-1.75,-.8,1.4),(-1.75,.8,1.4),(0,1,0)),
        'pier_height':pair('LeftPier',(-1.75,0,-.2),(-1.75,0,3.3),(0,0,1)),
        'canopy_gauge':pair('Canopy',(0,0,2.7),(0,0,3.4),(0,0,1)),
        'threshold_gauge':pair('Threshold',(0,0,-.1),(0,0,.15),(0,0,1)),
        'foundation_gauge':pair('PresentationFoundationOnly',(0,0,-.2),(0,0,.1),(0,0,1)),
    }
    for key,want in [('pier_width',.22),('pier_depth',.82),('pier_height',2.92),('canopy_gauge',.22),
                     ('threshold_gauge',.03),('foundation_gauge',.08)]:
        close(sections[key]['distance'],want,key,'flat_paired_gauge')
    left = ray('LeftPier',(0,0,1.4),(-1,0,0))
    right = ray('RightPier',(0,0,1.4),(1,0,0))
    underside = ray('Canopy',(0,0,1.4),(0,0,1))
    threshold = ray('Threshold',(0,0,1.4),(0,0,-1))
    require(all(r['hit'] for r in (left,right,underside,threshold)), 'Portal','opening_hits',
            dict(left=left,right=right,underside=underside,threshold=threshold))
    clear_width = right['position'][0]-left['position'][0]
    headroom = underside['position'][2]-threshold['position'][2]
    close(clear_width,3.28,'Portal','clear_width')
    close(headroom,2.89,'Portal','headroom_above_threshold')
    gaps = [data['Threshold']['bounds'][0][0]-data['LeftPier']['bounds'][0][1],
            data['RightPier']['bounds'][0][0]-data['Threshold']['bounds'][0][1]]
    for i,gap in enumerate(gaps):
        close(gap,.01,'Threshold',f'noncontact_side_gap{i}')
    side_hits = [dict(pier=ray('LeftPier',(0,0,.015),(-1,0,0)),
                      threshold=ray('Threshold',(-2,0,.015),(1,0,0))),
                 dict(pier=ray('RightPier',(0,0,.015),(1,0,0)),
                      threshold=ray('Threshold',(2,0,.015),(-1,0,0)))]
    for i,hits in enumerate(side_hits):
        require(hits['pier']['hit'] and hits['threshold']['hit'],'Threshold','side_gap_hits',hits)
        gap=abs(hits['pier']['position'][0]-hits['threshold']['position'][0])
        close(gap,.01,'Threshold',f'paired_actual_side_gap{i}')
        hits['distance']=gap
        hits['opposing_normal_dot']=Vector(hits['pier']['normal']).dot(Vector(hits['threshold']['normal']))
        require(hits['opposing_normal_dot']<-.999999,'Threshold','side_gap_opposing_normals',hits)
    passage = [ray(name,(0,-3,1.65),(0,1,0)) for name in NAMES[:-1]]
    require(all(not r['hit'] for r in passage),'Portal','through_open_axis_must_miss',passage)
    contacts = [contact(a,b,data) for a,b in [('LeftPier','PresentationFoundationOnly'),
                ('RightPier','PresentationFoundationOnly'),('Threshold','PresentationFoundationOnly'),
                ('Canopy','LeftPier'),('Canopy','RightPier')]]
    # The octagon loses 4 triangles of area c*c/2; clipped canopy loses two at inner corners.
    for interface in contacts[:2]:
        require(abs(interface['area_m2']-(.22*.82-2*PARAMS['pier_corner']**2))<=AREA_TOL,
                interface['upper'],'octagonal_foot_area_m2',interface)
    for interface in contacts[3:]:
        require(abs(interface['area_m2']-(.185*.82-PARAMS['pier_corner']**2))<=AREA_TOL,
                interface['lower'],'partial_clipped_bearing_area_m2',interface)
        require(len(interface['patch_xy'])==6, interface['lower'],'six_corner_clipped_patch',interface)
    shoulders = [data['Canopy']['bounds'][0][0]-data['LeftPier']['bounds'][0][0],
                 data['RightPier']['bounds'][0][1]-data['Canopy']['bounds'][0][1]]
    for i,shoulder in enumerate(shoulders):
        close(shoulder,.035,'Portal',f'exposed_outer_shoulder{i}')
    result = dict(tolerance_m=TOL, solids=data, flat_sections=sections, contacts=contacts,
                  clear_width=clear_width, headroom_above_threshold=headroom, source_comment_headroom=2.85,
                  opening_hits=dict(left=left,right=right,underside=underside,threshold=threshold),
                  threshold_side_gaps=gaps,through_open_axis=passage, exposed_shoulders=shoulders,
                  paired_side_gap_hits=side_hits, area_tolerance_m2=AREA_TOL,
                  pier_centres=[[(b[0]+b[1])/2 for b in data[n]['bounds']] for n in NAMES[:2]],
                  architecture_outer_width=data['RightPier']['bounds'][0][1]-data['LeftPier']['bounds'][0][0],
                  overall_architecture_height=data['Canopy']['bounds'][2][1],
                  overall_architecture_depth=data['Canopy']['bounds'][1][1]-data['Canopy']['bounds'][1][0],
                  limits='Sampled flat sections, square longitudinal ends, no engineering/accessibility certification.')
    receipt('measurements-'+tag+'.json',result)
    return result


def camera(name, position, target, lens):
    cam = bpy.data.cameras.new('Camera_'+name)
    cam.lens = lens
    cam.clip_start = .01
    cam.clip_end = 100
    obj = bpy.data.objects.new('Camera_'+name,cam)
    bpy.context.scene.collection.objects.link(obj)
    obj.location = position
    obj.rotation_euler = (Vector(target)-obj.location).to_track_quat('-Z','Y').to_euler()


def lighting():
    scene = bpy.context.scene
    world = bpy.data.worlds.new('OwnedNeutralWorld')
    world.use_nodes = True
    bg = world.node_tree.nodes.get('Background')
    bg.inputs['Color'].default_value = (.24,.24,.24,1)
    bg.inputs['Strength'].default_value = .65
    scene.world = world
    for name,pos,energy,size in [('NeutralKey',(-3,-4,7),1100,5),
                                  ('NeutralFill',(4,-1,4),650,4),('NeutralReverse',(-1,5,5),850,4)]:
        light = bpy.data.lights.new(name,'AREA')
        light.energy, light.shape, light.size = energy, 'DISK', size
        light.color = (1,1,1)
        obj = bpy.data.objects.new(name,light)
        scene.collection.objects.link(obj)
        obj.location = pos
        obj.rotation_euler = (Vector((0,0,1.5))-obj.location).to_track_quat('-Z','Y').to_euler()


def render_view(name, output):
    scene = bpy.context.scene
    scene.camera = bpy.data.objects['Camera_'+name]
    scene.render.filepath = str(ROOT/output)
    bpy.data.objects['PresentationFoundationOnly'].hide_render = (name == 'reverse')
    bpy.ops.render.render(write_still=True)
    bpy.data.objects['PresentationFoundationOnly'].hide_render = False


def readback():
    scene = bpy.context.scene
    meshes = {name:evaluated(name) for name in NAMES}
    slots = diagnostic()
    used = set()
    for data in slots.values():
        for slot in data['slots']:
            if slot['material'] is not None:
                used.add(slot['material'])
    materials = {}
    for name in sorted(used):
        m = bpy.data.materials[name]
        node = m.node_tree.nodes.get('Principled BSDF')
        materials[name] = dict(diffuse=list(m.diffuse_color),use_nodes=m.use_nodes,
                              nodes=sorted((n.name,n.bl_idname) for n in m.node_tree.nodes),
                              links=sorted((l.from_node.name,l.from_socket.name,l.to_node.name,l.to_socket.name)
                                           for l in m.node_tree.links),
                              base_colour=list(node.inputs['Base Color'].default_value),
                              roughness=node.inputs['Roughness'].default_value,
                              metallic=node.inputs['Metallic'].default_value,
                              ior=node.inputs['IOR'].default_value,
                              alpha=node.inputs['Alpha'].default_value,
                              emission_colour=list(node.inputs['Emission Color'].default_value),
                              emission_strength=node.inputs['Emission Strength'].default_value)
    lights, cameras, transforms = {},{},{}
    for obj in scene.objects:
        transforms[obj.name] = dict(matrix=[list(row) for row in obj.matrix_world],hide_render=obj.hide_render)
        if obj.type=='LIGHT':
            d=obj.data
            lights[obj.name]=dict(type=d.type,energy=d.energy,color=list(d.color),shape=d.shape,size=d.size)
        if obj.type=='CAMERA':
            d=obj.data
            cameras[obj.name]=dict(type=d.type,lens=d.lens,sensor_width=d.sensor_width,
                                   clip_start=d.clip_start,clip_end=d.clip_end,shift_x=d.shift_x,shift_y=d.shift_y)
    bg=scene.world.node_tree.nodes.get('Background')
    return dict(scene=scene.name,geometry=meshes,slots=slots,used_materials=materials,
                transforms=transforms,lights=lights,cameras=cameras,active_camera=scene.camera.name,
                world=dict(name=scene.world.name,color=list(bg.inputs['Color'].default_value),
                           strength=bg.inputs['Strength'].default_value),
                units=dict(system=scene.unit_settings.system,scale=scene.unit_settings.scale_length),
                render=dict(engine=scene.render.engine,device=scene.cycles.device,samples=scene.cycles.samples,
                            denoising=scene.cycles.use_denoising,resolution_x=scene.render.resolution_x,
                            resolution_y=scene.render.resolution_y,percentage=scene.render.resolution_percentage,
                            threads=scene.render.threads,threads_mode=scene.render.threads_mode,
                            filepath=scene.render.filepath,format=scene.render.image_settings.file_format,
                            colour_mode=scene.render.image_settings.color_mode,
                            colour_depth=scene.render.image_settings.color_depth,
                            view_transform=scene.view_settings.view_transform,look=scene.view_settings.look,
                            exposure=scene.view_settings.exposure,gamma=scene.view_settings.gamma,
                            transparent=scene.render.film_transparent))


def main():
    receipt('initial-identity.json',identity())
    require(bpy.data.filepath=='','Job','fresh_empty_file',identity())
    require(bpy.app.version[:3]==(5,2,2),'Job','installed_version',identity())
    scene=bpy.context.scene
    scene.name='EntranceStonePortal01'
    for obj in list(scene.objects):
        bpy.data.objects.remove(obj,do_unlink=True)
    scene.unit_settings.system='METRIC'
    scene.unit_settings.scale_length=1
    scene.render.engine='CYCLES'
    scene.cycles.device='CPU'
    scene.cycles.samples=24
    scene.cycles.use_denoising=True
    scene.render.threads_mode='FIXED'
    scene.render.threads=6
    scene.render.resolution_x=800
    scene.render.resolution_y=800
    scene.render.resolution_percentage=100
    scene.render.image_settings.file_format='PNG'
    scene.render.image_settings.color_mode='RGB'
    scene.render.image_settings.color_depth='8'
    scene.view_settings.view_transform='AgX'
    scene.view_settings.look='None'
    scene.view_settings.exposure=0
    scene.view_settings.gamma=1
    material('PortalStone','#d8cfbb',.92)
    material('ThresholdStone','#b6b4a5',.92)
    material('FoundationStone','#bdb6a5',.94)
    construct(PARAMS)
    lighting()
    for name,(pos,target,lens) in VIEWS.items():
        camera(name,pos,target,lens)
    scene.camera=bpy.data.objects['Camera_front']
    scene.render.filepath=str(ROOT/'front.png')
    bpy.context.view_layer.update()
    receipt('authored-identity.json',identity())
    bpy.ops.wm.save_as_mainfile(filepath=str(ROOT/'work-in-progress.blend'),check_existing=False)
    receipt('wip-identity.json',identity())
    diag=diagnostic()
    receipt('material-diagnostics-early.json',diag)
    guard_materials(diag)
    measurements('early')
    render_view('front','early-front.png')
    receipt('preview-ready.json',identity())
    # The same Python main thread remains available for explicit geometry reconstruction.
    start=time.monotonic()
    while not (ROOT/'continue.json').exists():
        if time.monotonic()-start > 1800:
            raise RuntimeError('Owned preview decision deadline exceeded; no relaunch')
        time.sleep(1)
    command=json.loads((ROOT/'continue.json').read_text(encoding='utf-8'))
    require(command.get('expected_pid')==os.getpid() and command.get('job')==JOB,
            'Job','correction_command_identity',command)
    if command['action']=='reconstruct-and-finish':
        updates=command['parameters']
        require(set(updates)<=set(PARAMS),'Job','owned_parameter_scope',updates)
        PARAMS.update(updates)
        construct(PARAMS,replace=True)
        measurements('corrected')
        render_view('front','corrected-front.png')
    else:
        require(command['action']=='finish','Job','explicit_action',command)
    receipt('asset-contract.json',dict(parameters=PARAMS,tolerance_m=TOL,source_status='Parent excerpts only; full TS absent Windows',
            axes='+X run, +Z up, front -Y; origin between pier feet; no canonical placement',
            refinement='Authored piers Z0..2.92 replacing source Z.005..2.855; source canopy/threshold envelopes retained',
            materials=EXPECTED_MATERIAL,omissions='No grain/moisture reproduction, doors, return walls, native placement or engineering claims',
            interfaces='Five separate solids; five bearing interfaces; threshold side gaps are NONCONTACT',
            edge_limits='2mm vertical pier corners; 2mm canopy top longitudinal corners; 1mm threshold top longitudinal corners; square X ends')))
    diag=diagnostic()
    receipt('material-diagnostics-final.json',diag)
    guard_materials(diag)
    measurements('final')
    scene=bpy.context.scene
    scene.camera=bpy.data.objects['Camera_front']
    scene.render.filepath=str(ROOT/'front.png')
    bpy.context.view_layer.update()
    before=readback()
    receipt('before-save-readback.json',before)
    final=str(ROOT/'EntranceStonePortal01.blend')
    bpy.ops.wm.save_as_mainfile(filepath=final,check_existing=False)
    receipt('saved-identity.json',identity())
    bpy.ops.wm.open_mainfile(filepath=final)
    bpy.context.view_layer.update()
    receipt('reopened-identity.json',identity())
    after=readback()
    receipt('reopened-readback.json',after)
    require(before==after,'Job','exact_saved_reopened_owned_used_state',dict(equal=before==after))
    receipt('readback-match.json',dict(exact=True,pid=os.getpid(),includes='Evaluated meshes/materials/transforms/lights/world/cameras/render'))
    guard_materials(diagnostic())
    measurements('reopened')
    for name in VIEWS:
        render_view(name,name+'.png')
    bpy.context.scene.camera=bpy.data.objects['Camera_front']
    bpy.context.scene.render.filepath=str(ROOT/'front.png')
    require(after==readback(),'Job','post_capture_same_saved_state',dict(phase='after-five-stills'))
    receipt('completion.json',identity())


try:
    main()
except Exception:
    receipt('failure-identity.json',identity())
    with (ROOT/'exact-error.txt').open('x',encoding='utf-8') as handle:
        handle.write(traceback.format_exc())
    raise

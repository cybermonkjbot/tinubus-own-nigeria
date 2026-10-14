"""Original, source-fitted native compound kit. Isolated Blender worker only.

Usage: blender --background --factory-startup --python this_file -- SURVEY OUTPUT
Units are metres, Z up. Native local X is mirrored explicitly for the existing
FBX importer. No third-party geometry, textures, baking or scene-wide changes.
"""
import hashlib
import json
import math
import os
from pathlib import Path
import runpy
import sys

import bpy
import bmesh
from mathutils import Vector
from mathutils.geometry import tessellate_polygon

HELPER = runpy.run_path(str(Path(__file__).with_name('export_eye_level_kit.py')))
Mesh = HELPER['Mesh']
PALETTE = {
    'plaster': ((.46,.425,.34,1), .87, 0),
    'stone': ((.28,.30,.265,1), .89, 0),
    'iron': ((.045,.052,.045,1), .58, .25),
    'paving': ((.32,.31,.265,1), .92, 0),
    'soil': ((.10,.066,.033,1), .98, 0),
}


def area(polygon):
    return sum(a[0]*b[1]-b[0]*a[1] for a,b in zip(polygon,polygon[1:]+polygon[:1]))/2


def halfplane(poly, a, b, positive):
    result=[]
    def side(p):
        return (b[0]-a[0])*(p[1]-a[1])-(b[1]-a[1])*(p[0]-a[0])
    for p,q in zip(poly,poly[1:]+poly[:1]):
        sp,sq=side(p),side(q)
        pin,qin=(sp>=-1e-7,sq>=-1e-7) if positive else (sp<=1e-7,sq<=1e-7)
        if pin:
            result.append(p)
        if pin!=qin and abs(sp-sq)>1e-10:
            t=sp/(sp-sq)
            result.append((p[0]+(q[0]-p[0])*t,p[1]+(q[1]-p[1])*t))
    return result


def subtract(poly, cutter):
    # Disjoint convex pieces outside a convex cutter; no duplicate overlap faces.
    remaining=poly
    outside=[]
    for a,b in zip(cutter,cutter[1:]+cutter[:1]):
        if len(remaining)<3:
            break
        part=halfplane(remaining,a,b,False)
        if len(part)>=3 and abs(area(part))>.2:
            outside.append(part)
        remaining=halfplane(remaining,a,b,True)
    return outside


def bounds(poly):
    return min(p[0] for p in poly),max(p[0] for p in poly),min(p[1] for p in poly),max(p[1] for p in poly)


def overlaps(a,b):
    return a[0]<b[1] and a[1]>b[0] and a[2]<b[3] and a[3]>b[2]


def rectangle(x0,x1,y0,y1):
    return [(x0,y0),(x1,y0),(x1,y1),(x0,y1)]


def box_polygon(box, padding=0):
    a=math.radians(box['yaw'])
    x,y=box['centre'][:2]
    ex,ey=box['extents'][:2]
    return [(x+dx*math.cos(a)-dy*math.sin(a),y+dx*math.sin(a)+dy*math.cos(a))
            for dx,dy in rectangle(-ex-padding,ex+padding,-ey-padding,ey+padding)]


def to_local(p,origin,yaw):
    a=math.radians(yaw)
    x,y=p[0]-origin[0],p[1]-origin[1]
    return x*math.cos(a)+y*math.sin(a),-x*math.sin(a)+y*math.cos(a)


def fitted_surface(polygon, cutters, z):
    if area(polygon)<0:
        polygon=list(reversed(polygon))
    triangles=tessellate_polygon([[Vector((x,y,0)) for x,y in polygon]])
    # Blender 5.2 returns flattened input indices; earlier versions return vectors.
    pieces=[[polygon[v] if isinstance(v,int) else (v.x,v.y) for v in tri] for tri in triangles]
    pieces=[p if area(p)>0 else list(reversed(p)) for p in pieces]
    for cutter in cutters:
        if area(cutter)<0:
            cutter=list(reversed(cutter))
        cb=bounds(cutter)
        new=[]
        for piece in pieces:
            new.extend(subtract(piece,cutter) if overlaps(bounds(piece),cb) else [piece])
        pieces=new
    m=Mesh()
    for piece in pieces:
        if area(piece)<0:
            piece=list(reversed(piece))
        # Blender coordinates mirror native X, reversing winding restores +Z.
        m.face([(-x/100,y/100,z/100) for x,y in reversed(piece)],'soil')
    if not m.f:
        raise RuntimeError('All fitted ground was excluded')
    return m, sum(abs(area(p)) for p in pieces)/10000


def author(kind):
    m=Mesh()
    if kind=='WallPanel':
        m.box((0,0,.71),(2.4,.18,1.42),'plaster')
        m.box((0,-.098,.21),(2.4,.025,.30),'plaster')
        m.box((0,.098,.21),(2.4,.025,.30),'plaster')
    elif kind=='Coping':
        m.box((0,0,1.47),(2.4,.26,.10),'stone')
    elif kind=='Pier':
        m.box((0,0,.82),(.28,.28,1.64),'plaster')
        m.box((0,0,1.69),(.36,.36,.10),'stone')
        m.box((0,0,.11),(.32,.32,.22),'stone')
    elif kind=='GateLeaf':
        # Horizontal X panel; its hinge end is X=-1.05. Positioned folded inward.
        for x in [-1.05,1.05]:
            m.box((x,0,.76),(.06,.055,1.48),'iron')
        for z in [.07,.42,1.45]:
            m.box((0,0,z),(2.10,.055,.055),'iron')
        for i in range(13):
            m.box((-.96+i*.16,0,.92),(.027,.034,1.01),'iron')
        m.box((0,0,.24),(2.06,.035,.30),'iron')
        for z in [.20,1.22]:
            m.tube((-1.07,.01,z),(-1.07,.01,z+.09),.027,'iron',n=8)
        m.box((.90,-.045,.80),(.11,.04,.03),'iron')
    elif kind=='DrainGrate':
        for y in [-.145,.145]:
            m.box((0,y,.01),(1.20,.035,.025),'iron')
        for x in [-.59,.59]:
            m.box((x,0,.01),(.035,.30,.025),'iron')
        for i in range(17):
            m.box((-.55+i*.06875,0,.01),(.018,.29,.025),'iron')
    elif kind=='Paving':
        # One metre tread with a restrained real joint and bevel at the edge.
        m.box((0,0,-.025),(.995,.995,.05),'paving')
    elif kind=='BedPlanter':
        m.box((0,0,.075),(2.4,1.0,.15),'soil')
        for y in [-.51,.51]:
            m.box((0,y,.085),(2.54,.07,.17),'stone')
        for x in [-1.235,1.235]:
            m.box((x,0,.085),(.07,1.0,.17),'stone')
    else:
        raise ValueError(kind)
    return m


def main():
    if bpy.data.filepath or not bpy.app.background:
        raise RuntimeError('Only run in an isolated factory-startup worker')
    print(json.dumps(runpy.run_path(str(Path(__file__).with_name('probe_scene.py')))['inspect_scene'](limit=5)),flush=True)
    args=sys.argv[sys.argv.index('--')+1:]
    source, destination=Path(args[0]),Path(args[1])
    if (destination/'manifest.json').exists():
        raise RuntimeError('Preserve the completed kit; choose a new version')
    data=json.loads(source.read_text())
    destination.mkdir(parents=True,exist_ok=True)
    scene=bpy.data.scenes.new('StreetFinish_Forecourts_20261014')
    bpy.context.window.scene=scene
    scene.unit_settings.system='METRIC'
    scene.unit_settings.scale_length=1
    materials={}
    for name,(color,roughness,metallic) in PALETTE.items():
        mat=bpy.data.materials.new(name)
        mat.diffuse_color=color; mat.roughness=roughness; mat.metallic=metallic
        materials[name]=mat
    assets=[('SM_FC_'+kind,author(kind),kind!='BedPlanter',{}) for kind in ['WallPanel','Coping','Pier','GateLeaf','DrainGrate','Paving','BedPlanter']]
    for p in data['plans']:
        poly=rectangle(p['front'],p['back'],-p['halfWidth'],p['halfWidth'])
        cutters=[rectangle(p['shellMin'][0],p['shellMax'][0],p['shellMin'][1],p['shellMax'][1])]
        for box in data['protectedGround']:
            if box['label'].endswith('_support') or (box['label'].endswith('_footprint') and p['parcelId'] not in box['label']):
                cutter=[to_local(v,p['originCm'],p['yaw']) for v in box_polygon(box,12)]
                if overlaps(bounds(poly),bounds(cutter)):
                    cutters.append(cutter)
        m,area_m2=fitted_surface(poly,cutters,1.0)
        assets.append(('SM_FC_Yard_'+p['parcelId'].replace('-','_'),m,False,{'surfaceAreaM2':area_m2,'source':p['parcelId']}))
    for park in data['parks']:
        poly=[to_local(p,park['centre'],0) for p in park['polygon']]
        cutters=[]
        for box in data['protectedGround']:
            cutter=[to_local(p,park['centre'],0) for p in box_polygon(box,18)]
            if overlaps(bounds(poly),bounds(cutter)):
                cutters.append(cutter)
        m,area_m2=fitted_surface(poly,cutters,park['topZ'])
        assets.append(('SM_FC_Park_'+park['id'].replace('-','_'),m,False,{'surfaceAreaM2':area_m2,'source':park['id']}))
    if len(args)>2:
        requested=set(args[2].split(','))
        if not requested.issubset({item[0] for item in assets}):
            raise ValueError('Unknown asset selection')
        assets=[item for item in assets if item[0] in requested]
    report={'pid':os.getpid(),'blender':bpy.app.version_string,'source_kind':'Original bevelled compound kit and source-fitted ground; no external assets',
            'surveySHA256':hashlib.sha256(source.read_bytes()).hexdigest(),'units':'metres','forward':'-Y','up':'Z',
            'materials':{k:{'base_color':v[0],'roughness':v[1],'metallic':v[2]} for k,v in PALETTE.items()},'assets':[]}
    for index,(name,authored,bevel,metadata) in enumerate(assets):
        mesh=bpy.data.meshes.new(name)
        mesh.from_pydata(authored.v,[],authored.f)
        used=sorted(set(authored.m))
        for material in used:
            mesh.materials.append(materials[material])
        for polygon,material in zip(mesh.polygons,authored.m):
            polygon.material_index=used.index(material)
        bm=bmesh.new();bm.from_mesh(mesh)
        bmesh.ops.remove_doubles(bm,verts=list(bm.verts),dist=.000001)
        if bevel:
            bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces))
            bmesh.ops.bevel(bm,geom=list(bm.edges),offset=.006,segments=1,affect='EDGES')
        bmesh.ops.triangulate(bm,faces=list(bm.faces))
        bm.to_mesh(mesh);bm.free();mesh.update()
        obj=bpy.data.objects.new(name,mesh);scene.collection.objects.link(obj)
        bpy.ops.object.select_all(action='DESELECT');obj.select_set(True);bpy.context.view_layer.objects.active=obj
        measured_bounds={'min':[min(v.co[i] for v in mesh.vertices) for i in range(3)],'max':[max(v.co[i] for v in mesh.vertices) for i in range(3)]}
        lods=[]
        # These are already economical architectural meshes. Preserve thin gate
        # bars and exact ground boundaries rather than decimating their silhouette.
        for level in range(3):
            mesh.calc_loop_triangles()
            path=destination/f'{name}_LOD{level}.fbx'
            bpy.ops.export_scene.fbx(filepath=str(path),use_selection=True,object_types={'MESH'},apply_unit_scale=True,
                apply_scale_options='FBX_SCALE_UNITS',axis_forward='-Y',axis_up='Z',bake_anim=False,
                use_mesh_modifiers=True,mesh_smooth_type='FACE',path_mode='AUTO')
            lods.append({'file':path.name,'triangles':len(mesh.loop_triangles),'sha256':hashlib.sha256(path.read_bytes()).hexdigest()})
        report['assets'].append({'id':name,'source_id':name,'materials':used,'bounds':measured_bounds,'lods':lods,**metadata})
        obj.location.y=index*8
        print(json.dumps(report['assets'][-1]),flush=True)
    bpy.ops.wm.save_as_mainfile(filepath=str(destination/'Forecourt_Kit.blend'))
    (destination/'manifest.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({'complete':True,'assets':len(assets),'pid':os.getpid()}),flush=True)


if __name__=='__main__':
    main()

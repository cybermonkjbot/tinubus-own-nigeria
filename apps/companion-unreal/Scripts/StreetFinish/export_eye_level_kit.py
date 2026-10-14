"""Author a native-only small-plant kit in an isolated Blender worker.

Run --background --factory-startup --python this_file -- OUTPUT_DIRECTORY.
Metres, Z up, front -Y. No external assets, bakes, renders or shared renderer edits.
"""
import hashlib
import json
import math
import os
from pathlib import Path
import random
import runpy
import sys

import bpy
import bmesh
from mathutils import Vector

PALETTE = {
    'terracotta': ((.34, .115, .055, 1), .82, 0),
    'stone': ((.31, .33, .285, 1), .84, 0),
    'soil': ((.055, .030, .014, 1), .98, 0),
    'leaf_dark': ((.035, .135, .045, 1), .74, 0),
    'leaf_light': ((.12, .25, .075, 1), .7, 0),
    'leaf_gold': ((.27, .32, .09, 1), .76, 0),
    'flower_coral': ((.65, .16, .13, 1), .82, 0),
    'flower_cream': ((.72, .62, .34, 1), .8, 0),
    'iron': ((.035, .042, .038, 1), .48, .35),
}


class Mesh:
    def __init__(self):
        self.v, self.f, self.m = [], [], []

    def face(self, points, material):
        start = len(self.v)
        self.v.extend(tuple(p) for p in points)
        for i in range(1, len(points) - 1):
            self.f.append((start, start + i, start + i + 1))
            self.m.append(material)

    def tube(self, a, b, r, material, n=6, end_radius=None):
        a, b = Vector(a), Vector(b)
        direction = (b - a).normalized()
        ref = Vector((0, 0, 1)) if abs(direction.z) < .9 else Vector((1, 0, 0))
        side = direction.cross(ref).normalized()
        up = direction.cross(side).normalized()
        rr = end_radius if end_radius is not None else r
        rings = [[p + (side * math.cos(i * math.tau / n) + up * math.sin(i * math.tau / n)) * radius
                  for i in range(n)] for p, radius in [(a, r), (b, rr)]]
        for i in range(n):
            j = (i + 1) % n
            self.face([rings[0][i], rings[0][j], rings[1][j], rings[1][i]], material)
        self.face(list(reversed(rings[0])), material)
        self.face(rings[1], material)

    def box(self, centre, size, material):
        x, y, z = centre
        a, b, c = [v / 2 for v in size]
        v = [(x + dx*a, y + dy*b, z + dz*c) for dx, dy, dz in
             [(-1,-1,-1),(1,-1,-1),(1,1,-1),(-1,1,-1),(-1,-1,1),(1,-1,1),(1,1,1),(-1,1,1)]]
        for f in [(0,3,2,1),(4,5,6,7),(0,1,5,4),(1,2,6,5),(2,3,7,6),(3,0,4,7)]:
            self.face([v[i] for i in f], material)

    def pot(self, centre, radius, height, material='terracotta', n=20):
        x, y, z = centre
        profile = [(radius*.70,0),(radius*.73,.025),(radius*.96,height*.84),
                   (radius,height*.85),(radius,height),(radius*.84,height),
                   (radius*.82,height*.90)]
        rings = [[(x + r*math.cos(i*math.tau/n), y + r*math.sin(i*math.tau/n), z+h)
                  for i in range(n)] for r,h in profile]
        self.face(list(reversed(rings[0])), material)
        for first, second in zip(rings, rings[1:]):
            for i in range(n):
                j=(i+1)%n
                self.face([first[i],first[j],second[j],second[i]], material)
        self.face(rings[-1], 'soil')

    def leaf(self, start, end, width, material):
        a, b = Vector(start), Vector(end)
        direction = (b-a).normalized()
        side = direction.cross(Vector((0,0,1)))
        if side.length < .01:
            side = Vector((1,0,0))
        side.normalize()
        mid = a.lerp(b,.46)
        left, right = mid + side*width, mid-side*width
        ridge = mid + Vector((0,0,width*.16))
        underside = mid - Vector((0,0,.0015))
        for p,q in [(a,left),(left,b),(b,right),(right,a)]:
            self.face([p,q,ridge],material)
            self.face([q,p,underside],material)


def fern(mesh, base, seed, reach=.34, count=12):
    rng = random.Random(seed)
    base = Vector(base)
    for i in range(count):
        angle = math.tau*i/count + rng.uniform(-.2,.2)
        d = Vector((math.cos(angle),math.sin(angle),0))
        side = Vector((-d.y,d.x,0))
        r=reach*rng.uniform(.8,1.15)
        curve=[base+d*r*t+Vector((0,0,reach*(1.45*t-1.15*t*t))) for t in [j/8 for j in range(9)]]
        for a,b in zip(curve,curve[1:]):
            mesh.tube(a,b,.003,'leaf_dark',n=4)
        for j in range(1,8):
            t=j/8
            for sign in [-1,1]:
                end=curve[j]+side*sign*reach*.36*(1-t*.7)+d*reach*.09+Vector((0,0,.03))
                mesh.leaf(curve[j],end,reach*.035,'leaf_light' if (i+j)%4==0 else 'leaf_dark')


def grasses(mesh, base, seed, height=.45, count=26, radius=.17):
    rng=random.Random(seed)
    base=Vector(base)
    for i in range(count):
        angle=rng.random()*math.tau
        direction=Vector((math.cos(angle),math.sin(angle),0))
        start=base+direction*radius*rng.uniform(.1,.75)
        h=height*rng.uniform(.58,1)
        middle=start+direction*h*.16+Vector((0,0,h*.6))
        end=start+direction*h*.47+Vector((0,0,h))
        mesh.leaf(start,middle,.014,'leaf_light' if i%4 else 'leaf_gold')
        mesh.leaf(middle,end,.010,'leaf_light' if i%4 else 'leaf_gold')


def author(kind):
    mesh=Mesh()
    if kind == 'FernPot':
        mesh.pot((0,0,0),.21,.34)
        fern(mesh,(0,0,.308),41,reach=.35)
    elif kind == 'UprightPot':
        mesh.pot((0,0,0),.22,.42,'stone')
        rng=random.Random(52)
        for i in range(15):
            a=i*math.tau/15
            start=Vector((math.cos(a)*.09,math.sin(a)*.09,.38))
            end=start+Vector((math.cos(a)*.13,math.sin(a)*.13,rng.uniform(.55,.94)))
            mesh.leaf(start,end,.037,'leaf_gold' if i%4==0 else 'leaf_dark')
    elif kind == 'GrassClump':
        grasses(mesh,(0,0,0),63)
    elif kind == 'Groundcover':
        fern(mesh,(0,0,.015),71,reach=.16,count=8)
    elif kind in {'FlowerTrough','GrassTrough'}:
        length=1.05 if kind == 'FlowerTrough' else 1.35
        mesh.box((0,0,.04),(length,.44,.08),'stone')
        for y in [-.22,.22]:
            mesh.box((0,y,.24),(length,.055,.42),'stone')
        for x in [-length/2+.03,length/2-.03]:
            mesh.box((x,0,.24),(.06,.44,.42),'stone')
        mesh.box((0,0,.36),(length-.12,.39,.035),'soil')
        for i in range(3):
            x=(i-1)*length*.28
            if kind == 'GrassTrough':
                grasses(mesh,(x,0,.38),85+i,height=.73,count=28)
            else:
                fern(mesh,(x,0,.38),93+i,reach=.18,count=6)
                for j in range(4):
                    a=j*math.tau/4+i
                    base=Vector((x,0,.38))
                    tip=base+Vector((.09*math.cos(a),.12*math.sin(a),.22+.04*(j%2)))
                    mesh.tube(base,tip,.004,'leaf_dark',n=4)
                    for p in range(5):
                        pa=p*math.tau/5
                        mesh.leaf(tip,tip+Vector((.038*math.cos(pa),.038*math.sin(pa),.014)),.018,
                                  'flower_coral' if i%2 else 'flower_cream')
    elif kind == 'WallBasket':
        # Origin is the wall fixing; bracket projects along -Y, basket hangs below it.
        mesh.box((0,-.015,0),(.09,.03,.27),'iron')
        mesh.tube((0,-.025,.10),(0,-.39,.10),.012,'iron')
        mesh.tube((0,-.025,-.11),(0,-.31,.10),.009,'iron')
        mesh.pot((0,-.36,-.43),.19,.19,'terracotta',n=18)
        for a in [0,math.tau/3,math.tau*2/3]:
            mesh.tube((0,-.36,.095),(.16*math.cos(a),-.36+.16*math.sin(a),-.25),.004,'iron',n=4)
        fern(mesh,(0,-.36,-.25),103,reach=.20,count=8)
        for i in range(7):
            a=i*math.tau/7
            base=Vector((.14*math.cos(a),-.36+.14*math.sin(a),-.26))
            for j in range(5):
                p=base+Vector((.017*math.sin(j+i),-.012*j,-.068*j))
                q=base+Vector((.017*math.sin(j+1+i),-.012*(j+1),-.068*(j+1)))
                mesh.tube(p,q,.003,'leaf_dark',n=4)
                mesh.leaf(p,p+Vector(((-1)**j*.06,-.025,-.04)),.024,'leaf_light')
    else:
        raise ValueError(kind)
    return mesh


def main():
    if bpy.data.filepath or not bpy.app.background:
        raise RuntimeError('Only run in a new isolated background worker')
    print(json.dumps(runpy.run_path(str(Path(__file__).with_name('probe_scene.py')))['inspect_scene'](limit=5)),flush=True)
    destination=Path(sys.argv[sys.argv.index('--')+1])
    if (destination/'manifest.json').exists():
        raise RuntimeError('Completed kit already exists; preserve the accepted version')
    destination.mkdir(parents=True,exist_ok=True)
    scene=bpy.data.scenes.new('StreetFinish_EyeLevel_20261014')
    bpy.context.window.scene=scene
    scene.unit_settings.system='METRIC'
    scene.unit_settings.scale_length=1
    materials={}
    for name,(color,roughness,metallic) in PALETTE.items():
        mat=bpy.data.materials.new(name)
        mat.diffuse_color=color
        mat.roughness=roughness
        mat.metallic=metallic
        materials[name]=mat
    report={'pid':os.getpid(),'blender':bpy.app.version_string,'source_kind':'Original native small-plant recipes, seed-stable; no external assets',
            'units':'metres','forward':'-Y','up':'Z','materials':{k:{'base_color':v[0],'roughness':v[1],'metallic':v[2]} for k,v in PALETTE.items()},'assets':[]}
    for row,kind in enumerate(['FernPot','UprightPot','GrassClump','Groundcover','FlowerTrough','GrassTrough','WallBasket']):
        authored=author(kind)
        name='SM_EL_'+kind
        mesh=bpy.data.meshes.new(name)
        mesh.from_pydata(authored.v,[],authored.f)
        used=sorted(set(authored.m))
        for m in used:
            mesh.materials.append(materials[m])
        for polygon,m in zip(mesh.polygons,authored.m):
            polygon.material_index=used.index(m)
        bm=bmesh.new()
        bm.from_mesh(mesh)
        bmesh.ops.remove_doubles(bm,verts=list(bm.verts),dist=.00001)
        bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces))
        bm.to_mesh(mesh)
        bm.free()
        mesh.update()
        obj=bpy.data.objects.new(name,mesh)
        scene.collection.objects.link(obj)
        bpy.ops.object.select_all(action='DESELECT')
        obj.select_set(True)
        bpy.context.view_layer.objects.active=obj
        bounds={'min':[min(v.co[i] for v in mesh.vertices) for i in range(3)],'max':[max(v.co[i] for v in mesh.vertices) for i in range(3)]}
        lods=[]
        for level,ratio in enumerate([1,.55,.28]):
            modifier=None
            if level:
                modifier=obj.modifiers.new('Delivery_LOD','DECIMATE')
                modifier.ratio=ratio
            bpy.context.view_layer.update()
            evaluated=obj.evaluated_get(bpy.context.evaluated_depsgraph_get())
            measured=evaluated.to_mesh()
            measured.calc_loop_triangles()
            triangles=len(measured.loop_triangles)
            evaluated.to_mesh_clear()
            path=destination/f'{name}_LOD{level}.fbx'
            bpy.ops.export_scene.fbx(filepath=str(path),use_selection=True,object_types={'MESH'},apply_unit_scale=True,
                apply_scale_options='FBX_SCALE_UNITS',axis_forward='-Y',axis_up='Z',bake_anim=False,
                use_mesh_modifiers=True,mesh_smooth_type='FACE',path_mode='AUTO')
            lods.append({'file':path.name,'triangles':triangles,'sha256':hashlib.sha256(path.read_bytes()).hexdigest()})
            if modifier:
                obj.modifiers.remove(modifier)
        report['assets'].append({'id':name,'source_id':kind,'materials':used,'bounds':bounds,'lods':lods})
        obj.location.x=row*2
        print(json.dumps(report['assets'][-1]),flush=True)
    bpy.ops.wm.save_as_mainfile(filepath=str(destination/'EyeLevel_Kit.blend'))
    (destination/'manifest.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({'complete':True,'assets':len(report['assets']),'pid':os.getpid()}),flush=True)


if __name__=='__main__':
    main()

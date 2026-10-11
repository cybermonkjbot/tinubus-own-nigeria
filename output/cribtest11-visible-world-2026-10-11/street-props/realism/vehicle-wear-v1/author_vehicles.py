"""Additive appearance-only vehicle library; one owned offline Blender batch."""
import argparse
from datetime import datetime, timezone
import hashlib
import json
import math
import os
from pathlib import Path
import random
import runpy
import shutil
import sys
import time
import traceback
import bmesh
import bpy
from mathutils import Matrix, Vector

JOB = 'lagos-vehicle-wear-v1'
ROOT = Path('C:/dev/cribtest-game/output/cribtest11-visible-world-2026-10-11')
OUT = ROOT/'street-props/realism/vehicle-wear-v1'
SOURCE = ROOT/'street-props/realism/danfo-detail-v1/DanfoDetail_v1.blend'
SOURCE_HASH = '8e5c0d4758bd8b92e7525328410998f3fcf6a907919a95048ac5ee825b3546c8'
MEDIA = ROOT/'material-inputs-verified'
PROBE = Path('C:/Users/cribadmin/.codex/skills/blender-production/scripts/probe_scene.py')
MATS, MAT_REPORT, VEHICLES, FRAMES = {}, {}, [], []

def digest(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream,'sha256').hexdigest()

def write(path,value):
    temporary=path.with_name(path.name+'.tmp')
    temporary.write_text(json.dumps(value,indent=2)+'\n',encoding='utf-8')
    temporary.replace(path)

def progress(step,**extra):
    record={'job':JOB,'pid':os.getpid(),'step':step,'updated_at':datetime.now(timezone.utc).isoformat(),**extra}
    write(OUT/'progress.json',record)
    print(json.dumps(record),flush=True)

def material(name,color,rough=.5,metal=0,coat=0,trans=0):
    mat=bpy.data.materials.new('VW_'+name)
    mat.use_nodes=True
    mat.diffuse_color=(*color,1)
    shader=mat.node_tree.nodes.get('Principled BSDF')
    for key,val in [('Base Color',(*color,1)),('Roughness',rough),('Metallic',metal),
                    ('Coat Weight',coat),('Coat Roughness',.3),('Transmission Weight',trans),('IOR',1.46)]:
        shader.inputs[key].default_value=val
    MATS[name]=mat
    MAT_REPORT[name]={'slot':mat.name,'color_linear':list(color),'roughness':rough,'metallic':metal,
                      'coat':coat,'transmission':trans,'uv':'SurfaceUV','coverage_m':[1,1],
                      'provenance':'Original authored physical shader constants; not an automotive paint scan','maps':{}}
    return mat

def scanned(name,asset,coverage,normal=True):
    spec=next(x for x in json.loads((MEDIA/'material-manifest.json').read_text())['materials'] if x['id']==asset)
    mat=material(name,(.3,.3,.3),.75,1 if name=='ExposedOxidizedMetal' else 0)
    report=MAT_REPORT[name]
    report.update(provenance='Coordinator supplied Poly Haven originals',license='CC0-1.0',authors=spec['authors'],coverage_m=coverage)
    nodes,links=mat.node_tree.nodes,mat.node_tree.links
    shader=nodes.get('Principled BSDF')
    uv=nodes.new('ShaderNodeUVMap');uv.uv_map='SurfaceUV'
    for role,channel in [('Diffuse','base_color'),('Rough','roughness'),('nor_gl','normal_gl')]:
        if role=='nor_gl' and not normal:continue
        info=next(x for x in spec['files'] if x['role']==role)
        source=MEDIA/info['path']
        if digest(source)!=info['sha256']:raise RuntimeError('Media hash mismatch '+str(source))
        dest=OUT/'textures'/source.name
        if not dest.exists():shutil.copyfile(source,dest)
        image=bpy.data.images.load(str(dest),check_existing=True)
        image.colorspace_settings.name='sRGB' if role=='Diffuse' else 'Non-Color'
        image.pack()
        tex=nodes.new('ShaderNodeTexImage');tex.image=image
        links.new(uv.outputs['UV'],tex.inputs['Vector'])
        if role=='Diffuse':links.new(tex.outputs['Color'],shader.inputs['Base Color'])
        elif role=='Rough':links.new(tex.outputs['Color'],shader.inputs['Roughness'])
        else:
            n=nodes.new('ShaderNodeNormalMap');n.uv_map='SurfaceUV';n.inputs['Strength'].default_value=.6
            links.new(tex.outputs['Color'],n.inputs['Color']);links.new(n.outputs['Normal'],shader.inputs['Normal'])
        report['maps'][channel]={'file':'textures/'+dest.name,'sha256':digest(dest),'source':info['source'],
                                 'color_space':image.colorspace_settings.name,'native_gl_green_flip_once':role=='nor_gl'}
    if name=='ExposedOxidizedMetal':
        report['limit']='Weathered corrugated iron colour/roughness applied only to small exposed repairs; scan normal omitted, not a measured car-body coating'
    return mat

def palette():
    for args in [
        ('YellowMaintained',(.71,.40,.016),.43,0,.12),('YellowFaded',(.52,.31,.028),.61),
        ('YellowRepair',(.64,.39,.070),.54),('YellowBleached',(.58,.42,.14),.67),
        ('BlackStripe',(.016,.019,.016),.56),('Rubber',(.021,.023,.025),.81),
        ('Steel',(.26,.29,.29),.46,1),('RimOld',(.19,.20,.19),.61,.92),
        ('RustSeam',(.14,.047,.015),.89),('Primer',(.34,.32,.25),.75),
        ('GlassClear',(.90,.96,.95),.045,0,0,1),('GlassAged',(.85,.91,.88),.095,0,0,1),
        ('Mirror',(.78,.81,.81),.055,1),('GlassEdgeFilm',(.27,.29,.24),.36,0,0,.55),
        ('SeatCloth',(.12,.105,.078),.91),('SeatRepair',(.24,.20,.13),.94),
        ('LampGlass',(.79,.80,.69),.2,0,0,.42),('Amber',(.60,.15,.009),.34),
        ('RedLamp',(.32,.013,.008),.29),('Ivory',(.68,.67,.57),.66),
        ('SedanBlue',(.075,.13,.16),.44,0,.16),('SedanFaded',(.13,.19,.21),.61),
        ('SedanPanel',(.105,.145,.16),.54),('Battery',(.035,.038,.037),.71),
        ('Hose',(.03,.035,.033),.77),('RoadsideDust',(.19,.16,.10),.91)]:
        material(*args)
    scanned('ExposedOxidizedMetal','corrugated_iron_02',[2.700000047683716]*2,False)
    scanned('OldSeatBacking','wood_planks_grey',[1.5,1.5000001192092896],False)
    scanned('Asphalt','worn_asphalt',[2,2],True)

def uvmesh(data,mat):
    layer=data.uv_layers.get('SurfaceUV') or data.uv_layers.new(name='SurfaceUV')
    coverage=MAT_REPORT[mat]['coverage_m']
    data.update()
    for face in data.polygons:
        axis=max(range(3),key=lambda a:abs(face.normal[a]));axes=[a for a in range(3) if a!=axis]
        for loop in face.loop_indices:
            co=data.vertices[data.loops[loop].vertex_index].co
            layer.data[loop].uv=(co[axes[0]]/coverage[0],co[axes[1]]/coverage[1])

def mesh(v,name,vertices,faces,mat,bevel=0,smooth=False):
    data=bpy.data.meshes.new(v['id']+'_'+name)
    data.from_pydata(vertices,[],faces)
    bm=bmesh.new();bm.from_mesh(data);bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces));bm.to_mesh(data);bm.free()
    data.materials.append(MATS[mat]);uvmesh(data,mat)
    for face in data.polygons:face.use_smooth=smooth
    obj=bpy.data.objects.new(data.name,data);v['collection'].objects.link(obj);obj.parent=v['root']
    obj['derivative_id']=v['id'];obj['part_role']=name
    if bevel:
        mod=obj.modifiers.new('PhysicalEdgeRadius','BEVEL');mod.width=bevel;mod.segments=2
    v['parts'].append(obj)
    return obj

def box(v,name,c,s,mat,bevel=.003):
    x,y,z=c;a,b,d=[q*.5 for q in s]
    verts=[(x+i*a,y+j*b,z+k*d) for i,j,k in [(-1,-1,-1),(1,-1,-1),(1,1,-1),(-1,1,-1),(-1,-1,1),(1,-1,1),(1,1,1),(-1,1,1)]]
    return mesh(v,name,verts,[(0,3,2,1),(4,5,6,7),(0,1,5,4),(1,2,6,5),(2,3,7,6),(3,0,4,7)],mat,bevel)

def slab(v,name,outline,offset,mat,bevel=.001):
    n=len(outline);verts=list(outline)+[tuple(Vector(p)+Vector(offset)) for p in outline]
    return mesh(v,name,verts,[tuple(reversed(range(n))),tuple(range(n,2*n))]+[(i,(i+1)%n,(i+1)%n+n,i+n) for i in range(n)],mat,bevel)

def tube(v,name,points,radius,mat,sides=10):
    points=[Vector(p) for p in points];verts=[]
    for i,p in enumerate(points):
        t=(points[min(i+1,len(points)-1)]-points[max(i-1,0)]).normalized()
        guide=Vector((0,0,1)) if abs(t.z)<.95 else Vector((1,0,0))
        u=t.cross(guide).normalized();w=t.cross(u).normalized()
        verts.extend(tuple(p+radius*(math.cos(j*math.tau/sides)*u+math.sin(j*math.tau/sides)*w)) for j in range(sides))
    faces=[tuple(reversed(range(sides))),tuple((len(points)-1)*sides+j for j in range(sides))]
    for i in range(len(points)-1):
        faces.extend((i*sides+j,i*sides+(j+1)%sides,(i+1)*sides+(j+1)%sides,(i+1)*sides+j) for j in range(sides))
    return mesh(v,name,verts,faces,mat,smooth=True)

def entry(identity):
    scene=bpy.data.scenes.new(identity+'_Authoring');scene.unit_settings.system='METRIC';scene.unit_settings.scale_length=1
    scene.unit_settings.length_unit='METERS'
    col=bpy.data.collections.new(identity+'_Editable');scene.collection.children.link(col)
    root=bpy.data.objects.new(identity+'_ROOT',None);col.objects.link(root)
    root['authoring_job']=JOB;root['derivative_id']=identity;root['canonical_assignment']='none; unplaced authoring asset'
    return {'id':identity,'scene':scene,'collection':col,'root':root,'parts':[],'notes':[]}

def assign(obj,key):
    obj.data.materials.clear();obj.data.materials.append(MATS[key]);uvmesh(obj.data,key)

def remove(v,obj):
    v['parts'].remove(obj);bpy.data.objects.remove(obj,do_unlink=True)

def wheel(v,side,axle,radius=.34,center=.34,track=.96,flat=False):
    segments=96;verts=[];faces=[]
    profile=[(-.113,.60),(-.105,.85),(-.082,.97),(-.055,1),(-.022,1),(.022,1),(.055,1),(.083,.97),(.103,.85),(.110,.60)]
    for ring,(dx,r) in enumerate(profile):
        for i in range(segments):
            theta=math.tau*i/segments
            sipe=.003 if 3<=ring<=6 and (i+ring)%7==0 else 0
            rr=radius*r-sipe
            y=axle+rr*math.cos(theta);z=center+rr*math.sin(theta)
            x=side*(track+dx)
            if flat:
                z=max(0,z)
                if math.sin(theta)<-.5:x+=side*.035*(-math.sin(theta))
            verts.append((x,y,z))
            if ring:
                a=(ring-1)*segments+i;b=(ring-1)*segments+(i+1)%segments
                faces.append((a,b,b+segments,a+segments))
    for i in range(segments):faces.append((i,(i+1)%segments,(len(profile)-1)*segments+(i+1)%segments,(len(profile)-1)*segments+i))
    tyre=mesh(v,'Tyre_%s_%s'%(side,axle),verts,faces,'Rubber',smooth=True)
    for ringname,prof in [('RimLip',[(track+.111,radius*.57),(track+.121,radius*.62),(track+.130,radius*.60),(track+.131,radius*.48)]),
                          ('Hub',[(track+.115,.042),(track+.146,.042),(track+.146,.020)])]:
        vv=[];ff=[]
        for ring,(dist,r) in enumerate(prof):
            for i in range(64):
                theta=math.tau*i/64;vv.append((side*dist,axle+r*math.cos(theta),center+r*math.sin(theta)))
                if ring:
                    a=(ring-1)*64+i;b=(ring-1)*64+(i+1)%64;ff.append((a,b,b+64,a+64))
        mesh(v,ringname+'_%s_%s'%(side,axle),vv,ff,'RimOld',smooth=True)
    tube(v,'BrakeDisc_%s_%s'%(side,axle),[(side*(track+.094),axle,center),(side*(track+.101),axle,center)],radius*.48,'Steel',40)
    for i in range(8):
        theta=i*math.tau/8
        outline=[]
        for r,angle in [(.04,theta-.28),(radius*.51,theta-.14),(radius*.51,theta+.14),(.04,theta+.28)]:
            outline.append((side*(track+.116),axle+r*math.cos(angle),center+r*math.sin(angle)))
        slab(v,'RimSpoke_%s_%s_%s'%(side,axle,i),outline,(side*.008,0,0),'RimOld')
        y=axle+.074*math.cos(theta);z=center+.074*math.sin(theta)
        tube(v,'Lug_%s_%s_%s'%(side,axle,i),[(side*(track+.128),y,z),(side*(track+.141),y,z)],.009,'Steel',6)
    return tyre

def dent(obj,side,cy,cz,amount):
    bm=bmesh.new();bm.from_mesh(obj.data)
    bmesh.ops.triangulate(bm,faces=list(bm.faces))
    bmesh.ops.subdivide_edges(bm,edges=list(bm.edges),cuts=5,use_grid_fill=True)
    for p in bm.verts:
        if side*p.co.x>.88:
            f=math.exp(-((p.co.y-cy)/.24)**2-((p.co.z-cz)/.15)**2)
            p.co.x-=side*amount*f
    bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces));bm.to_mesh(obj.data);bm.free();obj.data.update()

def chips(v,side,broken=False,sedan=False):
    rng=random.Random(1003+(1 if broken else 0)+(9 if sedan else 0))
    for i in range(28 if not broken else 44):
        y=rng.uniform(-1.0,.7) if not sedan else rng.uniform(-.95,.8)
        z=rng.choice([.555,.57,1.265]) if not sedan else rng.choice([.38,.89])
        w=rng.uniform(.018,.07);h=rng.uniform(.004,.014)
        x=side*(.969 if not sedan else .849)
        outline=[(x,y-w/2,z),(x,y-w*.16,z+h),(x,y+w/2,z+h*.55),(x,y+w*.3,z-h*.3)]
        slab(v,'SeamChip_%s_%s'%(side,i),outline,(-side*.0007,0,0),'RustSeam' if i%3 else 'Primer',0)
    for i in range(7):
        y=-.64+i*.04
        z=.78+(.024 if i%2 else 0)
        x=side*(.97 if not sedan else .85)
        tube(v,'HandlingScratch_%s_%s'%(side,i),[(x,y,z),(x,y+.10,z+.002)],.0012,'Primer',4)

def danfo(identity,broken=False):
    v=entry(identity);bpy.context.window.scene=v['scene']
    with bpy.data.libraries.load(str(SOURCE),link=False) as (available,loaded):
        loaded.collections=['DanfoDetail_v1_Editable']
    imported=loaded.collections[0]
    roots=[o for o in imported.objects if o.type=='EMPTY']
    for obj in list(imported.objects):
        if obj.type!='MESH':continue
        copy=obj.copy();copy.data=obj.data.copy();v['collection'].objects.link(copy);copy.parent=v['root']
        copy.name=identity+'_'+obj.name;copy['part_role']=obj.name;copy['derivative_id']=identity;v['parts'].append(copy)
    # Only this newly appended source collection is removed; source file is never saved.
    for obj in list(imported.objects):bpy.data.objects.remove(obj,do_unlink=True)
    bpy.data.collections.remove(imported)
    colors={'DanfoYellow':'YellowFaded' if broken else 'YellowMaintained','YellowFaded':'YellowBleached' if broken else 'YellowFaded',
            'StripeBlack':'BlackStripe','GlassDark':'GlassAged' if broken else 'GlassClear','Steel':'Steel','Rubber':'Rubber',
            'LampIvory':'LampGlass','LampAmber':'Amber','LampRed':'RedLamp','IvoryPaint':'Ivory','Seat':'SeatCloth'}
    for obj in list(v['parts']):
        old=obj.name.split(identity+'_',1)[1]
        if any(word in old for word in ('ProfileTyre_','Rim_','HubCentre_','RimVent_','Danfo_Lug_')) or old=='Danfo_RearWindowSeal':
            remove(v,obj);continue
        base=obj.data.materials[0].name.split('.')[0] if obj.data.materials else 'Steel'
        assign(obj,colors.get(base,'Steel'))
        if 'MirrorFace' in old:assign(obj,'Mirror')
        if 'SidePanel_' in old:
            side=1 if old.endswith('_1') else -1;dent(obj,side,-.58,.80,.035 if broken else .019)
        if 'Window_' in old and not broken and old.endswith('_1_2'):
            # Half-open slider is real missing glass area, not black backing.
            for vert in obj.data.vertices:vert.co.y=-.185+(vert.co.y+.185)*.52
            uvmesh(obj.data,'GlassClear')
        for mod in obj.modifiers:
            if mod.type=='BEVEL':mod.width=min(mod.width,.006)
    for side in (-1,1):
        for axle in (-1.32,1.30):wheel(v,side,axle,center=.25 if broken and side==1 and axle>0 else .34,flat=broken and side==1 and axle>0)
        for label,c,s in [('RearGlassLower',(0,-2.175,1.442),(1.56,.026,.022)),('RearGlassUpper',(0,-2.175,1.938),(1.56,.026,.022))]:
            if side==1:box(v,label,c,s,'Rubber')
        if side==1:
            for x in (-.758,.758):box(v,'RearGlassJamb',(x,-2.175,1.69),(.022,.026,.50),'Rubber')
        chips(v,side,broken)
        # Welded repair follows a handled lower door panel rather than uniform dirt.
        patch=[(side*.968,.76,.62),(side*.968,1.18,.64),(side*.968,1.16,.91),(side*.968,.79,.92)]
        slab(v,'ReplacementDoorPatch_%s'%side,patch,(-side*.012,0,0),'YellowRepair' if not broken else 'ExposedOxidizedMetal')
        for y in (.80,1.13):
            for z in (.66,.88):tube(v,'PatchRivet_%s_%s_%s'%(side,y,z),[(side*.97,y,z),(side*.974,y,z)],.004,'Steel',6)
        tube(v,'LowerBodyRustSeam_%s'%side,[(side*.968,-.87,.56),(side*.968,.68,.56)],.0026,'RustSeam',5)
        for y in (-1.32,1.30):
            tube(v,'Damper_%s_%s'%(side,y),[(side*.73,y-.09,.39),(side*.67,y+.08,.63)],.025,'Steel')
    for i in range(6):
        box(v,'BumperAbrasion_%s'%i,(-.72+i*.235,2.283,.578),(.13,.0018,.008),'Primer',0)
    box(v,'BenchRecover',(0,-.52,1.015),(1.22,.27,.018),'SeatRepair',.004)
    box(v,'WoodBenchBacking',(0,-1.64,1.10),(1.20,.018,.24),'OldSeatBacking')
    tube(v,'GearLever',[(0,1.57,.69),(0,1.55,1.08)],.012,'Steel')
    box(v,'GearKnob',(0,1.55,1.10),(.052,.052,.055),'Rubber',.01)
    for x in (-.30,-.52):box(v,'Pedal',(x,1.80,.74),(.07,.055,.024),'Rubber')
    for x in (-.32,-.14):
        tube(v,'InstrumentRing',[(x,1.767,1.37),(x,1.758,1.37)],.055,'Steel',24)
        tube(v,'InstrumentFace',[(x,1.753,1.37),(x,1.749,1.37)],.048,'Ivory',24)
    tube(v,'Exhaust',[(.52,-.6,.44),(.52,-1.5,.37),(.66,-2.25,.34)],.027,'ExposedOxidizedMetal',12)
    if broken:
        for obj in list(v['parts']):
            if obj['part_role'] in ('Danfo_FrontPanel','Danfo_Grille') or obj.name.endswith('_Danfo_FrontPanel') or obj.name.endswith('_Danfo_Grille'):remove(v,obj)
        for x in (-.705,.705):box(v,'ServiceOpeningSide',(x,2.13,.94),(.45,.09,.69),'YellowFaded',.006)
        box(v,'ServiceOpeningTop',(0,2.13,1.252),(.96,.09,.065),'YellowBleached')
        box(v,'ServiceOpeningBottom',(0,2.13,.662),(.96,.09,.12),'YellowFaded')
        box(v,'ServiceBayBack',(0,1.94,1.02),(.95,.014,.45),'Battery')
        for x in (-.40,.40):box(v,'ServiceCavitySide',(x,2.04,1.02),(.018,.18,.43),'Steel')
        for i in range(18):box(v,'RadiatorFin',(-.21+i*.025,2.03,1.04),(.011,.037,.30),'RimOld',0)
        tube(v,'ServiceHose',[(-.39,2.04,.90),(-.26,2.10,.83),(.21,2.10,.85),(.35,2.03,1.13)],.019,'Hose',10)
        box(v,'BatteryCase',(.31,2.03,.88),(.17,.12,.16),'Battery',.008)
        for x in (.27,.34):tube(v,'BatteryTerminal',[(x,2.03,.97),(x,2.03,1.00)],.007,'Steel',8)
        outline=[(-.46,2.20,.77),(.46,2.20,.77),(.46,2.20,1.25),(-.46,2.20,1.25)]
        hatch=slab(v,'OpenServiceHatch',outline,(0,-.012,0),'YellowRepair')
        pivot=Vector((0,2.20,1.25));rot=Matrix.Rotation(math.radians(62),4,'X')
        for vert in hatch.data.vertices:vert.co=pivot+rot.to_3x3()@(vert.co-pivot)
        for x in (-.43,.43):
            tube(v,'HatchSupport',[(x,2.16,1.10),(x,2.55,1.02)],.007,'Steel')
            tube(v,'HatchHinge',[(x-.03,2.20,1.25),(x+.03,2.20,1.25)],.009,'Steel')
        box(v,'StandFoot',(.69,1.22,.015),(.32,.30,.03),'Steel')
        box(v,'StandSaddle',(.69,1.22,.395),(.16,.12,.01),'Steel')
        for dx,dy in [(-.13,-.12),(.13,-.12),(-.13,.12),(.13,.12)]:tube(v,'StandLeg',[(.69+dx,1.22+dy,.03),(.69,1.22,.38)],.014,'Steel')
        slab(v,'RearWheelChock',[(.84,-1.74,0),(.84,-1.52,0),(.84,-1.56,.14)],(.24,0,0),'OldSeatBacking')
        v['notes']+=['Static service hatch open with cavity/hoses/battery detail; appearance only','Right front flat tyre centre.25m, other tyres.34m; jack saddle contacts chassis underside.40m, ground-contact support and rear chock']
    else:v['notes'].append('Worn maintained-looking exterior and inflated tyres; no driving implementation')
    return v

def sedan():
    v=entry('SM_OlderSedan_Worn_v1');bpy.context.window.scene=v['scene']
    def width(y):return .85-.115*(abs(y)/2.25)**3
    def belt(y):return .88-.065*(max(0,abs(y)-1.1)/1.15)
    for side in (-1,1):
        verts=[];faces=[];count=101
        for i in range(count):
            y=-2.25+i*4.50/(count-1);bottom=.36
            for axle in (-1.35,1.35):
                d=abs(y-axle)
                if d<.341:bottom=max(bottom,.30+math.sqrt(.347**2-d*d))
            for j in range(6):
                t=j/5;z=bottom+(belt(y)-bottom)*t
                x=side*(width(y)-.026*t+.012*math.sin(t*math.pi))
                dentval=.015*math.exp(-((y+.48)/.22)**2-((z-.65)/.14)**2)
                x-=side*dentval
                verts.append((x,y,z))
                if i and j:faces.append(((i-1)*6+j-1,(i-1)*6+j,i*6+j,i*6+j-1))
        sideobj=mesh(v,'SculptedBodySide_%s'%side,verts,faces,'SedanBlue',smooth=True)
        mod=sideobj.modifiers.new('SteelSkin1_2mm','SOLIDIFY');mod.thickness=.0012
        for axle in (-1.35,1.35):
            arc=[(side*(width(axle)+.003),axle+.353*math.cos(math.pi*i/32),.30+.353*math.sin(math.pi*i/32)) for i in range(33)]
            tube(v,'FenderLip_%s_%s'%(side,axle),arc,.009,'SedanFaded')
            wheel(v,side,axle,.30,.30,.80)
        tube(v,'BeltTrim_%s'%side,[(side*.826,y,belt(y)+.013) for y in (-1.18,-.6,0,.62)],.006,'Steel')
        for y in (-.14,-1.16,.64):
            tube(v,'DoorGap_%s_%s'%(side,y),[(side*.846,y,.38),(side*.846,y,.85)],.0023,'BlackStripe',6)
        for y in (-.33,.37):
            box(v,'DoorHandle_%s_%s'%(side,y),(side*.852,y,.81),(.021,.13,.022),'RimOld')
        windows=[[(side*.806,.59,.938),(side*.648,.16,1.389),(side*.652,-.085,1.399),(side*.812,-.085,.938)],
                 [(side*.812,-.16,.938),(side*.652,-.16,1.399),(side*.650,-.72,1.375),(side*.794,-1.12,.946)]]
        for idx,outline in enumerate(windows):
            slab(v,'SideGlass_%s_%s'%(side,idx),outline,(-side*.005,0,0),'GlassAged',0)
            tube(v,'WindowSeal_%s_%s'%(side,idx),outline+[outline[0]],.010,'Rubber',8)
        for name,p,q in [('APillar',(side*.808,.65,.90),(side*.643,.19,1.42)),
                         ('BPillar',(side*.819,-.12,.89),(side*.654,-.12,1.43)),
                         ('CPillar',(side*.794,-1.19,.91),(side*.640,-.76,1.40))]:
            tube(v,name+str(side),[p,q],.024 if name!='CPillar' else .043,'SedanFaded',12)
        tube(v,'Sill_%s'%side,[(side*.835,-1.05,.365),(side*.835,1.00,.365)],.022,'SedanPanel',10)
        chips(v,side,sedan=True)
        mirror=box(v,'MirrorShell_%s'%side,(side*.932,.53,1.00),(.08,.18,.11),'SedanPanel',.018)
        tube(v,'MirrorStem_%s'%side,[(side*.81,.54,.965),(side*.92,.54,.99)],.011,'Steel')
        box(v,'MirrorFace_%s'%side,(side*.932,.435,1.0),(.061,.009,.085),'Mirror',.007)
    def deck(name,y0,y1,mat):
        verts=[];faces=[];n=24;m=12
        for j in range(n+1):
            y=y0+(y1-y0)*j/n
            for i in range(m+1):
                t=-1+2*i/m;x=width(y)*.967*t;z=belt(y)+.020+.032*(1-t*t)
                verts.append((x,y,z))
                if i and j:faces.append(((j-1)*(m+1)+i-1,(j-1)*(m+1)+i,j*(m+1)+i,j*(m+1)+i-1))
        obj=mesh(v,name,verts,faces,mat,smooth=True);mod=obj.modifiers.new('PanelGauge','SOLIDIFY');mod.thickness=.0012
    deck('Bonnet',.67,2.25,'SedanFaded');deck('Boot',-2.25,-1.17,'SedanPanel')
    roof=[];faces=[]
    for j in range(15):
        y=-.78+j*1.00/14
        for i in range(17):
            t=-1+2*i/16;roof.append((t*.65,y,1.418+.032*(1-t*t)))
            if i and j:faces.append(((j-1)*17+i-1,(j-1)*17+i,j*17+i,j*17+i-1))
    obj=mesh(v,'CurvedRoof',roof,faces,'SedanFaded',smooth=True);mod=obj.modifiers.new('RoofGauge','SOLIDIFY');mod.thickness=.012
    front=[(-.778,.674,.918),(.778,.674,.918),(.623,.226,1.405),(-.623,.226,1.405)]
    rear=[(-.767,-1.169,.936),(-.628,-.785,1.402),(.628,-.785,1.402),(.767,-1.169,.936)]
    for name,outline in [('Windscreen',front),('RearScreen',rear)]:
        slab(v,name,outline,(0,-.004,0),'GlassClear',0);tube(v,name+'Seal',outline+[outline[0]],.012,'Rubber')
    for x in (-.38,.30):tube(v,'Wiper',[(x,.692,.94),(x+.18,.539,1.09)],.005,'Rubber',8)
    for end in (-1,1):
        y=end*2.235
        box(v,'EndValance_%s'%end,(0,y,.62),(1.46,.035,.37),'SedanPanel',.035)
        box(v,'Bumper_%s'%end,(0,end*2.29,.475),(1.62,.095,.10),'RimOld',.014)
        box(v,'BumperRubber_%s'%end,(0,end*2.343,.477),(1.48,.014,.037),'Rubber',.005)
        box(v,'BlankPlate_%s'%end,(0,end*2.345,.62),(.34,.009,.10),'Ivory')
        for x in (-.57,.57):
            box(v,'LampHousing_%s_%s'%(end,x),(x,end*2.266,.766),(.30,.037,.115),'Steel',.012)
            box(v,'LampLens_%s_%s'%(end,x),(x,end*2.291,.766),(.281,.009,.097),'LampGlass' if end==1 else 'RedLamp',.009)
            if end==1:
                for i in range(12):box(v,'LensFlute',(x-.125+i*.022,2.298,.766),(.002,.001,.085),'LampGlass',0)
        if end==1:
            for i in range(7):box(v,'GrilleSlat',(0,2.262,.737+i*.018),(.49,.018,.007),'BlackStripe',0)
    box(v,'CabinFloor',(0,-.22,.46),(1.42,2.65,.018),'BlackStripe')
    box(v,'Dash',(0,.55,.875),(1.42,.25,.14),'BlackStripe',.028)
    for x in (-.39,.39):
        box(v,'FrontSeatBase',(x,.00,.61),(.52,.48,.12),'SeatCloth',.035)
        box(v,'FrontSeatBack',(x,-.235,.825),(.52,.09,.48),'SeatCloth',.026)
        box(v,'Headrest',(x,-.25,1.095),(.22,.065,.14),'SeatRepair',.027)
    box(v,'RearBench',(0,-.80,.625),(1.28,.42,.14),'SeatCloth',.028)
    box(v,'RearBack',(0,-1.02,.87),(1.28,.07,.38),'SeatCloth',.024)
    center=Vector((-.39,.33,.92))
    tube(v,'SteeringColumn',[(-.39,.58,.81),tuple(center)],.018,'Steel')
    u=Vector((1,0,0));w=Vector((0,.71,.71))
    ring=[tuple(center+.15*(math.cos(i*math.tau/32)*u+math.sin(i*math.tau/32)*w)) for i in range(33)]
    tube(v,'SteeringWheel',ring,.010,'Rubber',8)
    for a in (0,math.pi):tube(v,'SteeringSpoke',[tuple(center),tuple(center+.13*math.cos(a)*u)],.009,'Steel')
    for x in (-.62,.62):box(v,'ChassisRail',(x,0,.385),(.052,3.6,.05),'Steel')
    for y in (-1.35,1.35):tube(v,'Axle',[(-.80,y,.30),(.80,y,.30)],.026,'Steel',12)
    tube(v,'Exhaust',[(.41,.3,.32),(.41,-1.75,.27),(.55,-2.35,.27)],.023,'ExposedOxidizedMetal',12)
    # One repainted repair, not a uniformly rust-covered fleet.
    slab(v,'DoorRepair',[(.853,-.83,.43),(.853,-.38,.43),(.853,-.40,.70),(.853,-.81,.71)],(-.004,0,0),'SedanPanel')
    v['notes']=['Generic older three-box passenger sedan, original authored silhouette; no brand/model reconstruction',
                 'Wheelbase2.7m, body4.5m, tyre.30m, cabin height1.45m; worn inflated tyres, local repaint/chips, clear glass and interior']
    return v

def setup_stage(v):
    bpy.context.window.scene=v['scene'];scene=v['scene']
    stage=bpy.data.collections.new(v['id']+'_PreviewOnly');scene.collection.children.link(stage)
    pseudo={'id':v['id']+'_Stage','collection':stage,'root':None,'parts':[]}
    box(pseudo,'Road',(0,0,-.04),(24,24,.08),'Asphalt',0)
    world=bpy.data.worlds.new(v['id']+'_Daylight');world.use_nodes=True
    world.node_tree.nodes.get('Background').inputs['Color'].default_value=(.62,.72,.83,1)
    world.node_tree.nodes.get('Background').inputs['Strength'].default_value=.65;scene.world=world
    sun=bpy.data.lights.new(v['id']+'_Sun','SUN');sun.energy=2.1;sun.angle=math.radians(5)
    obj=bpy.data.objects.new(sun.name,sun);stage.objects.link(obj);obj.rotation_euler=(math.radians(25),math.radians(-25),math.radians(-28))
    scene.render.engine='CYCLES';scene.cycles.device='CPU';scene.cycles.samples=16
    scene.cycles.use_denoising=True;scene.cycles.max_bounces=7;scene.cycles.transmission_bounces=6
    scene.render.threads_mode='FIXED';scene.render.threads=2
    scene.render.resolution_x=960;scene.render.resolution_y=640;scene.render.resolution_percentage=100
    scene.render.image_settings.file_format='PNG';scene.render.image_settings.color_mode='RGB';scene.render.image_settings.color_depth='8'
    scene.view_settings.view_transform='AgX';scene.view_settings.look='None';scene.view_settings.exposure=0
    return stage

def camera(v,name,eye,target,lens):
    data=bpy.data.cameras.new(name);obj=bpy.data.objects.new(name,data);v['scene'].collection.objects.link(obj)
    obj.location=eye;obj.rotation_euler=(Vector(target)-obj.location).to_track_quat('-Z','Y').to_euler();data.lens=lens
    return obj

def bounds(v):
    bpy.context.window.scene=v['scene'];bpy.context.view_layer.update();graph=bpy.context.evaluated_depsgraph_get()
    lo=[float('inf')]*3;hi=[float('-inf')]*3
    for obj in v['parts']:
        ev=obj.evaluated_get(graph)
        for corner in ev.bound_box:
            p=ev.matrix_world@Vector(corner)
            for a in range(3):lo[a]=min(lo[a],p[a]);hi[a]=max(hi[a],p[a])
    return {'min_m':lo,'max_m':hi,'dimensions_m':[hi[a]-lo[a] for a in range(3)]}

def export_vehicle(v,folder):
    folder.mkdir(exist_ok=True);bpy.context.window.scene=v['scene'];bpy.context.view_layer.update()
    for obj in bpy.context.view_layer.objects:obj.select_set(False)
    graph=bpy.context.evaluated_depsgraph_get();copies=[]
    for part in v['parts']:
        data=bpy.data.meshes.new_from_object(part.evaluated_get(graph),preserve_all_data_layers=True,depsgraph=graph)
        obj=bpy.data.objects.new('EXP_'+part.name,data);v['scene'].collection.objects.link(obj)
        obj.matrix_world=v['root'].matrix_world.inverted()@part.matrix_world;obj.select_set(True);copies.append(obj)
    bpy.context.view_layer.objects.active=copies[0];bpy.ops.object.join()
    joined=bpy.context.view_layer.objects.active;joined.name=v['id'];joined.data.calc_loop_triangles()
    path=folder/(v['id']+'.fbx')
    bpy.ops.export_scene.fbx(filepath=str(path),use_selection=True,object_types={'MESH'},global_scale=1,
        apply_unit_scale=True,apply_scale_options='FBX_SCALE_UNITS',axis_forward='-Y',axis_up='Z',bake_anim=False,
        use_mesh_modifiers=False,mesh_smooth_type='FACE',use_triangles=True,path_mode='RELATIVE',use_custom_props=False)
    report={'id':v['id'],'file':str(path.relative_to(OUT)).replace('\\','/'),'sha256':digest(path),
        'vertices':len(joined.data.vertices),'triangles':len(joined.data.loop_triangles),
        'material_slots':[m.name for m in joined.data.materials],'uv_layers':[u.name for u in joined.data.uv_layers],
        'bounds':bounds(v),'parts':[{'name':p.name,'role':p.get('part_role'),
            'matrix_root_local_rows':[list(row) for row in v['root'].matrix_world.inverted()@p.matrix_world]} for p in v['parts']],
        'notes':v['notes']}
    data=joined.data;bpy.data.objects.remove(joined,do_unlink=True);bpy.data.meshes.remove(data)
    return report

def save_delivery(folder):
    folder.mkdir(exist_ok=True)
    bpy.context.window.scene=VEHICLES[0]['scene']
    path=folder/'LagosVehicleWearLibrary.blend'
    bpy.ops.wm.save_as_mainfile(filepath=str(path))
    reports=[export_vehicle(v,folder) for v in VEHICLES]
    bpy.ops.wm.open_mainfile(filepath=str(path))
    for key in list(MATS):MATS[key]=bpy.data.materials['VW_'+key]
    probes={}
    for v in VEHICLES:
        v['scene']=bpy.data.scenes[v['id']+'_Authoring'];v['collection']=bpy.data.collections[v['id']+'_Editable']
        v['root']=bpy.data.objects[v['id']+'_ROOT'];v['parts']=[o for o in v['collection'].objects if o.type=='MESH']
        bpy.context.window.scene=v['scene'];bpy.context.view_layer.update()
        probes[v['id']]=runpy.run_path(str(PROBE))['inspect_scene'](limit=12)
        if probes[v['id']]['assets']['missing_count']:raise RuntimeError('Missing dependencies after reopen')
    write(folder/'scene-probe-reopened.json',probes)
    write(folder/'vehicle-manifest.json',{'job':JOB,'pid':os.getpid(),'blend':{'path':str(path),'sha256':digest(path)},
        'assets':reports,'material_bindings':'../material-bindings.json','units':'metres','up':'+Z','vehicle_front':'+Y',
        'origin':'Ground datum below centre between axles; root identity; breakdown support is static',
        'fbx':{'axis_forward':'-Y','axis_up':'Z','global_scale':1,'apply_unit_scale':True,'apply_scale_options':'FBX_SCALE_UNITS'},
        'native_importer_basis':'Not verified by Blender writer on Unreal; import one module and check cm bounds,front/up and origin before placing. No canonical transform assumed',
        'uv':'SurfaceUV predivided per material coverage; native multiplier1x',
        'source':{'blend':str(SOURCE),'sha256':SOURCE_HASH},'script_sha256':digest(Path(__file__)),
        'appearance_only':'No driving, damage simulation, breakdown logic, repair gameplay, rig/collision/LOD or runtime registration',
        'visual_acceptance':'Pending actual PNG review','errors':[]})
    return path,reports

def render(v,name,folder):
    bpy.context.window.scene=v['scene'];scene=v['scene'];scene.camera=bpy.data.objects[name]
    partial=folder/(name+'.partial.png');final=folder/(name+'.png');scene.render.filepath=str(partial)
    progress('rendering',camera=name,vehicle=v['id'],frames=FRAMES)
    start=time.monotonic();bpy.ops.render.render(write_still=True,scene=scene.name)
    image=bpy.data.images.load(str(partial),check_existing=False)
    if list(image.size)!=[960,640] or not image.has_data:raise RuntimeError('PNG decode/dimension failure')
    bpy.data.images.remove(image);partial.replace(final)
    FRAMES.append({'path':str(final),'sha256':digest(final),'camera':name,'vehicle':v['id'],'seconds':time.monotonic()-start})
    write(OUT/'frames.json',FRAMES)

def main():
    parser=argparse.ArgumentParser();parser.add_argument('--owned-background-job',required=True)
    args=parser.parse_args(sys.argv[sys.argv.index('--')+1:])
    if not bpy.app.background or bpy.data.filepath or os.getpid()==3224 or args.owned_background_job!=JOB or os.environ.get('CRIBTEST_BLENDER_OWNER_JOB')!=JOB:
        raise RuntimeError('Fresh owned background handshake required')
    if digest(SOURCE)!=SOURCE_HASH:raise RuntimeError('Frozen Danfo source mismatch')
    if (OUT/'initial').exists() or (OUT/'delivery').exists():raise RuntimeError('Fresh vehicle revision required')
    (OUT/'textures').mkdir();(OUT/'initial').mkdir();(OUT/'frames-initial').mkdir();(OUT/'frames-delivery').mkdir()
    progress('authoring_vehicle_variants',source=str(SOURCE),source_sha256=SOURCE_HASH,version=bpy.app.version_string)
    palette()
    VEHICLES.extend([danfo('SM_Danfo_Worn_v1'),danfo('SM_Danfo_Breakdown_v1',True),sedan()])
    for v in VEHICLES:setup_stage(v)
    camera(VEHICLES[0],'01-Worn-Danfo-Quarter',(5.9,7.6,2.65),(0,0,1.03),44)
    camera(VEHICLES[1],'02-Breakdown-Danfo-Quarter',(5.9,7.6,2.65),(0,.1,1.03),44)
    camera(VEHICLES[2],'03-Older-Sedan-Quarter',(5.2,6.7,2.2),(0,0,.73),48)
    camera(VEHICLES[0],'04-Danfo-Wheel-Cabin',(4.4,3.4,1.57),(.18,.70,.99),38)
    library=bpy.data.scenes.new('VehicleLibraryOverview');library.unit_settings.system='METRIC';library.unit_settings.scale_length=1
    for i,v in enumerate(VEHICLES):
        obj=bpy.data.objects.new(v['id']+'_LibraryDisplay',None);obj.instance_type='COLLECTION';obj.instance_collection=v['collection']
        library.collection.objects.link(obj);obj.location=((i-1)*6,0,0)
    for i,v in enumerate(VEHICLES):v['scene'].camera=bpy.data.objects['0%s-%s'%(i+1,['Worn-Danfo-Quarter','Breakdown-Danfo-Quarter','Older-Sedan-Quarter'][i])]
    write(OUT/'material-bindings.json',MAT_REPORT)
    path,reports=save_delivery(OUT/'initial')
    progress('saved_exported_reopened_initial',blend=str(path),sha256=digest(path),assets=reports)
    for v,name in [(VEHICLES[0],'01-Worn-Danfo-Quarter'),(VEHICLES[1],'02-Breakdown-Danfo-Quarter'),
                   (VEHICLES[2],'03-Older-Sedan-Quarter'),(VEHICLES[0],'04-Danfo-Wheel-Cabin')]:render(v,name,OUT/'frames-initial')
    progress('awaiting_worker_actual_image_review',frames=FRAMES,deadline_seconds=900)
    deadline=time.monotonic()+900
    request=OUT/'review-decision.json'
    while not request.exists():
        if time.monotonic()>deadline:raise RuntimeError('Image review gate expired; initial saved outputs retained')
        if (OUT/'STOP').exists():raise RuntimeError('Owned stop marker at review gate')
        time.sleep(.5)
    decision=json.loads(request.read_text(encoding='utf-8'))
    if 'expected_pid' in decision and decision['expected_pid']!=os.getpid():raise RuntimeError('Review PID mismatch')
    if decision['action']=='repair':
        script=Path(decision['script']).resolve()
        if not script.is_relative_to(OUT.resolve()):raise RuntimeError('Repair source outside owned revision')
        ns=runpy.run_path(str(script));ns['repair'](VEHICLES,MATS,mesh,box,slab,tube,assign)
        progress('causal_repair_applied',repair_script=str(script),sha256=digest(script),defect=decision['defect'])
    elif decision['action']!='accept_batch':raise RuntimeError('Unsupported review decision')
    final_path,reports=save_delivery(OUT/'delivery')
    for name in decision.get('rerender',[]):
        v=next(v for v in VEHICLES if name in v['scene'].objects);render(v,name,OUT/'frames-delivery')
    progress('vehicle_library_delivered_pending_final_image_review',blend=str(final_path),sha256=digest(final_path),frames=FRAMES,
             manifest=str(OUT/'delivery/vehicle-manifest.json'),errors=[])
    if digest(SOURCE)!=SOURCE_HASH:raise RuntimeError('Immutable source changed')

if __name__=='__main__':
    try:main()
    except Exception:
        progress('failed',errors=[traceback.format_exc()]);raise

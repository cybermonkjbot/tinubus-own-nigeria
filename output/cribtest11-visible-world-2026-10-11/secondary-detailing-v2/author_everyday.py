"""Original metric prop library. Run in the idle identified Blender workbench.

Exports only owned assets, never modifies the primary world or v1 assets.
No rendering, tests, app compilation or Unreal editor mutations.
"""
import contextlib
import hashlib
import io
import json
import math
import os
from pathlib import Path

import bpy
import numpy as np
from mathutils import Vector

ROOT=Path(__file__).resolve().parent
OWNER='LagosEverydayLifeV2'
MATERIAL='M_LagosEveryday_Opaque'


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def image_png(name,data,srgb=False):
    image=bpy.data.images.new(OWNER+'_'+name,width=data.shape[1],height=data.shape[0],alpha=True)
    image.colorspace_settings.name='Non-Color'
    image.pixels.foreach_set(np.flipud(data).astype(np.float32).ravel())
    image.file_format='PNG';image.filepath_raw=str(ROOT/'textures'/f'{name}.png');image.save()
    image.colorspace_settings.name='sRGB' if srgb else 'Non-Color'
    image.pack()
    return image


def make_atlas():
    n=2048
    yy,xx=np.mgrid[0:n,0:n]
    rng=np.random.default_rng(20261011)
    noise=rng.random((n,n),dtype=np.float32)
    color=np.ones((n,n,4),dtype=np.float32)
    orm=np.ones_like(color);orm[:,:,2]=0
    palettes=[(.35,.30,.24),(.045,.043,.039),(.10,.30,.38),(.19,.40,.24),
              (.73,.72,.65),(.60,.45,.12),(.21,.23,.24),(.33,.20,.095),
              (.11,.20,.34),(.065,.052,.039),(.69,.65,.53),(.78,.17,.07),
              (.53,.32,.16),(.76,.70,.52),(.22,.31,.24),(.11,.11,.10)]
    rough=[.82,.91,.53,.56,.62,.71,.38,.86,.52,.75,.94,.48,.86,.88,.81,.94]
    for tile,base in enumerate(palettes):
        row,col=divmod(tile,4);sl=np.s_[row*512:(row+1)*512,col*512:(col+1)*512]
        y,x=np.mgrid[0:512,0:512]
        edge=np.minimum.reduce([x,y,511-x,511-y])/512
        # Restrained handling scuffs; no baked lighting/AO.
        variation=(noise[sl]-.5)*.035
        color[sl][:,:,:3]=np.clip(np.array(base)+variation[:,:,None],0,1)
        orm[sl][:,:,1]=np.clip(rough[tile]+(noise[sl]-.5)*.05,0,1)
        if tile in (5,14):
            # Handling scratches change surface pigment/roughness; no dark
            # tile-edge vignette that could be mistaken for baked AO.
            scuff=(noise[sl]>.91)&(edge<.12)
            color[sl][:,:,:3]+=scuff[:,:,None]*.045
            orm[sl][:,:,1]=np.clip(orm[sl][:,:,1]+scuff*.12,0,1)
        if tile==6:orm[sl][:,:,2]=1
        if tile==10:
            weave=(np.sin(x*math.pi/3)*np.sin(y*math.pi/3)+1)*.015
            color[sl][:,:,:3]-=weave[:,:,None]
    source_root=ROOT.parent/'material-inputs-verified'
    source=json.loads((source_root/'material-manifest.json').read_text())
    wood=next(m for m in source['materials'] if m['id']=='wood_planks_grey')
    provenance=[]
    for role,target in [('Diffuse',color),('Rough',orm)]:
        record=next(f for f in wood['files'] if f['role']==role)
        path=source_root/record['path']
        if sha(path)!=record['sha256']:raise RuntimeError('Wood source hash mismatch')
        image=bpy.data.images.load(str(path),check_existing=False)
        if role=='Rough':image.colorspace_settings.name='Non-Color'
        data=np.empty(len(image.pixels),dtype=np.float32);image.pixels.foreach_get(data)
        data=np.flipud(data.reshape(image.size[1],image.size[0],4))
        yi=np.linspace(0,image.size[1]-1,512).astype(int);xi=np.linspace(0,image.size[0]-1,512).astype(int)
        sample=data[yi[:,None],xi[None,:],:3]
        if role=='Diffuse':
            sample=np.where(sample<=.0031308,sample*12.92,1.055*np.maximum(sample,0)**(1/2.4)-.055)
            target[:512,:512,:3]=np.clip(sample,0,1)
        else:target[:512,:512,1]=sample[:,:,0]
        bpy.data.images.remove(image)
        provenance.append({'role':role,'file':record['path'],'sha256':record['sha256']})
    base=image_png('T_DetailAtlas_BC',color,True);packed=image_png('T_DetailAtlas_ORM',orm)
    material=bpy.data.materials.new(MATERIAL);material.use_nodes=True
    nodes=material.node_tree.nodes;links=material.node_tree.links
    bsdf=nodes.get('Principled BSDF')
    tex=nodes.new('ShaderNodeTexImage');tex.image=base
    rough_node=nodes.new('ShaderNodeTexImage');rough_node.image=packed
    split=nodes.new('ShaderNodeSeparateColor')
    links.new(tex.outputs['Color'],bsdf.inputs['Base Color']);links.new(rough_node.outputs['Color'],split.inputs['Color'])
    links.new(split.outputs['Green'],bsdf.inputs['Roughness']);links.new(split.outputs['Blue'],bsdf.inputs['Metallic'])
    return material,[base,packed],{'id':wood['id'],'license':source['license'],'authors':wood['authors'],'maps':provenance,'source_manifest_sha256':sha(source_root/'material-manifest.json')}


class Assembly:
    def __init__(self):self.vertices=[];self.faces=[];self.tiles=[]

    def add(self,vertices,faces,tile):
        first=len(self.vertices);self.vertices.extend(vertices)
        self.faces.extend([tuple(first+i for i in face) for face in faces]);self.tiles.extend([tile]*len(faces))

    def box(self,center,size,tile):
        x,y,z=center;dx,dy,dz=size
        v=[(x+sx*dx/2,y+sy*dy/2,z+sz*dz/2) for sz in [-1,1] for sy in [-1,1] for sx in [-1,1]]
        self.add(v,[(0,2,3,1),(4,5,7,6),(0,1,5,4),(2,6,7,3),(0,4,6,2),(1,3,7,5)],tile)

    def chamfer_box(self,center,size,tile,radius=.012):
        x,y,z=center;dx,dy,dz=size
        r=min(radius,dx*.2,dy*.2)
        outline=[(-dx/2+r,-dy/2),(dx/2-r,-dy/2),(dx/2,-dy/2+r),(dx/2,dy/2-r),
                 (dx/2-r,dy/2),(-dx/2+r,dy/2),(-dx/2,dy/2-r),(-dx/2,-dy/2+r)]
        vertices=[(x+a,y+b,z+height) for height in [-dz/2,dz/2] for a,b in outline]
        faces=[tuple(reversed(range(8))),tuple(8+i for i in range(8))]
        faces.extend([(i,(i+1)%8,8+(i+1)%8,8+i) for i in range(8)])
        self.add(vertices,faces,tile)

    def tapered_tray(self,width,length,height,tile):
        # Continuous thin-walled tapered bowl with an open mouth and bottom.
        bottom=(width*.73,length*.67);top=(width,length)
        outer=[(-1,-1),(1,-1),(1,1),(-1,1)]
        vertices=[]
        for w,l,z in [(bottom[0],bottom[1],0),(top[0],top[1],height),
                      (top[0]-.025,top[1]-.025,height),(bottom[0]-.025,bottom[1]-.025,.015)]:
            vertices.extend([(sx*w/2,sy*l/2,z) for sx,sy in outer])
        faces=[(3,2,1,0),(12,13,14,15)]
        for j in range(3):
            for i in range(4):faces.append((j*4+i,j*4+(i+1)%4,(j+1)*4+(i+1)%4,(j+1)*4+i))
        self.add(vertices,faces,tile)

    def lathe(self,center,profile,segments,tile,axis='Z',caps=True):
        # Profile=(radius, axial offset). Axial conventions let cylinders/tanks
        # retain real ring geometry instead of inflated subdivided boxes.
        v=[];faces=[];x,y,z=center
        for radius,height in profile:
            for i in range(segments):
                a=i*2*math.pi/segments;u=radius*math.cos(a);w=radius*math.sin(a)
                v.append((x+u,y+w,z+height) if axis=='Z' else ((x+height,y+u,z+w) if axis=='X' else (x+u,y+height,z+w)))
        for j in range(len(profile)-1):
            for i in range(segments):faces.append((j*segments+i,j*segments+(i+1)%segments,(j+1)*segments+(i+1)%segments,(j+1)*segments+i))
        if caps:faces.extend([tuple(reversed(range(segments))),tuple((len(profile)-1)*segments+i for i in range(segments))])
        self.add(v,faces,tile)

    def rod(self,start,end,radius,segments,tile):
        a,b=Vector(start),Vector(end);direction=(b-a).normalized()
        tangent=direction.cross(Vector((0,0,1)))
        if tangent.length<.001:tangent=direction.cross(Vector((0,1,0)))
        tangent.normalize();second=direction.cross(tangent).normalized()
        v=[]
        for point in [a,b]:
            for i in range(segments):
                angle=i*2*math.pi/segments
                v.append(tuple(point+radius*(math.cos(angle)*tangent+math.sin(angle)*second)))
        f=[(i,(i+1)%segments,segments+(i+1)%segments,segments+i) for i in range(segments)]
        f.extend([tuple(reversed(range(segments))),tuple(segments+i for i in range(segments))]);self.add(v,f,tile)

    def ring(self,center,radius,tube,segments,tile,plane='XY',arc=2*math.pi):
        points=[]
        for i in range(segments+1):
            angle=i*arc/segments
            u,w=radius*math.cos(angle),radius*math.sin(angle)
            c=center
            points.append((c[0]+u,c[1]+w,c[2]) if plane=='XY' else ((c[0]+u,c[1],c[2]+w) if plane=='XZ' else (c[0],c[1]+u,c[2]+w)))
        for a,b in zip(points,points[1:]):self.rod(a,b,tube,4,tile)

    def object(self,collection,name,material):
        mesh=bpy.data.meshes.new(name+'_Mesh');mesh.from_pydata(self.vertices,[],self.faces);mesh.update()
        obj=bpy.data.objects.new(name,mesh);collection.objects.link(obj);mesh.materials.append(material)
        uv=mesh.uv_layers.new(name='UV0')
        for poly,tile in zip(mesh.polygons,self.tiles):
            row,col=divmod(tile,4)
            axis=max(range(3),key=lambda k:abs(poly.normal[k]));axes=[k for k in range(3) if k!=axis]
            coords=[mesh.vertices[i].co for i in poly.vertices]
            lo=[min(c[k] for c in coords) for k in axes];hi=[max(c[k] for c in coords) for k in axes]
            for li,co in zip(poly.loop_indices,coords):
                # Timber has physical 1.5m coverage; other parts map into inset
                # atlas tiles. No accidental cross-tile sampling.
                uv_pair=[(co[k]-lo[j])/max(hi[j]-lo[j],1e-5) for j,k in enumerate(axes)]
                if tile==0:uv_pair=[(co[k]-lo[j])/1.5 for j,k in enumerate(axes)]
                u,v=[min(1,max(0,p)) for p in uv_pair]
                uv.data[li].uv=((col+(20+u*472)/512)/4,1-(row+(20+v*472)/512)/4)
        obj['detail_owner']=OWNER;obj['asset_id']=name.split('_LOD')[0]
        return obj


def build(kind,lod,worn=False):
    a=Assembly();s=[24,12,8][lod];tube=[8,6,4][lod];small=lod==0
    plastic=14 if worn else 3
    if kind in ('chair','stool'):
        h=.43;w=.42 if kind=='chair' else .32;d=.39 if kind=='chair' else .30
        a.box((0,0,h),(w,d,.038),plastic)
        for x in [-w*.40,w*.40]:
            for y in [-d*.40,d*.40]:a.rod((x*1.2,y*1.2,.015),(x,y,h),.022,tube,plastic)
        if kind=='chair':
            for x in [-.19,.19]:a.rod((x,.16,h),(x,.19,.84),.020,tube,plastic)
            a.box((0,.185,.82),(.41,.035,.06),plastic)
            for x in [-.135,-.045,.045,.135]:a.box((x,.185,.665),(.037,.027,.25),plastic)
            if small:
                for x in [-.19,.19]:a.box((x,0,.61),(.030,.37,.026),plastic)
    elif kind=='crate':
        crate_tile=14 if worn else 2
        a.box((0,0,.012),(.40,.30,.024),crate_tile)
        for z in ([.065,.16,.255] if lod<2 else [.06,.255]):
            for y in [-.145,.145]:a.box((0,y,z),(.40,.018,.025),crate_tile)
            for x in [-.19,.19]:a.box((x,0,z),(.018,.30,.025),crate_tile)
        for x in [-.19,0,.19]:
            for y in [-.145,.145]:a.box((x,y,.14),(.020,.018,.28),crate_tile)
        for x in [-.19,.19]:
            for y in [0]:a.box((x,y,.14),(.018,.022,.28),crate_tile)
        if small:
            for x in [-.10,.10]:a.box((x,0,.018),(.012,.28,.012),crate_tile)
    elif kind=='cooler':
        a.chamfer_box((0,0,.24),(.58,.37,.46),2,.022);a.chamfer_box((0,0,.485),(.61,.40,.055),4,.020)
        for x in [-.305,.305]:a.box((x,0,.36),(.030,.18,.045),4)
        if small:a.lathe((0,-.19,.10),[(.015,0),(.015,.015)],8,4,'Y')
    elif kind in ('tray','basket'):
        if kind=='tray':
            a.box((0,0,.012),(.42,.30,.024),0)
            for y in [-.15,.15]:a.box((0,y,.065),(.44,.018,.12),0)
            for x in [-.21,.21]:a.box((x,0,.065),(.018,.30,.12),0)
        else:
            a.lathe((0,0,0),[(.11,0),(.15,.015),(.20,.20),(.195,.215),(.18,.20),(.13,.025)],s,10,caps=False)
            a.ring((0,0,.21),.19,.009,s,10)
    elif kind in ('tomatoes','onions'):
        for index,(x,y) in enumerate([(-.12,-.065),(0,-.065),(.12,-.065),(-.06,.045),(.06,.045)]):
            radius=.047 if kind=='tomatoes' else .041
            profile=[(.018,.005),(radius,.027),(radius*.88,.057),(.014,.075)]
            a.lathe((x,y,0),profile,[12,8,6][lod],11 if kind=='tomatoes' else 12)
            if small and kind=='tomatoes':a.box((x,y,.077),(.024,.006,.003),3)
    elif kind=='scale':
        a.box((0,0,.075),(.22,.25,.15),4);a.box((0,0,.168),(.27,.29,.018),6)
        a.box((0,-.128,.085),(.135,.008,.070),9)
        if small:
            for x in [-.045,-.015,.015,.045]:a.box((x,-.134,.09),(.018,.002,.030),4)
    elif kind=='table':
        a.box((0,0,.77),(1.10,.60,.042),0)
        for x in [-.47,.47]:
            for y in [-.24,.24]:a.box((x,y,.38),(.055,.055,.76),0)
        for y in [-.24,.24]:a.box((0,y,.19),(.94,.035,.055),0)
    elif kind=='jerrycan':
        a.chamfer_box((0,0,.225),(.28,.18,.43),5,.018)
        a.chamfer_box((0,0,.455),(.26,.17,.045),5,.018)
        a.box((0,.005,.53),(.14,.035,.027),5)
        for x in [-.065,.065]:a.box((x,.005,.49),(.025,.035,.075),5)
        a.lathe((.085,-.045,.46),[(.024,0),(.024,.024)],tube,9)
        if small:
            for y in [-.095,.095]:
                a.rod((-.10,y,.12),(.10,y,.35),.006,4,5);a.rod((-.10,y,.35),(.10,y,.12),.006,4,5)
    elif kind in ('drum','tank'):
        r,h=(.30,.88) if kind=='drum' else (.59,1.20)
        profile=[(r*.91,0),(r,.04)]
        for j in range(1,6):
            z=h*j/6
            profile.extend([(r,z-.02),(r*1.025,z),(r,z+.02)])
        profile.extend([(r,h-.07),(r*.91,h)])
        if lod==2:profile=[(r*.91,0),(r,.05),(r,h-.07),(r*.91,h)]
        a.lathe((0,0,0),profile,s,8 if kind=='drum' else 9)
        a.lathe((0,0,h),[(r*.82,0),(r*.82,.022)],s,9)
        if small:
            a.lathe((r*.15,-r,.14),[(.018,0),(.018,.035)],8,6,'Y')
    elif kind in ('bucket','basin'):
        radius,height=(.15,.27) if kind=='bucket' else (.24,.105)
        a.lathe((0,0,0),[(radius*.74,0),(radius,height),(radius*.97,height+.008),(radius*.94,height-.008),(radius*.73,.012)],s,2,caps=True)
        if lod==0:a.ring((0,0,height),radius,.005,s,2)
        if kind=='bucket':a.ring((0,0,height-.015),radius*.95,.004,[16,10,6][lod],6,plane='XZ',arc=math.pi)
    elif kind=='ac':
        a.box((0,0,.19),(.56,.43,.38),4)
        for z in np.linspace(.055,.32,[12,7,4][lod]):a.box((0,-.22,float(z)),(.49,.018,.012),4)
        a.box((.185,-.235,.09),(.12,.008,.08),9)
        if small:
            for x in np.linspace(-.20,.20,9):a.box((float(x),.221,.20),(.008,.016,.29),15)
    elif kind=='meter':
        a.box((0,0,.19),(.24,.075,.38),4);a.box((0,-.043,.22),(.14,.008,.14),9)
        a.box((0,-.049,.24),(.095,.002,.025),13)
        for x in [-.055,.055]:a.rod((x,0,.10),(x,0,.0),.008,tube,15)
    elif kind=='dish':
        a.lathe((0,0,.48),[(0,0),(.11,.015),(.23,.07),(.30,.13)],s,4,'Y',caps=False)
        a.rod((0,0,.16),(0,.08,.48),.018,tube,6)
        a.rod((0,0,.48),(0,-.24,.56),.009,tube,6)
        a.box((0,-.24,.56),(.07,.04,.05),9)
        a.box((0,0,.04),(.15,.18,.08),6)
    elif kind=='bin':
        a.box((0,0,.43),(.43,.46,.82),3);a.box((0,0,.86),(.46,.49,.040),3)
        for x in [-.23,.23]:a.lathe((x,.15,.085),[(.075,-.02),(.075,.02)],tube,1,'X')
        a.rod((-.19,.22,.73),(.19,.22,.73),.017,tube,9)
    elif kind=='mannequin':
        a.lathe((0,0,.68),[(.105,0),(.13,.10),(.10,.30),(.19,.49),(.06,.55)],s,13)
        a.rod((0,0,.03),(0,0,.68),.012,tube,6)
        for angle in [0,2.094,4.189]:a.rod((0,0,.035),(.21*math.cos(angle),.21*math.sin(angle),.015),.013,tube,6)
    elif kind=='rail':
        for x in [-.48,.48]:
            a.rod((x,0,.03),(x,0,1.48),.018,tube,6)
            a.rod((x,-.25,.025),(x,.25,.025),.017,tube,6)
        a.rod((-.50,0,1.48),(.50,0,1.48),.019,tube,6)
        if small:
            for x in [-.25,0,.25]:
                a.rod((x,0,1.42),(x-.12,0,1.30),.004,4,6);a.rod((x-.12,0,1.30),(x+.12,0,1.30),.004,4,6);a.rod((x+.12,0,1.30),(x,0,1.42),.004,4,6)
    elif kind in ('broom','shovel'):
        a.rod((0,0,.15),(0,0,1.13),.012,tube,0)
        if kind=='shovel':a.box((0,0,.11),(.19,.012,.22),6)
        else:
            for x in np.linspace(-.085,.085,[14,8,5][lod]):a.rod((float(x),0,.012),(float(x)*.30,0,.22),.007,4,10)
            a.box((0,0,.195),(.06,.028,.025),9)
    elif kind=='dustpan':
        a.box((0,0,.012),(.23,.22,.024),5)
        for x in [-.11,.11]:a.box((x,0,.04),(.014,.22,.055),5)
        a.rod((0,.11,.04),(0,.29,.06),.012,tube,5)
    elif kind=='pallet':
        for x in [-.38,0,.38]:a.box((x,0,.070),(.10,.75,.10),0)
        for y in [-.32,-.16,0,.16,.32]:a.box((0,y,.135),(.90,.13,.030),0)
        if small:
            for x in [-.38,.38]:
                for y in [-.32,.32]:a.lathe((x,y,.151),[(.004,0),(.004,.001)],6,6)
    elif kind=='sack':
        a.lathe((0,0,0),[(.11,0),(.22,.09),(.24,.22),(.20,.32),(.07,.37)],s,10)
        a.box((0,0,.385),(.13,.026,.032),10)
    elif kind=='compressor':
        a.lathe((0,0,.29),[(.09,-.29),(.14,-.24),(.14,.24),(.09,.29)],s,5,'X')
        a.box((0,0,.47),(.29,.18,.14),15)
        for x in [-.22,.22]:a.lathe((x,.075,.075),[(.067,-.018),(.067,.018)],tube,1,'X')
        a.box((0,-.13,.06),(.10,.04,.12),6)
        if small:
            for x in [-.10,-.05,0,.05,.10]:a.box((x,0,.55),(.015,.18,.015),6)
            a.lathe((.10,-.10,.50),[(.032,0),(.032,.012)],12,4,'Y')
    elif kind=='pump':
        a.box((0,0,.015),(.24,.16,.030),6)
        a.lathe((0,0,.03),[(.024,0),(.024,.37)],tube,6)
        a.rod((0,0,.38),(0,0,.49),.006,tube,6);a.rod((-.09,0,.49),(.09,0,.49),.015,tube,9)
        a.ring((.06,.02,.035),.08,.006,[14,8,5][lod],15)
    elif kind=='tooltray':
        a.box((0,0,.012),(.35,.22,.024),6)
        for y in [-.11,.11]:a.box((0,y,.032),(.35,.010,.040),6)
        for x in [-.175,.175]:a.box((x,0,.032),(.010,.22,.04),6)
        for x in [-.09,.025]:
            a.box((x,0,.028),(.014,.15,.012),6);a.box((x,.073,.030),(.031,.025,.013),6)
        if small:a.rod((.10,-.06,.03),(.10,.055,.03),.008,tube,5)
    elif kind=='coil':
        for j in range([4,3,2][lod]):a.ring((0,0,.015+j*.012),.10+j*.005,.007,[24,12,8][lod],15)
    elif kind in ('cart','barrow'):
        if kind=='cart':
            a.box((0,0,.42),(.65,.85,.04),0)
            for x in [-.32,.32]:a.lathe((x,0,.21),[(.20,-.032),(.20,.032)],s,1,'X')
            for x in [-.28,.28]:a.rod((x,.35,.42),(x,.78,.70),.024,tube,6)
            a.rod((-.28,.78,.70),(.28,.78,.70),.020,tube,6)
        else:
            tray=Assembly();tray.tapered_tray(.53,.69,.20,7)
            a.add([(x,y,z+.44) for x,y,z in tray.vertices],tray.faces,7)
            a.lathe((0,-.41,.15),[(.14,-.035),(.14,.035)],s,1,'X')
            for x in [-.19,.19]:
                a.rod((x,-.39,.15),(x,.58,.59),.022,tube,6)
                a.rod((x,.16,.46),(x,.25,.015),.019,tube,6)
    elif kind=='awning':
        a.lathe((0,0,.10),[(.08,-.70),(.08,.70)],s,8,'X')
        for x in [-.60,.60]:a.ring((x,0,.10),.082,.009,tube,9,plane='YZ')
    elif kind=='padlock':
        a.chamfer_box((0,0,.030),(.062,.027,.060),6,.006)
        a.ring((0,0,.070),.022,.004,[16,10,6][lod],6,plane='XZ',arc=math.pi)
        for x in [-.022,.022]:a.rod((x,0,.047),(x,0,.070),.004,tube,6)
        if small:a.box((0,-.015,.021),(.005,.002,.011),9)
    elif kind=='grille':
        # Add-on grille fitted to an existing aperture, not new architecture.
        for x in [-.37,.37]:a.box((x,0,.52),(.023,.030,1.04),6)
        for z in [.012,1.028]:a.box((0,0,z),(.76,.030,.023),6)
        for x in [-.25,-.125,0,.125,.25]:a.rod((x,0,.025),(x,0,1.015),.007,tube,6)
        for z in [.35,.70]:a.rod((-.355,0,z),(.355,0,z),.007,tube,6)
    elif kind=='battery':
        a.chamfer_box((0,0,.16),(.33,.18,.31),9,.009)
        a.chamfer_box((0,0,.325),(.34,.19,.020),9,.006)
        for x in [-.115,.115]:a.lathe((x,0,.337),[(.012,0),(.012,.023)],tube,6)
        if small:
            for x in [-.115,.115]:a.box((x,0,.365),(.032,.028,.007),11 if x>0 else 9)
    elif kind=='fan':
        a.lathe((0,0,0),[(.18,0),(.18,.025),(.04,.050)],s,4)
        a.rod((0,0,.04),(0,0,.94),.015,tube,6)
        for y in [-.040,.040]:a.ring((0,y,1.14),.22,.005,[24,12,8][lod],6,plane='XZ')
        for i in range([16,8,5][lod]):
            angle=i*2*math.pi/[16,8,5][lod]
            a.rod((0,-.040,1.14),(.215*math.cos(angle),-.040,1.14+.215*math.sin(angle)),.0025,4,6)
        a.lathe((0,-.045,1.14),[(.040,0),(.040,.045)],tube,4,'Y')
        for i in range(3):
            angle=i*2*math.pi/3
            a.rod((0,0,1.14),(.14*math.cos(angle),0,1.14+.14*math.sin(angle)),.022,tube,4)
    elif kind=='stoolstack':
        for j in range(3):
            offset=j*.075
            a.box((0,0,.34+offset),(.32,.30,.030),plastic)
            for x in [-.13,.13]:
                for y in [-.12,.12]:a.rod((x*1.12,y*1.12,.01+offset),(x,y,.34+offset),.015,tube,plastic)
    else:raise ValueError(kind)
    return a


SPECS=[
    ('SM_PlasticChair_Maintained','shop-life','chair',False),('SM_PlasticChair_Worn','shop-life','chair',True),
    ('SM_PlasticStool_A','shop-life','stool',False),('SM_BeverageCrate_A','shop-life','crate',False),
    ('SM_BeverageCrate_B','shop-life','crate',True),('SM_CoolerBox_A','shop-life','cooler',False),
    ('SM_ProduceTray_A','shop-life','tray',False),('SM_ProduceTomatoes_A','shop-life','tomatoes',False),
    ('SM_ProduceOnions_A','shop-life','onions',False),('SM_CounterScale_A','shop-life','scale',False),
    ('SM_MarketTable_A','shop-life','table',False),('SM_WovenBasket_A','shop-life','basket',False),
    ('SM_TailorMannequin_A','shop-life','mannequin',False),('SM_ClothesRail_A','shop-life','rail',False),
    ('SM_Jerrycan20L_A','household-utilities','jerrycan',False),('SM_WaterDrum_A','household-utilities','drum',False),
    ('SM_WaterTank_A','household-utilities','tank',False),('SM_HandledBucket_A','household-utilities','bucket',False),
    ('SM_WashBasin_A','household-utilities','basin',False),('SM_WindowAC_A','household-utilities','ac',False),
    ('SM_MeterBox_A','household-utilities','meter',False),('SM_SatelliteDish_A','household-utilities','dish',False),
    ('SM_WheeledBin_A','household-utilities','bin',False),('SM_Broom_A','street-work','broom',False),
    ('SM_Dustpan_A','street-work','dustpan',False),('SM_Pallet_A','street-work','pallet',False),
    ('SM_SandSack_A','street-work','sack',False),('SM_AirCompressor_A','street-work','compressor',False),
    ('SM_FootPump_A','street-work','pump',False),('SM_ToolTray_A','street-work','tooltray',False),
    ('SM_CableCoil_A','street-work','coil',False),('SM_Handcart_A','street-work','cart',False),
    ('SM_Wheelbarrow_A','street-work','barrow',False),('SM_Shovel_A','street-work','shovel',False),
    ('SM_AwningRoll_A','street-work','awning',False),
    ('SM_Padlock_A','household-utilities','padlock',False),('SM_WindowSecurityGrille_A','household-utilities','grille',False),
    ('SM_InverterBattery_A','household-utilities','battery',False),('SM_PedestalFan_A','shop-life','fan',False),
    ('SM_StackedStools_A','shop-life','stoolstack',False)]


def main(expected_pid=3224):
    if os.getpid()!=expected_pid or bpy.data.filepath.replace('\\','/')!='C:/dev/baddie-assets/Blender/LagosWorkbench.blend':raise RuntimeError('Wrong workbench identity')
    if bpy.app.is_job_running('RENDER') or bpy.data.scenes.get(OWNER) or (ROOT/'manifest.json').exists():raise RuntimeError('Busy or already-authored kit')
    for name in ['textures','fbx','receipts']:(ROOT/name).mkdir(exist_ok=True)
    before={'file':bpy.data.filepath,'scene':bpy.context.scene.name,'objects':[o.name for o in bpy.context.scene.objects]}
    scene=bpy.data.scenes.new(OWNER);scene.unit_settings.system='METRIC';scene.unit_settings.scale_length=1;scene['detail_owner']=OWNER
    collection=bpy.data.collections.new(OWNER);scene.collection.children.link(collection)
    material,images,provenance=make_atlas()
    records=[];previous=bpy.context.window.scene
    try:
        bpy.context.window.scene=scene
        for name,batch,kind,worn in SPECS:
            lods=[]
            for lod in range(3):
                label=name if lod==0 else name+'_LOD'+str(lod)
                obj=build(kind,lod,worn).object(collection,label,material)
                obj['lod_index']=lod
                # Tiny physical bevels on broad hard edges; never inflate props.
                if lod==0 and kind not in ('tomatoes','onions','basket','coil','broom','sack'):
                    bevel=obj.modifiers.new('Contact edge radius','BEVEL');bevel.width=.0015;bevel.segments=1;bevel.limit_method='ANGLE'
                bpy.context.view_layer.update()
                evaluated=obj.evaluated_get(bpy.context.evaluated_depsgraph_get());mesh=evaluated.to_mesh();mesh.calc_loop_triangles()
                triangles=len(mesh.loop_triangles);evaluated.to_mesh_clear()
                for other in scene.objects:other.select_set(False)
                obj.select_set(True);bpy.context.view_layer.objects.active=obj
                path=ROOT/'fbx'/f'{label}.fbx'
                with contextlib.redirect_stdout(io.StringIO()):
                    bpy.ops.export_scene.fbx(filepath=str(path),use_selection=True,object_types={'MESH'},global_scale=1,apply_unit_scale=True,apply_scale_options='FBX_SCALE_UNITS',axis_forward='-Y',axis_up='Z',use_mesh_modifiers=True,bake_anim=False,use_custom_props=True,path_mode='STRIP')
                payload=path.read_bytes()
                if label.encode() not in payload:raise RuntimeError('Wrong FBX selection '+label)
                lods.append({'file':str(path.relative_to(ROOT)).replace('\\','/'),'sha256':sha(path),'bytes':len(payload),'triangles':triangles,'dimensions_metres':[round(float(v),5) for v in obj.dimensions]})
                obj.hide_set(lod>0);obj.hide_render=lod>0
            records.append({'id':name,'batch':batch,'kind':kind,'maintenance':'worn' if worn else 'usable-maintained-or-neutral','material':MATERIAL,'collision':'none','nanite_default':False,'lod_screen_sizes':[1,.12,.035],'lods':lods,'pivot':'local attachment/contact origin; Z up; metres'})
    finally:bpy.context.window.scene=previous
    blend=ROOT/'LagosEverydayLifeV2.blend';bpy.data.libraries.write(str(blend),{scene},path_remap='RELATIVE',fake_user=True,compress=True)
    after={'file':bpy.data.filepath,'scene':bpy.context.scene.name,'objects':[o.name for o in bpy.context.scene.objects]}
    manifest={'schema_version':1,'kit':OWNER,'status':'exported-unplaced-native-visual-acceptance-pending','blender_version':bpy.app.version_string,'pid':os.getpid(),
              'blend':blend.name,'blend_sha256':sha(blend),'author_script_sha256':sha(Path(__file__)),
              'assets':records,'textures':[{'file':str(p.relative_to(ROOT)).replace('\\','/'),'sha256':sha(p),'bytes':p.stat().st_size,'srgb':p.name.endswith('_BC.png')} for p in sorted((ROOT/'textures').glob('*.png'))],
              'provenance':provenance,'coordinate_contract':{'units':'metres','axis_forward':'-Y','axis_up':'Z','unreal_convert_scene':True,'unreal_convert_scene_unit':True},
              'workbench_unchanged':before==after,'performance':{'shared_master_materials':1,'atlas_resolution':2048,'tick':False,'simulation':False,'measured_frame_time':None},
              'acceptance':{'source_export':'object IDs and hashes inspected','rendered_geometry':'not-run','unreal_import':'not-run','texture_review':'pending','scene_placement':'none'}}
    (ROOT/'manifest.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')
    return {'assets':len(records),'fbx':len(records)*3,'blend':str(blend),'manifest':str(ROOT/'manifest.json'),'workbench_unchanged':before==after}


if __name__=='__main__':result=main()

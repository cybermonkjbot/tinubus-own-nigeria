"""Owned connection-detail meshes. No primary layout or Unreal writes.

Use the identified idle workbench. Share v2 textures rather than generating
another copy of the same atlas. Every deliverable is an isolated source asset.
"""
import contextlib
import io
import json
import math
import os
import runpy
from pathlib import Path

import bmesh
import bpy
from mathutils import Vector

ROOT=Path(__file__).resolve().parent
OWNER='LagosConnectionsV5'
MATERIAL='M_LagosConnections_Opaque'
HELPER=ROOT.parent/'secondary-detailing-v2/author_everyday.py'
TEXTURE_ROOT=ROOT.parent/'secondary-detailing-v2/textures'
shared=runpy.run_path(str(HELPER))
sha=shared['sha']


class Assembly(shared['Assembly']):
    def __init__(self):
        super().__init__()
        self.smooth_faces=[]

    def tube(self,points,radius,wall,segments,tile,end_tangents=None):
        # Closed wall volume, open bore. Repeated sections maintain endpoint
        # and aperture dimensions at every LOD.
        frames=[]
        points=[Vector(p) for p in points]
        for i,p in enumerate(points):
            tangent=(points[min(i+1,len(points)-1)]-points[max(0,i-1)]).normalized()
            if end_tangents and i in (0,len(points)-1):
                tangent=Vector(end_tangents[0 if i==0 else 1]).normalized()
            x=Vector((1,0,0))
            if abs(tangent.dot(x))>.95:x=Vector((0,1,0))
            u=(x-tangent*x.dot(tangent)).normalized();v=tangent.cross(u).normalized()
            frames.append((p,u,v))
        vertices=[]
        for r in [radius,radius-wall]:
            for p,u,v in frames:
                for j in range(segments):
                    angle=2*math.pi*j/segments
                    vertices.append(tuple(p+r*(u*math.cos(angle)+v*math.sin(angle))))
        count=len(points)*segments;faces=[]
        for side in range(2):
            offset=side*count
            for i in range(len(points)-1):
                for j in range(segments):
                    face=(offset+i*segments+j,offset+i*segments+(j+1)%segments,
                          offset+(i+1)*segments+(j+1)%segments,offset+(i+1)*segments+j)
                    faces.append(face if side==0 else tuple(reversed(face)))
        first=len(self.faces)
        smooth_count=len(faces)
        for end in [0,len(points)-1]:
            for j in range(segments):
                outer=end*segments+j;next_=end*segments+(j+1)%segments
                faces.append((outer,next_,next_+count,outer+count))
        self.add(vertices,faces,tile)
        self.smooth_faces.extend(range(first,first+smooth_count))

    def torus(self,center,major,minor,segments,sides,tile,plane='XY'):
        vertices=[];faces=[]
        for i in range(segments):
            u=2*math.pi*i/segments
            for j in range(sides):
                v=2*math.pi*j/sides;r=major+minor*math.cos(v)
                x,y,z=r*math.cos(u),r*math.sin(u),minor*math.sin(v)
                if plane=='XZ':x,y,z=x,z,y
                vertices.append((x+center[0],y+center[1],z+center[2]))
        for i in range(segments):
            for j in range(sides):faces.append((i*sides+j,i*sides+(j+1)%sides,((i+1)%segments)*sides+(j+1)%sides,((i+1)%segments)*sides+j))
        first=len(self.faces);self.add(vertices,faces,tile);self.smooth_faces.extend(range(first,len(self.faces)))

    def object(self,collection,name,material):
        obj=super().object(collection,name,material)
        # Recalculate modeled solid-shell normals before exporting. No
        # albedo-derived normals or smoothing across unrelated box joints.
        bm=bmesh.new();bm.from_mesh(obj.data)
        bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces))
        bm.to_mesh(obj.data);bm.free()
        for i in self.smooth_faces:obj.data.polygons[i].use_smooth=True
        obj['detail_owner']=OWNER
        return obj


def build(kind,lod):
    a=Assembly();s=[20,12,8][lod];small=lod==0
    if kind=='straight':
        a.tube([(0,0,0),(0,0,1)],.050,.004,s,4)
        a.tube([(0,0,.89),(0,0,1)],.055,.004,s,4)
    elif kind=='elbow':
        steps=[12,6,4][lod]
        points=[(0,.13*(1-math.cos(i*math.pi/2/steps)),.13*math.sin(i*math.pi/2/steps)) for i in range(steps+1)]
        a.tube(points,.050,.004,s,4,end_tangents=((0,0,1),(0,1,0)))
    elif kind=='clip':
        a.tube([(0,0,0),(0,0,.038)],.057,.006,s,6)
        for x in [-.075,.075]:
            a.box((x,0,.019),(.040,.026,.018),6)
            if small:a.lathe((x,0,.028),[(.005,0),(.005,.003)],6,6)
    elif kind=='outlet':
        a.tube([(0,.030,.085),(0,-.13,.085)],.061,.007,s,4)
        a.box((-.09,.030,.085),(.025,.02,.17),7)
        a.box((.09,.030,.085),(.025,.02,.17),7)
    elif kind=='tap':
        a.tube([(0,.015,.11),(0,-.075,.11),(0,-.09,.08)],.014,.003,s,6)
        a.rod((0,-.040,.11),(0,-.040,.15),.009,[8,6,4][lod],6)
        a.rod((-.035,-.040,.15),(.035,-.040,.15),.006,[8,6,4][lod],6)
        a.box((0,.020,.11),(.050,.015,.050),6)
    elif kind=='valve':
        a.tube([(0,.10,.055),(0,-.10,.055)],.025,.004,s,6)
        a.rod((0,0,.055),(0,0,.13),.010,[8,6,4][lod],6)
        a.torus((0,0,.14),.046,.006,s,4,14)
        for angle in [0,2.094,4.189]:a.rod((0,0,.14),(.043*math.cos(angle),.043*math.sin(angle),.14),.004,4,14)
    elif kind=='hose':
        for i in range(3):a.torus((0,0,.014+i*.023),.16,.011,s,[6,5,4][lod],9)
        a.tube([(.16,0,.012),(.26,0,.012)],.011,.003,s,9)
    elif kind=='acdrain':
        a.tube([(0,0,.60),(0,0,.07),(0,-.045,.025)],.008,.0015,s,4)
        if small:
            for z in [.13,.48]:a.box((0,.012,z),(.021,.012,.017),4)
    elif kind=='junction':
        a.chamfer_box((0,0,.13),(.16,.068,.26),14,.008)
        a.box((0,-.039,.13),(.135,.008,.225),14)
        for x in [-.040,.040]:a.tube([(x,0,0),(x,0,.032)],.012,.003,s,9)
        if small:
            for x in [-.055,.055]:
                for z in [.040,.220]:a.lathe((x,-.042,z),[(.003,0),(.003,.003)],6,6,'Y')
    elif kind=='cableclip':
        a.tube([(0,0,0),(0,0,.024)],.010,.002,s,9)
        a.box((.016,0,.012),(.020,.016,.010),9)
    elif kind=='insulator':
        profile=[(.014,0)]
        for i in range(3):profile.extend([(.014,i*.028+.003),(.029,i*.028+.015),(.014,i*.028+.024)])
        profile.append((.014,.096))
        a.lathe((0,0,0),profile,s,4)
        a.rod((0,0,0),(0,0,.107),.006,[8,6,4][lod],6)
    elif kind=='driploop':
        points=[(0,-.15*math.sin(i*math.pi/[16,8,4][lod]),.35-.22*math.sin(i*math.pi/[16,8,4][lod])) for i in range([16,8,4][lod]+1)]
        # Two distinct wall attachments, not a closed curve with zero-length
        # tangent at coincident endpoints.
        for i in range(len(points)):points[i]=((i/(len(points)-1)-.5)*.22,points[i][1],points[i][2])
        a.tube(points,.006,.001,s,9)
    elif kind=='roofpatch':
        waves=[24,16,12][lod];vertices=[]
        for zoffset in [0,.001]:
            for y in [-.20,.20]:
                for i in range(waves+1):
                    x=(i/waves-.5)*.58;z=.018+.014*math.cos(i/waves*math.pi*12)+zoffset
                    vertices.append((x,y,z))
        row=waves+1;side=row*2;faces=[]
        for layer in range(2):
            for i in range(waves):faces.append((layer*side+i,layer*side+i+1,layer*side+row+i+1,layer*side+row+i))
        for i in range(waves):
            faces.extend([(i,i+1,side+i+1,side+i),(row+i,row+i+1,side+row+i+1,side+row+i)])
        faces.extend([(0,row,side+row,side),(waves,row+waves,side+row+waves,side+waves)])
        a.add(vertices,faces,7)
        if small:
            for x in [-.24,.24]:
                # Match the piecewise-linear modeled crest, not an assumed
                # peak height: bolt shanks touch the actual patch surface.
                sample=(x/.58+.5)*waves;index=min(waves-1,int(sample));fraction=sample-index
                z0=.018+.014*math.cos(index/waves*math.pi*12)
                z1=.018+.014*math.cos((index+1)/waves*math.pi*12)
                surface=z0+(z1-z0)*fraction+.001
                for y in [-.15,.15]:a.lathe((x,y,surface),[(.004,0),(.004,.003)],6,6)
    elif kind=='ridgecap':
        vertices=[(-.12,y,0) for y in [-.42,.42]]+[(0,y,.09) for y in [-.42,.42]]+[(.12,y,0) for y in [-.42,.42]]
        vertices+= [(x,y,z+.001) for x,y,z in list(vertices)]
        a.add(vertices,[(0,1,3,2),(2,3,5,4),(6,8,9,7),(8,10,11,9),(0,6,7,1),(4,5,11,10),(0,2,8,6),(2,4,10,8),(1,7,9,3),(3,9,11,5)],6)
    elif kind=='boltstrip':
        a.box((0,0,.004),(.30,.027,.008),6)
        for x in [-.12,0,.12]:a.lathe((x,0,.008),[(.008,0),(.008,.004),(.005,.005),(.005,.009)],6,6)
    elif kind=='hasp':
        a.box((0,0,.022),(.13,.018,.044),6)
        a.tube([(.035,-.013,.010),(.035,-.013,.035)],.010,.003,s,6)
        if small:
            for x in [-.045,.050]:a.lathe((x,-.011,.022),[(.003,0),(.003,.003)],6,6,'Y')
    elif kind=='hinge':
        for x in [-.034,.034]:a.box((x,0,.055),(.055,.004,.11),6)
        a.lathe((0,-.006,0),[(.006,0),(.006,.11)],s,6)
        if small:
            for x in [-.039,.039]:
                for z in [.024,.086]:a.lathe((x,-.004,z),[(.003,0),(.003,.002)],6,6,'Y')
    elif kind=='bracket':
        a.box((0,0,.15),(.030,.016,.30),6)
        a.box((0,-.12,.30),(.030,.26,.016),6)
        a.rod((0,0,.045),(0,-.22,.295),.010,[8,6,4][lod],6)
    elif kind=='catchscreen':
        # Removable accessory screen for an existing opening; no new gutter.
        for x in [-.15,.15]:a.box((x,0,.085),(.012,.030,.17),6)
        for z in [.006,.164]:a.box((0,0,z),(.30,.030,.012),6)
        for x in [-.10,-.05,0,.05,.10]:a.rod((x,0,.018),(x,0,.152),.004,[8,6,4][lod],6)
    elif kind=='handle':
        for x in [-.045,.045]:a.box((x,0,.004),(.030,.030,.008),6)
        a.rod((-.045,0,.008),(-.045,0,.037),.005,[8,6,4][lod],6)
        a.rod((-.045,0,.037),(.045,0,.037),.005,[8,6,4][lod],6)
        a.rod((.045,0,.037),(.045,0,.008),.005,[8,6,4][lod],6)
    else:raise ValueError(kind)
    return a


SPECS=[('SM_Downpipe1m_A','water-connections','straight'),('SM_DownpipeElbow90_A','water-connections','elbow'),
       ('SM_DownpipeClip_A','water-connections','clip'),('SM_DrainWallOutlet_A','water-connections','outlet'),
       ('SM_WallTap_A','water-connections','tap'),('SM_WaterStopValve_A','water-connections','valve'),
       ('SM_HoseCoil_A','water-connections','hose'),('SM_ACDrainTube_A','water-connections','acdrain'),
       ('SM_CableJunctionBox_A','cable-fittings','junction'),('SM_CableClip_A','cable-fittings','cableclip'),
       ('SM_PorcelainInsulator_A','cable-fittings','insulator'),('SM_CableDripLoop_A','cable-fittings','driploop'),
       ('SM_CorrugatedRoofPatch_A','repair-hardware','roofpatch'),('SM_RoofRidgeCap_A','repair-hardware','ridgecap'),
       ('SM_RoofBoltStrip_A','repair-hardware','boltstrip'),('SM_ShutterHasp_A','repair-hardware','hasp'),
       ('SM_DoorHinge_A','repair-hardware','hinge'),('SM_CounterSupportBracket_A','repair-hardware','bracket'),
       ('SM_DrainCatchScreen_A','repair-hardware','catchscreen'),('SM_AccessCoverHandle_A','repair-hardware','handle')]


def materials():
    manifest=json.loads((ROOT.parent/'secondary-detailing-v2/manifest.json').read_text())
    images={};records=[]
    for record in manifest['textures']:
        path=ROOT.parent/'secondary-detailing-v2'/record['file']
        if sha(path)!=record['sha256']:raise RuntimeError('Shared atlas source mismatch')
        image=bpy.data.images.load(str(path),check_existing=False)
        image.colorspace_settings.name='sRGB' if record['srgb'] else 'Non-Color';image.pack()
        images[path.stem]=image
        records.append({**record,'file':'../secondary-detailing-v2/'+record['file']})
    mat=bpy.data.materials.new(MATERIAL);mat.use_nodes=True
    nodes=mat.node_tree.nodes;links=mat.node_tree.links;bsdf=nodes.get('Principled BSDF')
    bc=nodes.new('ShaderNodeTexImage');bc.image=images['T_DetailAtlas_BC']
    orm=nodes.new('ShaderNodeTexImage');orm.image=images['T_DetailAtlas_ORM']
    split=nodes.new('ShaderNodeSeparateColor')
    links.new(bc.outputs['Color'],bsdf.inputs['Base Color']);links.new(orm.outputs['Color'],split.inputs['Color'])
    links.new(split.outputs['Green'],bsdf.inputs['Roughness']);links.new(split.outputs['Blue'],bsdf.inputs['Metallic'])
    return mat,records


def main(expected_pid,expected_file):
    if os.getpid()!=expected_pid or bpy.data.filepath.replace('\\','/')!=expected_file:raise RuntimeError('Workbench identity mismatch')
    if bpy.app.is_job_running('RENDER') or bpy.data.scenes.get(OWNER) or (ROOT/'manifest.json').exists():raise RuntimeError('Busy or existing kit')
    for folder in ['fbx','receipts']:(ROOT/folder).mkdir(exist_ok=True)
    previous=bpy.context.window.scene
    before={'file':bpy.data.filepath,'scene':previous.name,'objects':[o.name for o in previous.objects]}
    scene=bpy.data.scenes.new(OWNER);scene['detail_owner']=OWNER;scene.unit_settings.system='METRIC';scene.unit_settings.scale_length=1
    collection=bpy.data.collections.new(OWNER);scene.collection.children.link(collection)
    mat,textures=materials();records=[]
    try:
        bpy.context.window.scene=scene
        for name,batch,kind in SPECS:
            lods=[]
            for lod in range(3):
                label=name if lod==0 else name+'_LOD'+str(lod)
                obj=build(kind,lod).object(collection,label,mat);obj['lod_index']=lod
                bpy.context.view_layer.update()
                evaluated=obj.evaluated_get(bpy.context.evaluated_depsgraph_get());mesh=evaluated.to_mesh();mesh.calc_loop_triangles()
                triangles=len(mesh.loop_triangles);evaluated.to_mesh_clear()
                for other in scene.objects:other.select_set(False)
                obj.select_set(True);bpy.context.view_layer.objects.active=obj
                path=ROOT/'fbx'/f'{label}.fbx'
                with contextlib.redirect_stdout(io.StringIO()):
                    bpy.ops.export_scene.fbx(filepath=str(path),use_selection=True,object_types={'MESH'},global_scale=1,apply_unit_scale=True,apply_scale_options='FBX_SCALE_UNITS',axis_forward='-Y',axis_up='Z',use_mesh_modifiers=True,bake_anim=False,use_custom_props=True,path_mode='STRIP')
                if label.encode() not in path.read_bytes():raise RuntimeError('Export selection mismatch')
                lods.append({'file':'fbx/'+path.name,'sha256':sha(path),'bytes':path.stat().st_size,'triangles':triangles,
                             'dimensions_metres':[round(float(v),5) for v in obj.dimensions]})
                obj.hide_set(lod>0);obj.hide_render=lod>0
            records.append({'id':name,'batch':batch,'kind':kind,'material':MATERIAL,'lods':lods,'collision':'none','nanite_default':False,
                            'lod_screen_sizes':[1,.12,.035],'pivot':'local contact or attachment origin; metres; Z up','placement':'none'})
    finally:bpy.context.window.scene=previous
    blend=ROOT/'LagosConnectionsV5.blend';bpy.data.libraries.write(str(blend),{scene},path_remap='RELATIVE',fake_user=True,compress=True)
    manifest={'schema_version':1,'kit':OWNER,'status':'exported-unplaced-native-acceptance-pending','blender_version':bpy.app.version_string,
              'blend':blend.name,'blend_sha256':sha(blend),'author_script_sha256':sha(Path(__file__)),'geometry_helper_sha256':sha(HELPER),
              'shared_importer_sha256_at_authoring':sha(ROOT.parent/'secondary-detailing-v1/import_unreal.py'),
              'assets':records,'textures':textures,'surface_provenance':'../secondary-detailing-v2/manifest.json',
              'coordinate_contract':{'units':'metres','axis_forward':'-Y','axis_up':'Z','convert_scene_unit':True},
              'performance':{'materials':1,'new_external_texture_bytes':0,'shared_atlas':2048,'tick':False,'physics':False,'measured_fps':None},
              'workbench_restored':before=={'file':bpy.data.filepath,'scene':bpy.context.scene.name,'objects':[o.name for o in bpy.context.scene.objects]},
              'acceptance':{'fbx_id_inspection':'done','native_import':'not-run','rendered_geometry':'not-run','world_placement':'none'}}
    (ROOT/'manifest.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')
    return {'assets':len(records),'fbx':len(records)*3,'blend':str(blend),'workbench_restored':manifest['workbench_restored']}

"""Static original waterfront props/light fittings; never author a world layout."""
import contextlib
import io
import json
import math
import os
import runpy
from pathlib import Path

import bpy

ROOT=Path(__file__).resolve().parent
OWNER='LagosWaterfrontLightsV4'
MATERIAL='M_LagosWaterfront_Opaque'
HELPER=ROOT.parent/'secondary-detailing-v2/author_everyday.py'
helper=runpy.run_path(str(HELPER))
helper['make_atlas'].__globals__.update(ROOT=ROOT,OWNER=OWNER,MATERIAL=MATERIAL)
Assembly=helper['Assembly'];sha=helper['sha']


def canoe(a,lod,painted=False):
    # Narrow double-ended open hull with real wall thickness and interior.
    # Designed craft, not a surveyed Makoko boat or a gameplay vessel.
    rows=[25,13,7][lod]
    stations=[]
    for j in range(rows):
        t=j/(rows-1);y=(t-.5)*4.25
        width=.015+.47*math.sin(math.pi*t)**.78
        rise=.04+.27*abs(2*t-1)**3
        stations.append((y,width,rise))
    vertices=[]
    # Traverse outside starboard->keel->port and inside port->keel->starboard.
    for y,w,r in stations:
        vertices.extend([(w,y,r+.47),(w*.71,y,r+.12),(w*.22,y,r),( -w*.22,y,r),(-w*.71,y,r+.12),(-w,y,r+.47),
                         (-max(.004,w-.025),y,r+.47),(-max(.003,w*.71-.020),y,r+.14),
                         (-max(.002,w*.22-.014),y,r+.025),(max(.002,w*.22-.014),y,r+.025),
                         (max(.003,w*.71-.020),y,r+.14),(max(.004,w-.025),y,r+.47)])
    faces=[]
    for j in range(rows-1):
        for i in range(12):faces.append((j*12+i,j*12+(i+1)%12,(j+1)*12+(i+1)%12,(j+1)*12+i))
    faces.extend([tuple(reversed(range(12))),tuple((rows-1)*12+i for i in range(12))])
    a.add(vertices,faces,2 if painted else 0)
    for y in [-.85,.05,.86]:
        t=(y+2.125)/4.25;w=.015+.47*math.sin(math.pi*t)**.78
        a.box((0,y,.43),((w-.03)*2,.16,.028),0)
    if lod==0:
        for y in [-1.3,-.5,.5,1.3]:
            t=(y+2.125)/4.25;w=.015+.47*math.sin(math.pi*t)**.78
            for sign in [-1,1]:a.rod((sign*w*.24,y,.070),(sign*(w-.035),y,.43),.013,4,0)


def build(kind,lod):
    a=Assembly();s=[24,12,8][lod];tube=[8,6,4][lod];small=lod==0
    if kind in ('canoe','canoepainted'):canoe(a,lod,kind=='canoepainted')
    elif kind=='paddle':
        a.rod((0,0,.30),(0,0,1.40),.014,tube,0)
        vertices=[(-.075,-.007,0),(.075,-.007,0),(.09,-.007,.24),(.025,-.007,.40),(-.025,-.007,.40),(-.09,-.007,.24)]
        n=len(vertices);vertices.extend([(x,.007,z) for x,y,z in vertices])
        a.add(vertices,[tuple(reversed(range(n))),tuple(n+i for i in range(n))]+[(i,(i+1)%n,n+(i+1)%n,n+i) for i in range(n)],0)
    elif kind=='pushpole':a.rod((0,0,0),(0,0,2.35),.018,tube,0)
    elif kind=='bailer':
        a.lathe((0,0,0),[(.055,0),(.075,.11),(.073,.12),(.065,.108),(.045,.015)],s,5,caps=True)
        a.rod((.06,0,.055),(.16,0,.105),.010,tube,5)
    elif kind=='mooringpost':
        a.lathe((0,0,0),[(.074,0),(.062,.70),(.07,.83)],s,0)
        a.rod((-.14,0,.62),(.14,0,.62),.030,tube,0)
    elif kind=='rope':
        for i in range([5,3,2][lod]):a.ring((0,0,.012+i*.012),.13+i*.004,.006,[24,12,8][lod],10)
    elif kind=='fender':
        # Tire on its side with two tie points, separate from existing v2 tire.
        vertices=[];faces=[];segments=[32,16,10][lod];rings=[10,6,4][lod]
        for j in range(rings):
            b=2*math.pi*j/rings
            for i in range(segments):
                angle=2*math.pi*i/segments;r=.20+.060*math.cos(b)
                vertices.append((r*math.cos(angle),.064+.060*math.sin(b),.26+r*math.sin(angle)))
        for j in range(rings):
            for i in range(segments):faces.append((j*segments+i,j*segments+(i+1)%segments,((j+1)%rings)*segments+(i+1)%segments,((j+1)%rings)*segments+i))
        a.add(vertices,faces,1)
        for x in [-.11,.11]:a.rod((x,.055,.40),(x,.055,.66),.005,tube,10)
    elif kind=='landingboards':
        for x in [-.23,0,.23]:a.box((x,0,.11),(.21,1.30,.040),0)
        for y in [-.47,.47]:a.box((0,y,.050),(.70,.10,.10),0)
    elif kind=='repairtimber':
        for i in range(4):a.box(((i-1.5)*.13,0,.03+i*.009),(.105,1.18-i*.10,.035),0)
    elif kind=='fishrack':
        for x in [-.42,.42]:
            for y in [-.25,.25]:a.box((x,y,.35),(.035,.035,.70),0)
        for y in [-.25,.25]:a.box((0,y,.70),(.88,.035,.040),0)
        for x in [-.42,.42]:a.box((x,0,.70),(.035,.54,.040),0)
        for x in [-.34,-.17,0,.17,.34]:a.rod((x,-.25,.69),(x,.25,.69),.004,tube,6)
    elif kind=='netbundle':
        # Compact bundled net: no full-screen transparent fishing-net plane.
        for j in range([6,4,2][lod]):
            a.ring((0,0,.014+j*.022),.11+j*.012,.009,[16,10,6][lod],14)
        for x in [-.07,.07]:a.rod((x,-.17,.02),(x,.17,.11),.006,tube,10)
    elif kind=='floatline':
        a.rod((-.48,0,.024),(.48,0,.024),.004,tube,10)
        for i in range(7):a.lathe(((i-3)*.135,0,.024),[(.028,-.040),(.034,0),(.028,.040)],tube,13,'X')
    elif kind=='fishtray':
        a.tapered_tray(.48,.32,.055,6)
    elif kind in ('lamphead','oldlamp','solarhead'):
        a.chamfer_box((0,0,.095),(.21,.62,.13),6 if kind!='oldlamp' else 14,.015)
        a.chamfer_box((0,-.040,.015),(.17,.46,.018),4,.012)
        a.rod((0,.29,.09),(0,.47,.09),.026,tube,6)
        if small:
            for y in [-.19,-.11,-.03,.05,.13]:a.box((0,y,.171),(.18,.014,.012),6)
        if kind=='solarhead':
            a.rod((0,.08,.16),(0,.08,.35),.022,tube,6)
            a.chamfer_box((0,.08,.38),(.57,.70,.028),6,.012)
            a.box((0,.08,.397),(.53,.66,.004),8)
            if small:
                for x in [-.18,-.09,0,.09,.18]:a.box((x,.08,.400),(.002,.66,.001),6)
    elif kind=='walllight':
        a.chamfer_box((0,0,.15),(.18,.085,.30),14,.012)
        a.box((0,-.048,.15),(.14,.018,.24),4)
        for x in [-.055,.055]:a.rod((x,-.063,.04),(x,-.063,.26),.004,tube,6)
        for z in [.045,.15,.255]:a.rod((-.063,-.063,z),(.063,-.063,z),.004,tube,6)
    elif kind=='floodlight':
        a.chamfer_box((0,0,.14),(.30,.075,.24),9,.012)
        a.box((0,-.045,.14),(.26,.012,.20),4)
        for x in [-.165,.165]:a.box((x,.015,.12),(.015,.050,.23),6)
        a.box((0,.015,.014),(.345,.050,.018),6)
    elif kind=='polebracket':
        a.box((0,0,.14),(.12,.018,.28),6)
        a.rod((0,0,.22),(0,-.70,.27),.022,tube,6)
        a.rod((0,0,.04),(0,-.50,.26),.013,tube,6)
    elif kind=='lampmast':
        # Freestanding prop; never places or replaces a primary utility pole.
        a.lathe((0,0,0),[(.105,0),(.090,.24),(.058,5.30)],s,6)
        a.box((0,0,.015),(.29,.29,.030),6)
        a.rod((0,0,5.25),(0,-.90,5.48),.034,tube,6)
        if small:a.box((0,-.058,.54),(.09,.013,.25),14)
    else:raise ValueError(kind)
    return a


SPECS=[('SM_MakokoCanoe_Timber_A','waterfront-boats','canoe'),('SM_MakokoCanoe_Painted_A','waterfront-boats','canoepainted'),
       ('SM_CanoePaddle_A','waterfront-boats','paddle'),('SM_CanoePushPole_A','waterfront-boats','pushpole'),('SM_CanoeBailer_A','waterfront-boats','bailer'),
       ('SM_MooringPost_A','landing-accessories','mooringpost'),('SM_MooringRopeCoil_A','landing-accessories','rope'),('SM_TireFender_A','landing-accessories','fender'),('SM_LandingBoards_A','landing-accessories','landingboards'),('SM_RepairTimberStack_A','landing-accessories','repairtimber'),
       ('SM_FishDryingRack_Empty_A','fishing-work','fishrack'),('SM_FishingNetBundle_A','fishing-work','netbundle'),('SM_FishingFloatLine_A','fishing-work','floatline'),('SM_FishHandlingTray_Empty_A','fishing-work','fishtray'),
       ('SM_RoadLampHead_LED_A','light-fixtures','lamphead'),('SM_RoadLampHead_Worn_A','light-fixtures','oldlamp'),('SM_SolarLampHead_A','light-fixtures','solarhead'),('SM_WallBulkheadLight_A','light-fixtures','walllight'),('SM_ShopFloodlight_A','light-fixtures','floodlight'),('SM_LampWallBracket_A','light-fixtures','polebracket'),('SM_LampMast5m_A','light-fixtures','lampmast')]


def main(expected_pid=3224):
    if os.getpid()!=expected_pid or bpy.data.filepath.replace('\\','/')!='C:/dev/baddie-assets/Blender/LagosWorkbench.blend':raise RuntimeError('Workbench identity mismatch')
    if bpy.app.is_job_running('RENDER') or bpy.data.scenes.get(OWNER) or (ROOT/'manifest.json').exists():raise RuntimeError('Busy/existing kit')
    for directory in ['textures','fbx','receipts']:(ROOT/directory).mkdir(exist_ok=True)
    scene=bpy.data.scenes.new(OWNER);scene['detail_owner']=OWNER;scene.unit_settings.system='METRIC';scene.unit_settings.scale_length=1
    collection=bpy.data.collections.new(OWNER);scene.collection.children.link(collection)
    material,images,provenance=helper['make_atlas']();previous=bpy.context.window.scene;records=[]
    try:
        bpy.context.window.scene=scene
        for name,batch,kind in SPECS:
            lods=[]
            for lod in range(3):
                label=name if lod==0 else name+'_LOD'+str(lod)
                obj=build(kind,lod).object(collection,label,material);obj['detail_owner']=OWNER;obj['lod_index']=lod
                if lod==0 and kind not in ('canoe','canoepainted','netbundle','rope','floatline','fender','pushpole'):
                    bevel=obj.modifiers.new('Small physical edges','BEVEL');bevel.width=.001;bevel.segments=1;bevel.limit_method='ANGLE'
                bpy.context.view_layer.update()
                evaluated=obj.evaluated_get(bpy.context.evaluated_depsgraph_get());mesh=evaluated.to_mesh();mesh.calc_loop_triangles();triangles=len(mesh.loop_triangles);evaluated.to_mesh_clear()
                for other in scene.objects:other.select_set(False)
                obj.select_set(True);bpy.context.view_layer.objects.active=obj
                path=ROOT/'fbx'/f'{label}.fbx'
                with contextlib.redirect_stdout(io.StringIO()):
                    bpy.ops.export_scene.fbx(filepath=str(path),use_selection=True,object_types={'MESH'},global_scale=1,apply_unit_scale=True,apply_scale_options='FBX_SCALE_UNITS',axis_forward='-Y',axis_up='Z',use_mesh_modifiers=True,bake_anim=False,use_custom_props=True,path_mode='STRIP')
                if label.encode() not in path.read_bytes():raise RuntimeError('Export ID mismatch '+label)
                lods.append({'file':str(path.relative_to(ROOT)).replace('\\','/'),'sha256':sha(path),'bytes':path.stat().st_size,'triangles':triangles,'dimensions_metres':[round(float(v),5) for v in obj.dimensions]})
                obj.hide_set(lod>0);obj.hide_render=lod>0
            records.append({'id':name,'batch':batch,'kind':kind,'material':MATERIAL,'collision':'none','nanite_default':False,'lod_screen_sizes':[1,.12,.035],'lods':lods,'pivot':'local contact/attachment origin; metres; Z up','runtime_behavior':'static unplaced prop'})
    finally:bpy.context.window.scene=previous
    blend=ROOT/'LagosWaterfrontLightsV4.blend';bpy.data.libraries.write(str(blend),{scene},path_remap='RELATIVE',fake_user=True,compress=True)
    manifest={'schema_version':1,'kit':OWNER,'status':'exported-unplaced-native-visual-acceptance-pending','pid':os.getpid(),'blender_version':bpy.app.version_string,'blend':blend.name,'blend_sha256':sha(blend),'author_script_sha256':sha(Path(__file__)),'geometry_helper_sha256':sha(HELPER),'assets':records,'textures':[{'file':str(p.relative_to(ROOT)).replace('\\','/'),'sha256':sha(p),'bytes':p.stat().st_size,'srgb':p.name.endswith('_BC.png')} for p in sorted((ROOT/'textures').glob('*.png'))],'provenance':provenance,'workbench_restored':bpy.context.scene==previous,'performance':{'materials':1,'atlas_size':2048,'boat_buoyancy':False,'niagara':False,'runtime_lights_created':0,'measured_fps':None},'acceptance':{'unreal_import':'not-run','rendered_geometry':'not-run','texture_review':'pending','light_output':'none; fittings only','world_placement':'none'}}
    (ROOT/'manifest.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')
    return {'assets':len(records),'fbx_count':len(records)*3,'manifest':str(ROOT/'manifest.json'),'workbench_restored':manifest['workbench_restored']}

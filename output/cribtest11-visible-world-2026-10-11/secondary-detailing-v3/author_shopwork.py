"""Owned static props; no primary scene placement, rendering or Unreal writes."""
import contextlib
import io
import json
import math
import os
import runpy
from pathlib import Path

import bpy

ROOT=Path(__file__).resolve().parent
OWNER='LagosShopWorkV3'
MATERIAL='M_LagosShopWork_Opaque'
HELPER=ROOT.parent/'secondary-detailing-v2/author_everyday.py'
shared=runpy.run_path(str(HELPER))
shared['make_atlas'].__globals__.update(ROOT=ROOT,OWNER=OWNER,MATERIAL=MATERIAL)
Assembly=shared['Assembly']
sha=shared['sha']


def translated(target,source,offset,tile):
    target.add([(x+offset[0],y+offset[1],z+offset[2]) for x,y,z in source.vertices],source.faces,tile)


def cloth(a,width,height,lod,tile,vertical=True,fold=.014):
    n=[12,6,2][lod];vertices=[];faces=[]
    for j in range(n+1):
        for i in range(n+1):
            u,v=i/n,j/n;x=(u-.5)*width
            ripple=math.sin(u*math.pi*8)*fold*(.4+.6*v)
            vertices.append((x,ripple,height*(1-v)) if vertical else (x,(v-.5)*height,.012+ripple))
    for j in range(n):
        for i in range(n):
            k=j*(n+1)+i;faces.append((k,k+1,k+n+2,k+n+1))
    # Thin two-surface textile volume keeps the baseline material opaque and
    # avoids per-prop two-sided/translucent passes.
    count=len(vertices);vertices.extend([(x,y+.001,z) if vertical else (x,y,z-.001) for x,y,z in vertices[:]])
    faces.extend([tuple(count+i for i in reversed(face)) for face in list(faces)])
    boundary=list(range(n+1))+[j*(n+1)+n for j in range(1,n+1)]+[n*(n+1)+i for i in range(n-1,-1,-1)]+[j*(n+1) for j in range(n-1,0,-1)]
    for p,q in zip(boundary,boundary[1:]+boundary[:1]):faces.append((p,q,q+count,p+count))
    a.add(vertices,faces,tile)


def build(kind,lod):
    a=Assembly();s=[24,12,8][lod];tube=[8,6,4][lod];small=lod==0
    if kind=='barberchair':
        a.lathe((0,0,0),[(.26,0),(.26,.030),(.16,.055)],s,6)
        a.rod((0,0,.04),(0,0,.43),.042,tube,6)
        a.chamfer_box((0,0,.48),(.48,.45,.10),9,.025)
        a.chamfer_box((0,.19,.76),(.48,.075,.50),9,.02)
        for x in [-.27,.27]:
            a.rod((x,.14,.50),(x,-.13,.66),.014,tube,6)
            a.chamfer_box((x,0,.68),(.060,.37,.045),9,.008)
        a.rod((-.17,-.17,.43),(-.17,-.43,.16),.010,tube,6);a.rod((.17,-.17,.43),(.17,-.43,.16),.010,tube,6)
        a.box((0,-.43,.16),(.38,.17,.018),6)
    elif kind=='mirrorframe':
        for x in [-.26,.26]:a.box((x,0,.43),(.030,.045,.86),0)
        for z in [.015,.845]:a.box((0,0,z),(.55,.045,.030),0)
        a.box((0,.020,.43),(.50,.018,.80),9)
        # Reflective panel is separate owner integration: dark backing is not
        # claimed as a functioning mirror or a reflection-capture solution.
    elif kind=='clipper':
        a.chamfer_box((0,0,.016),(.05,.145,.032),9,.011)
        a.box((0,.075,.020),(.047,.027,.009),6)
        if small:
            for i in range(9):a.box((-.021+i*.005,.087,.021),(.002,.012,.004),6)
    elif kind=='comb':
        a.box((0,0,.007),(.035,.18,.014),9)
        for i in range([22,12,7][lod]):a.box((.025,-.082+i*.164/([22,12,7][lod]-1),.007),(.035,.003,.010),9)
    elif kind=='spray':
        a.lathe((0,0,0),[(.028,0),(.035,.02),(.035,.12),(.015,.15)],s,2)
        a.chamfer_box((0,0,.165),(.055,.028,.030),9,.005)
        a.box((0,-.031,.164),(.018,.040,.016),9)
    elif kind=='hairbottles':
        for x in [-.045,.045]:
            a.lathe((x,0,0),[(.025,0),(.025,.12),(.012,.135)],s,4 if x<0 else 2)
            a.lathe((x,0,.135),[(.014,0),(.014,.025)],tube,9)
    elif kind=='sewingmachine':
        a.chamfer_box((0,0,.025),(.38,.17,.05),9,.01)
        a.chamfer_box((.115,0,.18),(.10,.10,.30),9,.01)
        a.chamfer_box((-.015,0,.31),(.29,.10,.065),9,.01)
        a.chamfer_box((-.145,0,.25),(.070,.085,.11),9,.009)
        a.rod((-.145,0,.17),(-.145,0,.056),.003,tube,6)
        a.lathe((.17,0,.265),[(.047,0),(.047,.024)],s,6,'X')
        if small:a.rod((.07,0,.34),(.07,0,.38),.004,tube,6)
    elif kind=='thread':
        for x,tile in [(-.05,8),(0,11),(.05,13)]:
            a.lathe((x,0,0),[(.019,0),(.019,.006),(.014,.012),(.014,.044),(.019,.050),(.019,.056)],s,tile)
    elif kind=='scissors':
        for x in [-.020,.020]:a.ring((x,-.04,.007),.014,.003,[12,8,6][lod],6)
        a.rod((-.020,-.024,.007),(.018,.08,.007),.004,tube,6)
        a.rod((.020,-.024,.009),(-.018,.08,.009),.004,tube,6)
    elif kind=='iron':
        a.add([(-.05,-.10,0),(.05,-.10,0),(.065,.02,0),(0,.14,0),(-.065,.02,0),
               (-.04,-.08,.05),(.04,-.08,.05),(.047,.02,.05),(0,.10,.05),(-.047,.02,.05)],
              [(4,3,2,1,0),(5,6,7,8,9)]+[(i,(i+1)%5,5+(i+1)%5,5+i) for i in range(5)],6)
        for y in [-.06,.04]:a.box((0,y,.073),(.026,.018,.050),9)
        a.box((0,-.01,.10),(.028,.13,.023),9)
    elif kind=='fabricrolls':
        for i,tile in enumerate([8,11,13]):a.lathe(((i-1)*.085,0,.047),[(.040,-.28),(.040,.28)],s,tile,'Y')
    elif kind=='foldedfabric':
        for i,tile in enumerate([13,8,11]):a.chamfer_box((0,0,.017+i*.026),(.30-i*.008,.24,.025),tile,.009)
    elif kind in ('towel','laundrysheet'):
        cloth(a,.42 if kind=='towel' else .95,.63 if kind=='towel' else 1.10,lod,13 if kind=='towel' else 4)
    elif kind=='shirt':
        outline=[(-.18,0),(-.18,.34),(-.28,.25),(-.37,.42),(-.20,.56),(-.065,.60),(0,.55),(.065,.60),(.20,.56),(.37,.42),(.28,.25),(.18,.34),(.18,0)]
        vertices=[(x,depth,z) for depth in [0,.002] for x,z in outline];n=len(outline)
        a.add(vertices,[tuple(reversed(range(n))),tuple(n+i for i in range(n))]+[(i,(i+1)%n,n+(i+1)%n,n+i) for i in range(n)],8)
        if small:
            for x in [-.12,.12]:a.rod((x,-.002,.02),(x,-.009,.44),.004,4,8)
    elif kind=='pegs':
        for i in range(5):
            x=(i-2)*.022
            a.box((x,0,.014),(.012,.060,.013),0);a.box((x,0,.028),(.012,.054,.013),0)
    elif kind=='dryingrack':
        for x in [-.44,.44]:
            a.rod((x,-.29,.01),(x,.23,.86),.010,tube,6);a.rod((x,.29,.01),(x,-.23,.86),.010,tube,6)
        for y in [-.23,-.115,0,.115,.23]:a.rod((-.44,y,.86),(.44,y,.86),.005,tube,6)
    elif kind=='pot':
        a.lathe((0,0,0),[(.15,0),(.19,.035),(.205,.235),(.20,.246),(.188,.230),(.175,.019)],s,6,caps=True)
        for x in [-.24,.24]:
            a.ring((x,0,.18),.037,.009,[12,8,6][lod],6,plane='YZ')
            for z in [.145,.215]:a.rod((math.copysign(.198,x),0,z),(x,0,z),.009,tube,6)
    elif kind=='lid':
        a.lathe((0,0,0),[(.21,0),(.21,.010),(.14,.033),(.02,.045)],s,6)
        a.lathe((0,0,.04),[(.020,0),(.020,.025)],tube,9)
    elif kind=='ladle':
        a.lathe((0,.16,0),[(.045,.010),(.050,.030),(.047,.034),(.010,.012)],s,6,caps=True)
        a.rod((0,.13,.033),(0,-.19,.038),.006,tube,6)
    elif kind=='foodtray':
        a.tapered_tray(.46,.34,.060,6)
    elif kind=='charcoalstove':
        a.lathe((0,0,0),[(.14,0),(.18,.08),(.18,.24),(.17,.25),(.15,.23),(.12,.070)],s,7,caps=True)
        for angle in [0,2.094,4.189]:
            x,y=.12*math.cos(angle),.12*math.sin(angle)
            a.rod((x,y,.23),(x,y,.28),.013,tube,6)
        if small:a.box((0,-.17,.14),(.055,.015,.038),9)
    elif kind=='breadbasket':
        a.box((0,0,.008),(.39,.26,.016),10)
        for y in [-.13,.13]:a.box((0,y,.055),(.40,.012,.10),10)
        for x in [-.19,.19]:a.box((x,0,.055),(.012,.26,.10),10)
    elif kind=='cupstack':
        for i in range(4):
            a.lathe((0,0,i*.022),[(.024,0),(.035,.085),(.033,.089),(.031,.083),(.023,.008)],s,4,caps=True)
    else:raise ValueError(kind)
    return a


SPECS=[('SM_BarberChair_A','barber','barberchair'),('SM_MirrorFrameBacking_A','barber','mirrorframe'),
       ('SM_HairClipper_A','barber','clipper'),('SM_BarberComb_A','barber','comb'),('SM_SprayBottle_A','barber','spray'),('SM_HairProductBottles_A','barber','hairbottles'),
       ('SM_SewingMachine_A','tailoring','sewingmachine'),('SM_ThreadSpools_A','tailoring','thread'),('SM_TailorScissors_A','tailoring','scissors'),('SM_PressingIron_A','tailoring','iron'),('SM_FabricRolls_A','tailoring','fabricrolls'),('SM_FoldedFabric_A','tailoring','foldedfabric'),
       ('SM_HangingTowel_A','courtyard-laundry','towel'),('SM_HangingSheet_A','courtyard-laundry','laundrysheet'),('SM_HangingShirt_A','courtyard-laundry','shirt'),('SM_ClothesPegs_A','courtyard-laundry','pegs'),('SM_DryingRack_A','courtyard-laundry','dryingrack'),
       ('SM_CookingPot_A','food-preparation','pot'),('SM_PotLid_A','food-preparation','lid'),('SM_ServingLadle_A','food-preparation','ladle'),('SM_MetalFoodTray_A','food-preparation','foodtray'),('SM_CharcoalStove_Cold_A','food-preparation','charcoalstove'),('SM_BreadBasket_Empty_A','food-preparation','breadbasket'),('SM_StackedCups_A','food-preparation','cupstack')]


def main(expected_pid=3224):
    if os.getpid()!=expected_pid or bpy.data.filepath.replace('\\','/')!='C:/dev/baddie-assets/Blender/LagosWorkbench.blend':raise RuntimeError('Workbench identity mismatch')
    if bpy.app.is_job_running('RENDER') or bpy.data.scenes.get(OWNER) or (ROOT/'manifest.json').exists():raise RuntimeError('Busy or existing kit')
    for name in ['textures','fbx','receipts']:(ROOT/name).mkdir(exist_ok=True)
    scene=bpy.data.scenes.new(OWNER);scene.unit_settings.system='METRIC';scene.unit_settings.scale_length=1;scene['detail_owner']=OWNER
    collection=bpy.data.collections.new(OWNER);scene.collection.children.link(collection)
    material,images,provenance=shared['make_atlas']()
    previous=bpy.context.window.scene;records=[]
    try:
        bpy.context.window.scene=scene
        for name,batch,kind in SPECS:
            lods=[]
            for lod in range(3):
                label=name if lod==0 else name+'_LOD'+str(lod)
                obj=build(kind,lod).object(collection,label,material);obj['detail_owner']=OWNER;obj['lod_index']=lod
                if lod==0 and kind not in ('towel','laundrysheet','shirt','pegs','thread','fabricrolls','cupstack'):
                    bevel=obj.modifiers.new('Small contact highlights','BEVEL');bevel.width=.001;bevel.segments=1;bevel.limit_method='ANGLE'
                bpy.context.view_layer.update()
                evaluated=obj.evaluated_get(bpy.context.evaluated_depsgraph_get());mesh=evaluated.to_mesh();mesh.calc_loop_triangles();triangles=len(mesh.loop_triangles);evaluated.to_mesh_clear()
                for other in scene.objects:other.select_set(False)
                obj.select_set(True);bpy.context.view_layer.objects.active=obj
                path=ROOT/'fbx'/f'{label}.fbx'
                with contextlib.redirect_stdout(io.StringIO()):
                    bpy.ops.export_scene.fbx(filepath=str(path),use_selection=True,object_types={'MESH'},global_scale=1,apply_unit_scale=True,apply_scale_options='FBX_SCALE_UNITS',axis_forward='-Y',axis_up='Z',use_mesh_modifiers=True,bake_anim=False,use_custom_props=True,path_mode='STRIP')
                if label.encode() not in path.read_bytes():raise RuntimeError('Export selection mismatch '+label)
                lods.append({'file':str(path.relative_to(ROOT)).replace('\\','/'),'sha256':sha(path),'bytes':path.stat().st_size,'triangles':triangles,'dimensions_metres':[round(float(v),5) for v in obj.dimensions]})
                obj.hide_set(lod>0);obj.hide_render=lod>0
            records.append({'id':name,'batch':batch,'kind':kind,'material':MATERIAL,'collision':'none','nanite_default':False,'lod_screen_sizes':[1,.12,.035],'lods':lods,'pivot':'local contact or attachment origin, metres, Z up'})
    finally:bpy.context.window.scene=previous
    blend=ROOT/'LagosShopWorkV3.blend';bpy.data.libraries.write(str(blend),{scene},path_remap='RELATIVE',fake_user=True,compress=True)
    manifest={'schema_version':1,'kit':OWNER,'status':'exported-unplaced-native-visual-acceptance-pending','blender_version':bpy.app.version_string,'pid':os.getpid(),'blend':blend.name,'blend_sha256':sha(blend),'author_script_sha256':sha(Path(__file__)),'geometry_helper_sha256':sha(HELPER),'assets':records,'textures':[{'file':str(p.relative_to(ROOT)).replace('\\','/'),'sha256':sha(p),'bytes':p.stat().st_size,'srgb':p.name.endswith('_BC.png')} for p in sorted((ROOT/'textures').glob('*.png'))],'provenance':provenance,'performance':{'master_materials':1,'texture_resolution':2048,'cloth_simulation':False,'tick':False,'reflection_capture':False,'measured_fps':None},'acceptance':{'fbx_ids':'inspected','texture':'pending','rendered_geometry':'not-run','unreal_import':'not-run','world_placement':'none'},'workbench_restored':bpy.context.scene==previous}
    (ROOT/'manifest.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')
    return {'assets':len(records),'fbx_count':len(records)*3,'manifest':str(ROOT/'manifest.json'),'workbench_restored':manifest['workbench_restored']}

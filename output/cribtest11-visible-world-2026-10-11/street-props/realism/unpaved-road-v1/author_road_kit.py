"""Owned mesh-recipe consumption: five weights, physical UVs, bounded stills."""
from datetime import datetime, timezone
import hashlib
import json
import math
import os
from pathlib import Path
import random
import runpy
import time
import traceback
import bpy
from mathutils import Vector

OUT=Path('C:/dev/cribtest-game/output/cribtest11-visible-world-2026-10-11/street-props/realism/unpaved-road-v1')
INPUT=OUT.parent/'unpaved-road-source'
MEDIA=OUT.parents[2]/'material-inputs-verified'
FRONTAGE=Path('C:/dev/baddie-assets/YabaRealismKit/frontage-v3-owned/frontage-v3/YabaFrontageV3.blend')
PROBE=Path('C:/Users/cribadmin/.codex/skills/blender-production/scripts/probe_scene.py')
JOB='lagos-unpaved-road-v1'
MATS={}; MATERIALS={}; ASSETS=[]; FRAMES=[]

def digest(p):
    with Path(p).open('rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()

def write(p,v):
    t=p.with_name(p.name+'.tmp');t.write_text(json.dumps(v,indent=2)+'\n',encoding='utf-8');t.replace(p)

def progress(step,**extra):
    r={'job':JOB,'pid':os.getpid(),'step':step,'updated_at':datetime.now(timezone.utc).isoformat(),'output':str(OUT),**extra}
    write(OUT/'progress.json',r);print(json.dumps(r),flush=True)

def constant(name,color,roughness,metal=0,transmission=0):
    mat=bpy.data.materials.new(name);mat.use_nodes=True
    p=mat.node_tree.nodes.get('Principled BSDF')
    for key,value in [('Base Color',(*color,1)),('Roughness',roughness),('Metallic',metal),('Transmission Weight',transmission)]:p.inputs[key].default_value=value
    MATS[name]=mat;MATERIALS[name]={'slot':name,'base_linear':color,'roughness':roughness,'metallic':metal,'transmission':transmission,'provenance':'Explicit original authored constants; not licensed soil scans','coverage_m':None}
    return mat

def scan(name,asset):
    spec=next(m for m in json.loads((MEDIA/'material-manifest.json').read_text())['materials'] if m['id']==asset)
    mat=constant(name,(.3,.3,.3),.82);nodes=mat.node_tree.nodes;links=mat.node_tree.links;p=nodes.get('Principled BSDF')
    uv=nodes.new('ShaderNodeUVMap');uv.uv_map='RoadUV_Metres'
    scale=nodes.new('ShaderNodeVectorMath');scale.operation='MULTIPLY';coverage=spec['coverageMetres']
    scale.inputs[1].default_value=(1/coverage[0],1/coverage[1],1);links.new(uv.outputs['UV'],scale.inputs[0])
    report=MATERIALS[name];report.update(provenance='Coordinator Poly Haven original CC0 scans',authors=spec['authors'],coverage_m=coverage,license='CC0-1.0',maps={})
    for role in ('Diffuse','Rough','nor_gl'):
        info=next(f for f in spec['files'] if f['role']==role);path=MEDIA/info['path']
        if digest(path)!=info['sha256']:raise RuntimeError('Verified map mismatch '+str(path))
        image=bpy.data.images.load(str(path),check_existing=True);image.colorspace_settings.name='sRGB' if role=='Diffuse' else 'Non-Color';image.pack()
        tex=nodes.new('ShaderNodeTexImage');tex.image=image;links.new(scale.outputs['Vector'],tex.inputs['Vector'])
        if role=='Diffuse':links.new(tex.outputs['Color'],p.inputs['Base Color'])
        elif role=='Rough':links.new(tex.outputs['Color'],p.inputs['Roughness'])
        else:
            normal=nodes.new('ShaderNodeNormalMap');normal.uv_map='RoadUV_Metres';normal.inputs['Strength'].default_value=.6
            links.new(tex.outputs['Color'],normal.inputs['Color']);links.new(normal.outputs['Normal'],p.inputs['Normal'])
        report['maps'][role]={'path':str(path),'sha256':info['sha256'],'source':info['source'],'colorspace':image.colorspace_settings.name,'GL_green_invert_once_in_Unreal':role=='nor_gl'}
    return mat

def weighted_material():
    name='Road_FiveSurface_Blend';mat=bpy.data.materials.new(name);mat.use_nodes=True
    nodes,links=mat.node_tree.nodes,mat.node_tree.links;output=nodes.get('Material Output');default=nodes.get('Principled BSDF');nodes.remove(default)
    colors=[(.115,.082,.050),(.40,.32,.20),(.045,.035,.022),(.17,.155,.13)]
    shaders=[];weights=[]
    for idx,key in enumerate(('compacted_earth','loose_sand','damp_soil','gravel','asphalt')):
        attr=nodes.new('ShaderNodeAttribute');attr.attribute_name='RoadWeight_'+key;weights.append(attr.outputs['Fac'])
        p=nodes.new('ShaderNodeBsdfPrincipled');p.label=key
        if idx<4:
            p.inputs['Base Color'].default_value=(*colors[idx],1);p.inputs['Roughness'].default_value=[.92,.94,.57,.91][idx]
        else:
            spec=next(m for m in json.loads((MEDIA/'material-manifest.json').read_text())['materials'] if m['id']=='worn_asphalt')
            uv=nodes.new('ShaderNodeUVMap');uv.uv_map='RoadUV_Metres';scale=nodes.new('ShaderNodeVectorMath');scale.operation='MULTIPLY';scale.inputs[1].default_value=(.5,.5,1)
            links.new(uv.outputs['UV'],scale.inputs[0])
            for role in ('Diffuse','Rough','nor_gl'):
                info=next(f for f in spec['files'] if f['role']==role);path=MEDIA/info['path']
                if digest(path)!=info['sha256']:raise RuntimeError('Asphalt map mismatch')
                im=bpy.data.images.load(str(path),check_existing=True);im.colorspace_settings.name='sRGB' if role=='Diffuse' else 'Non-Color';im.pack()
                tex=nodes.new('ShaderNodeTexImage');tex.image=im;links.new(scale.outputs['Vector'],tex.inputs['Vector'])
                if role=='Diffuse':links.new(tex.outputs['Color'],p.inputs['Base Color'])
                elif role=='Rough':links.new(tex.outputs['Color'],p.inputs['Roughness'])
                else:
                    normal=nodes.new('ShaderNodeNormalMap');normal.uv_map='RoadUV_Metres';normal.inputs['Strength'].default_value=.6
                    links.new(tex.outputs['Color'],normal.inputs['Color']);links.new(normal.outputs['Normal'],p.inputs['Normal'])
        shaders.append(p.outputs['BSDF'])
    # Sequential weighted shader mixture uses normalized conditional sums, all five channels.
    mixed=shaders[0];summed=weights[0]
    for idx in range(1,5):
        add=nodes.new('ShaderNodeMath');add.operation='ADD';links.new(summed,add.inputs[0]);links.new(weights[idx],add.inputs[1])
        denom=nodes.new('ShaderNodeMath');denom.operation='MAXIMUM';denom.inputs[1].default_value=.000001;links.new(add.outputs[0],denom.inputs[0])
        factor=nodes.new('ShaderNodeMath');factor.operation='DIVIDE';links.new(weights[idx],factor.inputs[0]);links.new(denom.outputs[0],factor.inputs[1])
        mix=nodes.new('ShaderNodeMixShader');links.new(factor.outputs[0],mix.inputs[0]);links.new(mixed,mix.inputs[1]);links.new(shaders[idx],mix.inputs[2]);mixed=mix.outputs[0];summed=add.outputs[0]
    links.new(mixed,output.inputs['Surface']);MATS[name]=mat
    MATERIALS[name]={'slot':name,'channels':['compacted_earth','loose_sand','damp_soil','gravel','asphalt'],
        'source':'Five exact recipe point weights, normalized conditional shader mix; asphalt licensed, first four explicitly authored fallback constants',
        'soil_scans':'Not present; no noise/displacement or relabelled asphalt/concrete into soil',
        'soil_color_choices':'Grey/brown compacted earth and pale sand authored, no claimed Lagos-location assignment or laterite scan',
        'roughness':[.92,.94,.57,.91,'verified asphalt roughness'],
        'UV':'RoadUV_Metres stores raw metre coordinates; asphalt shader divides by verified2m exactly once, unknown soil coverage not invented'}

def mesh(scene,name,vertices,faces,mat):
    data=bpy.data.meshes.new(name);data.from_pydata(vertices,[],faces);data.materials.append(MATS[mat]);data.update()
    uv=data.uv_layers.new(name='RoadUV_Metres')
    for poly in data.polygons:
        axis=max(range(3),key=lambda a:abs(poly.normal[a]));axes=[a for a in range(3) if a!=axis]
        for loop in poly.loop_indices:
            p=data.vertices[data.loops[loop].vertex_index].co;uv.data[loop].uv=(p[axes[0]],p[axes[1]])
    obj=bpy.data.objects.new(name,data);scene.collection.objects.link(obj);return obj

def box(scene,name,c,s,mat):
    a,b,d=[n/2 for n in s];x,y,z=c
    verts=[(x+i*a,y+j*b,z+k*d) for i,j,k in [(-1,-1,-1),(1,-1,-1),(1,1,-1),(-1,1,-1),(-1,-1,1),(1,-1,1),(1,1,1),(-1,1,1)]]
    return mesh(scene,name,verts,[(0,3,2,1),(4,5,6,7),(0,1,5,4),(1,2,6,5),(2,3,7,6),(3,0,4,7)],mat)

def measured_bounds(obj):
    points=[obj.matrix_world@Vector(p) for p in obj.bound_box];lo=[min(p[a] for p in points) for a in range(3)];hi=[max(p[a] for p in points) for a in range(3)]
    return {'min_m':lo,'max_m':hi,'dimensions_m':[hi[a]-lo[a] for a in range(3)]}

def road(scene,spec):
    name='SM_Road_'+spec['variant']+'_s%s'%spec['authoring_parameters']['seed']
    obj=mesh(scene,name,spec['vertices'],spec['triangles'],'Road_FiveSurface_Blend');data=obj.data
    for slot in ('Road_CompactedEarth','Road_LooseSand','Road_DampSoil','Road_Gravel','Road_Asphalt'):
        data.materials.append(MATS[slot])
    obj['derivative_id']=spec['id'];obj['canonical_assignment']='none';obj['weights_contract']='weights/'+name+'.json'
    for poly in data.polygons:
        for loop in poly.loop_indices:polyid=data.loops[loop].vertex_index;data.uv_layers[0].data[loop].uv=spec['uv_metres'][polyid]
        if poly.index<spec['top_triangle_count']:poly.use_smooth=True
    for idx,key in enumerate(spec['surface_channels']):
        attr=data.attributes.new('RoadWeight_'+key,'FLOAT','POINT');attr.data.foreach_set('value',[w[idx] for w in spec['surface_weights']])
    wet=data.attributes.new('CandidateWetMask','FLOAT','POINT');wet.data.foreach_set('value',spec['candidate_wet_mask'])
    first=data.color_attributes.new(name='RoadWeights0_EarthSandDampGravel',type='FLOAT_COLOR',domain='CORNER')
    last=data.color_attributes.new(name='RoadWeights1_AsphaltWet',type='FLOAT_COLOR',domain='CORNER')
    for loop in data.loops:
        w=spec['surface_weights'][loop.vertex_index];first.data[loop.index].color=w[:4]
        last.data[loop.index].color=(w[4],spec['candidate_wet_mask'][loop.vertex_index],0,1)
    data.color_attributes.active_color_index=0
    write(OUT/'weights'/(name+'.json'),spec)
    ASSETS.append({'id':spec['id'],'name':name,'variant':spec['variant'],'seed':spec['authoring_parameters']['seed'],
                   'weights':'weights/'+name+'.json','weights_sha256':digest(OUT/'weights'/(name+'.json')),'bounds':measured_bounds(obj),
                   'origin':'Source start join Y0/Zsurface datum; underside below datum by design; not arbitrary ground-shift',
                   'uv':'RoadUV_Metres raw metre coordinates; shader physical division once','material_slots':[m.name for m in data.materials],
                   'attributes':[a.name for a in data.attributes],'join_profile':spec['join_profile']})
    return obj

def clipped_puddle(scene,spec):
    candidates=[i for i in range(spec['top_vertex_count']) if spec['candidate_wet_mask'][i]>.3]
    low=min(candidates,key=lambda i:spec['vertices'][i][2]);cx,cy,zmin=spec['vertices'][low];level=zmin+.012
    vertices=[];faces=[]
    def clip(poly,axis,value,less):
        out=[]
        for p,q in zip(poly,poly[1:]+poly[:1]):
            pin=p[axis]<=value if less else p[axis]>=value;qin=q[axis]<=value if less else q[axis]>=value
            if pin:out.append(p)
            if pin!=qin:
                t=(value-p[axis])/(q[axis]-p[axis]);out.append([p[a]+(q[a]-p[a])*t for a in range(3)])
        return out
    area=0
    for tri in spec['triangles'][:spec['top_triangle_count']]:
        poly=[list(spec['vertices'][i]) for i in tri]
        for axis,value,less in [(2,level,True),(0,cx-.7,False),(0,cx+.7,True),(1,cy-1,False),(1,cy+1,True)]:
            if poly:poly=clip(poly,axis,value,less)
        if len(poly)<3:continue
        start=len(vertices);vertices += [(p[0],p[1],level+.0008) for p in poly];faces.append(tuple(range(start,len(vertices))))
        area+=abs(sum(poly[i][0]*poly[(i+1)%len(poly)][1]-poly[(i+1)%len(poly)][0]*poly[i][1] for i in range(len(poly))))*.5
    if area<=0:raise RuntimeError('No real depression available for clipped puddle')
    obj=mesh(scene,'PreviewOnly_ContourClippedPuddle',vertices,faces,'Road_LocalWater')
    write(OUT/'puddle-inspection.json',{'lowest_candidate_point_m':[cx,cy,zmin],'water_plane_m':level,'max_depth_m':.012,
          'area_m2':area,'geometry':'Triangle clipping to actual below-plane terrain, bounded1.4x2m local area; +.8mm display offset',
          'state':'Separate preview water, NOT conversion of wet mask into full road water; native placement/render/collision unverified'})
    return obj

def context(scene):
    # Additive gallery context from accepted frozen v3 meshes, not altered exported frontage.
    with bpy.data.libraries.load(str(FRONTAGE),link=False) as (available,loaded):loaded.collections=['ShopBay_V3Left','ResidentialBay_V3Left','OpenShopInterior','Window1300_V3Curtain']
    cols={c.name:c for c in loaded.collections if c}
    for name,c in cols.items():
        obj=bpy.data.objects.new('PreviewContext_'+name,None);scene.collection.objects.link(obj);obj.instance_type='COLLECTION';obj.instance_collection=c
        obj.location=(5.02,1.4,3 if name=='ResidentialBay_V3Left' else 4.025 if name=='Window1300_V3Curtain' else 0)
        if name=='OpenShopInterior':obj.location.y+=1.95
        if name=='Window1300_V3Curtain':obj.location.y+=1.3
        obj.rotation_euler.z=math.pi/2
    # Rotated front -X sits at4.985m; returned room/rear on +X, clear of6.2m road.
    for z in (-.10,2.90,5.98):box(scene,'PreviewContext_StoreySlab',(6.48,3.35,z),(3.21,3.9,.20),'Road_ConcreteSupport')
    box(scene,'PreviewContext_RearWall',(8.00,3.35,2.94),(.25,3.9,5.88),'Road_EarthBrown')
    # Clear passage between shoulder X3.1 and drains3.72; no auto canonical placement.
    for y in (1,3,5,7,9):
        box(scene,'PreviewContext_DrainBed',(3.72,y,-.145),(.60,1.98,.08),'Road_ConcreteSupport')
        for x in (3.44,4.00):box(scene,'PreviewContext_DrainWall',(x,y,.02),(.08,1.98,.25),'Road_ConcreteSupport')
    for y in (2,6,8):box(scene,'PreviewContext_DrainCover',(3.72,y,.135),(.58,1.94,.055),'Road_ConcreteSupport')
    # One supported access slab over drain plus sloped threshold path.
    box(scene,'PreviewContext_EntryBridge',(3.72,3.35,.14),(.60,1.2,.10),'Road_ConcreteSupport')
    mesh(scene,'PreviewContext_AccessRamp',[(4.05,2.75,.145),(4.9,2.75,0),(4.9,3.95,0),(4.05,3.95,.145),
         (4.05,2.75,-.06),(4.9,2.75,-.06),(4.9,3.95,-.06),(4.05,3.95,-.06)],
         [(0,1,2,3),(4,7,6,5),(0,4,5,1),(1,5,6,2),(2,6,7,3),(3,7,4,0)],'Road_ConcreteSupport')
    box(scene,'PreviewContext_WalkingStrip',(4.55,6.8,-.055),(.9,5.8,.11),'Road_ConcreteSupport')

def stage(scene,variant):
    scene.unit_settings.system='METRIC';scene.unit_settings.scale_length=1
    scene.render.engine='CYCLES';scene.cycles.device='CPU';scene.cycles.samples=16;scene.cycles.use_denoising=True;scene.cycles.max_bounces=6
    scene.render.threads_mode='FIXED';scene.render.threads=2;scene.render.resolution_x=960;scene.render.resolution_y=540;scene.render.resolution_percentage=100
    scene.render.image_settings.file_format='PNG';scene.render.image_settings.color_mode='RGB';scene.view_settings.view_transform='AgX';scene.view_settings.look='None'
    world=bpy.data.worlds.new('RoadDaylight_'+variant);world.use_nodes=True;world.node_tree.nodes.get('Background').inputs['Color'].default_value=(.62,.70,.8,1);world.node_tree.nodes.get('Background').inputs['Strength'].default_value=.6;scene.world=world
    light=bpy.data.lights.new('RoadSun_'+variant,'SUN');light.energy=2.0;light.angle=math.radians(7)
    obj=bpy.data.objects.new(light.name,light);scene.collection.objects.link(obj);obj.rotation_euler=(.6,-.3,-.6)
    box(scene,'PreviewOnly_SurroundingEarth',(0,5,-.255),(24,28,.12),'Road_EarthBrown')
    context(scene)

def camera(scene,name,eye,target,lens):
    data=bpy.data.cameras.new(name);obj=bpy.data.objects.new(name,data);scene.collection.objects.link(obj)
    obj.location=eye;obj.rotation_euler=(Vector(target)-obj.location).to_track_quat('-Z','Y').to_euler();data.lens=lens;scene.camera=obj

def main():
    if not bpy.app.background or bpy.data.filepath or os.getpid()==3224 or os.environ.get('CRIBTEST_BLENDER_OWNER_JOB')!=JOB:raise RuntimeError('Fresh owned road process required')
    if digest(INPUT/'road_mesh_recipe.py')!='063f9f7f9a7eb1b55cc6a0fd8c285ba8481a16600b16a4a51305cf874205f197':raise RuntimeError('Recipe hash mismatch')
    if digest(INPUT/'HANDOFF.md')!='2dd655b9edb829782adbd55d70ea4914c417b66fd0556f96ffd92801540ec641':raise RuntimeError('Handoff hash mismatch')
    if (OUT/'LagosUnpavedRoadKit.blend').exists():raise RuntimeError('Fresh road output required')
    for name in ('weights','fbx','frames'): (OUT/name).mkdir(exist_ok=False)
    progress('authoring_recipe_modules',source=str(INPUT/'road_mesh_recipe.py'),source_sha256=digest(INPUT/'road_mesh_recipe.py'))
    constant('Road_EarthBrown',(.105,.080,.049),.94)
    constant('Road_CompactedEarth',(.115,.082,.050),.92)
    constant('Road_LooseSand',(.40,.32,.20),.94)
    constant('Road_DampSoil',(.045,.035,.022),.57)
    constant('Road_Gravel',(.17,.155,.13),.91)
    constant('Road_LocalWater',(.93,.97,.98),.025,transmission=1)
    scan('Road_Asphalt','worn_asphalt');scan('Road_ConcreteSupport','concrete_wall_008');weighted_material()
    recipe=runpy.run_path(str(INPUT/'road_mesh_recipe.py'));specs=[];objects=[];scenes=[]
    for variant,seed,name in [('compacted_earth',7,'01-Dry-Earth-Ground'),('sandy_lane',19,'02-Sandy-Lane-Ground'),('rutted_earth',31,'03-Damp-Ruts-Ground'),('paved_to_earth',7,'04-Asphalt-Earth-Join')]:
        scene=bpy.data.scenes.new('Road_'+variant+'_Authoring');bpy.context.window.scene=scene
        spec=recipe['make_road_module'](variant,seed=seed);specs.append(spec);obj=road(scene,spec);objects.append(obj);scenes.append(scene);stage(scene,variant)
        if variant=='rutted_earth':clipped_puddle(scene,spec)
        camera(scene,name,(-2.35,-3.2,1.62),(.2,5,.12),32)
        if variant=='paved_to_earth':
            # Matching source-profile asphalt mouth, not moved existing canonical pavement.
            mouth=recipe['make_road_module']('paved_to_earth',seed=7,length_m=2,max_relief_m=0)
            for vertex in mouth['vertices']:vertex[1]-=2
            objmouth=mesh(scene,'PreviewOnly_MatchedAsphaltMouth',mouth['vertices'],mouth['triangles'],'Road_Asphalt')
            for poly in objmouth.data.polygons:
                for loop in poly.loop_indices:
                    p=objmouth.data.vertices[objmouth.data.loops[loop].vertex_index].co;objmouth.data.uv_layers[0].data[loop].uv=(p.x,p.y)
    library=bpy.data.scenes.new('RoadKit_EditableLibrary');library.unit_settings.system='METRIC';library.unit_settings.scale_length=1
    for idx,obj in enumerate(objects):
        copy=bpy.data.objects.new(obj.name+'_LibraryDisplay',obj.data);library.collection.objects.link(copy);copy.location.x=(idx-1.5)*7
    bpy.context.window.scene=scenes[0];saved=OUT/'LagosUnpavedRoadKit.blend'
    bpy.ops.wm.save_as_mainfile(filepath=str(saved));progress('saved_before_export_render',blend=str(saved),sha256=digest(saved))
    for obj,scene,report in zip(objects,scenes,ASSETS):
        bpy.context.window.scene=scene
        for o in bpy.context.view_layer.objects:o.select_set(False)
        obj.select_set(True);bpy.context.view_layer.objects.active=obj;path=OUT/'fbx'/(obj.name+'.fbx')
        bpy.ops.export_scene.fbx(filepath=str(path),use_selection=True,object_types={'MESH'},global_scale=1,apply_unit_scale=True,
             apply_scale_options='FBX_SCALE_UNITS',axis_forward='-Y',axis_up='Z',bake_anim=False,use_triangles=True,
             use_mesh_modifiers=False,mesh_smooth_type='FACE',colors_type='LINEAR',prioritize_active_color=True,path_mode='RELATIVE')
        report.update(fbx='fbx/'+path.name,fbx_sha256=digest(path))
    bpy.ops.wm.open_mainfile(filepath=str(saved));probes={}
    for name in ('compacted_earth','sandy_lane','rutted_earth','paved_to_earth'):
        scene=bpy.data.scenes['Road_'+name+'_Authoring'];bpy.context.window.scene=scene
        probes[name]=runpy.run_path(str(PROBE))['inspect_scene'](limit=12)
        if probes[name]['assets']['missing_count']:raise RuntimeError('Reopen dependencies missing')
    write(OUT/'scene-probe-reopened.json',probes)
    inspected=[]
    for report,spec in zip(ASSETS,specs):
        obj=bpy.data.objects[report['name']]
        names=[a.name for a in obj.data.attributes]
        if any('RoadWeight_'+key not in names for key in spec['surface_channels']):raise RuntimeError('Saved five weights missing')
        errors=[abs(sum(w)-1) for w in spec['surface_weights']]
        inspected.append({'id':report['id'],'saved_bounds':measured_bounds(obj),'five_float_attributes_present':True,'max_weight_sum_error':max(errors),'color_layers':[a.name for a in obj.data.color_attributes]})
    # In-process Blender FBX roundtrip is separate from actual native Unreal importer basis.
    roundtrip=bpy.data.scenes.new('ReadOnly_FBX_RoundtripInspection');roundtrip.unit_settings.system='METRIC';roundtrip.unit_settings.scale_length=1;bpy.context.window.scene=roundtrip
    imported=[]
    for report in ASSETS:
        before=set(bpy.data.objects);bpy.ops.import_scene.fbx(filepath=str(OUT/report['fbx']),use_custom_normals=True,colors_type='LINEAR')
        obs=[o for o in set(bpy.data.objects)-before if o.type=='MESH']
        imported.append({'id':report['id'],'meshes':[{'name':o.name,'bounds':measured_bounds(o),'uv_layers':[u.name for u in o.data.uv_layers],
              'color_layers':[c.name for c in o.data.color_attributes]} for o in obs],
              'limitation':'Blender FBX importer only; Unreal units/axis/layer retention and materials unverified'})
        for o in set(bpy.data.objects)-before:bpy.data.objects.remove(o,do_unlink=True)
    write(OUT/'asset-inspection.json',{'saved':inspected,'Blender_FBX_roundtrip':imported})
    write(OUT/'material-bindings.json',MATERIALS)
    write(OUT/'road-manifest.json',{'job':JOB,'pid':os.getpid(),'script_sha256':digest(Path(__file__)),'recipe_sha256':digest(INPUT/'road_mesh_recipe.py'),
          'blend':{'path':str(saved),'sha256':digest(saved)},'assets':ASSETS,'units':'metres','axes':{'across':'+X','run':'+Y','up':'+Z'},
          'FBX':{'axis_forward':'-Y','axis_up':'Z','apply_unit_scale':True,'global_scale':1,'apply_scale_options':'FBX_SCALE_UNITS','vertex_colors':'LINEAR, two named corner layers'},
          'five_weights_contract':{'authoritative':'Per-vertex surface_weights arrays with exact topology in weights/*.json; point FLOAT attributes in saved blend',
              'order':list(recipe['SURFACE_CHANNELS']),'FBX_layer0':'RoadWeights0_EarthSandDampGravel RGBA=earth,sand,damp,gravel',
              'FBX_layer1':'RoadWeights1_AsphaltWet R=asphalt G=candidatewet B0 A1; consumer must verify two-layer retention or use authoritative JSON. Never silently drop asphalt.',
              'UV':'RoadUV_Metres raw metre coordinates. Each scanned shader divides by coverage exactly once. NOT frontage predivided SurfaceUV.'},
          'preview_context':{'frontage_blend':str(FRONTAGE),'sha256':digest(FRONTAGE),'state':'Additive isolated partial shop context, unchanged meshes not canonical placement'},
          'known_gaps':['No licensed soil/sand/gravel scans; explicit authored earth fallbacks not photographic fidelity','No dated location-specific road reference or geography assignment',
             'Native five-weight shader/import units,collision,LOD,performance and final visuals unverified','Preview local water clipped; wet mask not water simulation','No traction,dust,flooding or gameplay effects'],
          'errors':[]})
    progress('saved_exported_reopened_five_weights_inspected',blend=str(saved),sha256=digest(saved),assets=ASSETS)
    for variant,name in [('compacted_earth','01-Dry-Earth-Ground'),('sandy_lane','02-Sandy-Lane-Ground'),('rutted_earth','03-Damp-Ruts-Ground'),('paved_to_earth','04-Asphalt-Earth-Join')]:
        scene=bpy.data.scenes['Road_'+variant+'_Authoring'];bpy.context.window.scene=scene;scene.camera=bpy.data.objects[name]
        partial=OUT/'frames'/(name+'.partial.png');target=OUT/'frames'/(name+'.png');scene.render.filepath=str(partial)
        progress('rendering',camera=name,frames=FRAMES,threads=2,samples=16,resolution=[960,540])
        start=time.monotonic();bpy.ops.render.render(write_still=True,scene=scene.name)
        im=bpy.data.images.load(str(partial),check_existing=False)
        if list(im.size)!=[960,540] or not im.has_data:raise RuntimeError('PNG decode/dimension failure')
        bpy.data.images.remove(im);partial.replace(target);FRAMES.append({'path':str(target),'sha256':digest(target),'seconds':time.monotonic()-start})
        write(OUT/'frames.json',FRAMES)
    progress('road_kit_saved_exported_reopened_frames_pending_review',blend=str(saved),sha256=digest(saved),frames=FRAMES,manifest=str(OUT/'road-manifest.json'),errors=[])

if __name__=='__main__':
    try:main()
    except Exception:
        progress('failed',errors=[traceback.format_exc()]);raise

"""Run inside the identified idle Blender workbench; export an isolated library.

No renders, world edits, factory resets or modifications to existing datablocks.
"""
import bpy
import hashlib
import json
import math
import os
from pathlib import Path

import numpy as np

ROOT = Path(__file__).resolve().parent
OWNER = 'LagosSecondaryDetailingV1'


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def png(name, pixels):
    image = bpy.data.images.new(OWNER + '_' + name, width=pixels.shape[1], height=pixels.shape[0], alpha=True)
    image.colorspace_settings.name = 'Non-Color'
    image.pixels.foreach_set(np.flipud(pixels).astype(np.float32).ravel())
    image.file_format = 'PNG'
    image.filepath_raw = str(ROOT / 'textures' / (name + '.png'))
    image.save()
    image.pack()
    return image


def atlas(overlay_file='typography-overlay-r2.png'):
    n = 2048
    yy, xx = np.mgrid[0:n, 0:n].astype(np.float32)
    rng = np.random.default_rng(419)
    noise = rng.random((n, n), dtype=np.float32)
    grain = np.sin(xx * .115 + np.sin(yy * .012) * 4)
    color = np.ones((n, n, 4), dtype=np.float32)
    orm = np.ones_like(color)
    orm[:, :, 2] = 0
    palettes = [(0.34, .29, .23), (.027, .025, .023), (.045, .040, .034), (.34, .32, .27),
                (.61, .68, .68), (.65, .11, .065), (.77, .73, .62), (.72, .66, .74),
                (.72, .68, .61), (.71, .66, .53), (.69, .64, .50), (.11, .25, .22),
                (.50, .28, .13), (.055, .050, .043), (.15, .23, .095), (.39, .19, .075)]
    rough = [.82, .90, .52, .93, .40, .50, .89, .88, .91, .88, .91, .76, .98, .98, .76, .91]
    for tile, base in enumerate(palettes):
        row, col = divmod(tile, 4)
        sl = np.s_[row*512:(row+1)*512, col*512:(col+1)*512]
        variation = (noise[sl] - .5) * .055
        if tile == 0:
            variation += grain[sl] * .055
        color[sl][:, :, :3] = np.clip(np.array(base) + variation[:, :, None], 0, 1)
        orm[sl][:, :, 1] = np.clip(rough[tile] + (noise[sl] - .5) * .07, 0, 1)
    # Reuse the locally supplied licensed timber input, not an unverified download.
    source_root = ROOT.parent / 'material-inputs-verified'
    source_manifest = json.loads((source_root / 'material-manifest.json').read_text())
    wood = next(m for m in source_manifest['materials'] if m['id'] == 'wood_planks_grey')
    diffuse = next(f for f in wood['files'] if f['role'] == 'Diffuse')
    wood_path = source_root / diffuse['path']
    if digest(wood_path) != diffuse['sha256']:
        raise RuntimeError('Timber input hash mismatch')
    image = bpy.data.images.load(str(wood_path), check_existing=False)
    data = np.empty(len(image.pixels), dtype=np.float32)
    image.pixels.foreach_get(data)
    data = np.flipud(data.reshape(image.size[1], image.size[0], 4))
    indices_y = np.linspace(0, image.size[1]-1, 512).astype(int)
    indices_x = np.linspace(0, image.size[0]-1, 512).astype(int)
    sample = data[indices_y[:, None], indices_x[None, :], :3]
    # Blender image API returns linear RGB; atlas PNG stores color in sRGB.
    sample = np.where(sample <= .0031308, sample * 12.92, 1.055 * np.maximum(sample, 0)**(1/2.4) - .055)
    color[:512, :512, :3] = np.clip(sample, 0, 1)
    bpy.data.images.remove(image)
    overlay = bpy.data.images.load(str(ROOT / 'textures' / overlay_file), check_existing=False)
    data = np.empty(len(overlay.pixels), dtype=np.float32)
    overlay.pixels.foreach_get(data)
    ink = np.flipud(data.reshape(n, n, 4))
    rgb = np.where(ink[:, :, :3] <= .0031308, ink[:, :, :3] * 12.92,
                   1.055 * np.maximum(ink[:, :, :3], 0)**(1/2.4) - .055)
    alpha = ink[:, :, 3:4]
    color[:, :, :3] = color[:, :, :3] * (1-alpha) + rgb * alpha
    bpy.data.images.remove(overlay)
    # Paper masks use authored irregular borders with safe tile padding.
    for tile in range(6, 12):
        row, col = divmod(tile, 4)
        y, x = np.mgrid[0:512, 0:512]
        edge = 32 + 4*np.sin(y*.10) + 2*np.sin(y*.31)
        cut = (x > edge) & (x < 512-edge) & (y > 32+3*np.sin(x*.17)) & (y < 480+3*np.sin(x*.13))
        color[row*512:(row+1)*512, col*512:(col+1)*512, 3] = cut.astype(float)
        if tile in (9,10):
            # Spray warnings are ink silhouettes, not beige paper rectangles.
            color[row*512:(row+1)*512, col*512:(col+1)*512, 3] *= ink[row*512:(row+1)*512, col*512:(col+1)*512, 3]
            color[row*512:(row+1)*512, col*512:(col+1)*512, :3] = (.10,.075,.055)
    base = png('T_DetailAtlas_BC', color)
    base.colorspace_settings.name = 'sRGB'
    packed = png('T_DetailAtlas_ORM', orm)
    # Separate grayscale masks packed RGBA: dust, soot, algae, rust.
    m = 1024
    y, x = np.mgrid[0:m, 0:m].astype(np.float32) / (m-1)
    rng=np.random.default_rng(420)
    fine = rng.random((m, m), dtype=np.float32)
    def noise_field(cells):
        grid=rng.random((cells+1,cells+1),dtype=np.float32)
        a=np.linspace(0,cells-.0001,m)
        i=a.astype(int);t=a-i;t=t*t*(3-2*t)
        top=grid[i[:,None],i[None,:]]*(1-t[None,:])+grid[i[:,None],i[None,:]+1]*t[None,:]
        bot=grid[i[:,None]+1,i[None,:]]*(1-t[None,:])+grid[i[:,None]+1,i[None,:]+1]*t[None,:]
        return top*(1-t[:,None])+bot*t[:,None]
    soft=.50*noise_field(5)+.27*noise_field(13)+.15*noise_field(37)+.08*noise_field(89)
    mask = np.zeros((m, m, 4), dtype=np.float32)
    mask[:, :, 0] = np.clip((1-y*1.45 + (soft-.5)*.5), 0, 1) * (.8+.2*fine)
    mask[:, :, 1] = np.exp(-((x-.5+(soft-.5)*.1)/(0.07+y*.25))**2) * np.clip((1-y)*1.3, 0, 1) * (.55+.45*soft)
    mask[:, :, 2] = np.clip((soft-.36)*3, 0, 1) * np.clip(1-y*1.25+(soft-.5)*.3, 0, 1)
    mask[:, :, 3] = np.clip((soft-.43)*4, 0, 1) * (.75+.25*fine)
    masks = png('T_WeatherMasks_RGBA', mask)
    return base, packed, masks, {'id': wood['id'], 'license': source_manifest['license'], 'authors': wood['authors'],
                                'source_path': str(wood_path), 'source_sha256': diffuse['sha256'],
                                'source_manifest_sha256': digest(source_root / 'material-manifest.json')}


def material(name, base, packed, masked=False):
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    bsdf = mat.node_tree.nodes.get('Principled BSDF')
    tex = mat.node_tree.nodes.new('ShaderNodeTexImage')
    tex.image = base
    orm = mat.node_tree.nodes.new('ShaderNodeTexImage')
    orm.image = packed
    split = mat.node_tree.nodes.new('ShaderNodeSeparateColor')
    mat.node_tree.links.new(tex.outputs['Color'], bsdf.inputs['Base Color'])
    mat.node_tree.links.new(orm.outputs['Color'], split.inputs['Color'])
    mat.node_tree.links.new(split.outputs['Green'], bsdf.inputs['Roughness'])
    mat.node_tree.links.new(split.outputs['Blue'], bsdf.inputs['Metallic'])
    if masked:
        mat.node_tree.links.new(tex.outputs['Alpha'], bsdf.inputs['Alpha'])
    mat['unreal_blend'] = 'MASKED_TWO_SIDED' if masked else 'OPAQUE'
    return mat


def mesh_object(collection, name, verts, faces, tiles, mat):
    mesh = bpy.data.meshes.new(name + '_Mesh')
    mesh.from_pydata(verts, [], faces)
    mesh.update()
    obj = bpy.data.objects.new(name, mesh)
    collection.objects.link(obj)
    mesh.materials.append(mat)
    uv = mesh.uv_layers.new(name='UV0')
    # Atlas box projection per face, with 24px gutters. Paper grids carry a
    # full-surface mapping below rather than repeating text on each face.
    for poly in mesh.polygons:
        tile = tiles[poly.index] if isinstance(tiles, list) else tiles
        row, col = divmod(tile, 4)
        axis = max(range(3), key=lambda k: abs(poly.normal[k]))
        axes = [k for k in range(3) if k != axis]
        coords = [mesh.vertices[v].co for v in poly.vertices]
        lo = [min(c[k] for c in coords) for k in axes]
        hi = [max(c[k] for c in coords) for k in axes]
        for loop_index, co in zip(poly.loop_indices, coords):
            u, v = [(co[k]-lo[j])/max(hi[j]-lo[j], 1e-6) for j,k in enumerate(axes)]
            uv.data[loop_index].uv = ((col+(24+u*464)/512)/4, 1-(row+(24+v*464)/512)/4)
    obj['detail_owner'] = OWNER
    return obj


def box(collection, name, size, tile, mat):
    x, y, z = size
    verts = [(sx*x/2, sy*y/2, sz*z) for sz in [0,1] for sy in [-1,1] for sx in [-1,1]]
    faces = [(0,2,3,1),(4,5,7,6),(0,1,5,4),(2,6,7,3),(0,4,6,2),(1,3,7,5)]
    return mesh_object(collection, name, verts, faces, tile, mat)


def sheet(collection, name, size, tile, mat, resolution, seed, vertical=False):
    rng = np.random.default_rng(seed)
    w, h = size
    verts, faces = [], []
    for j in range(resolution+1):
        for i in range(resolution+1):
            x, y = (i/resolution-.5)*w, (j/resolution-.5)*h
            z = .001 + .006*(1+math.sin(i*1.7+j*.9)) + float(rng.uniform(0,.003))
            if vertical:
                verts.append((x, z, y+h/2))
            else:
                verts.append((x, y, z))
    for j in range(resolution):
        for i in range(resolution):
            a=j*(resolution+1)+i
            faces.append((a,a+1,a+resolution+2,a+resolution+1))
    obj = mesh_object(collection, name, verts, faces, tile, mat)
    uv = obj.data.uv_layers.active
    row, col = divmod(tile,4)
    for poly in obj.data.polygons:
        for li in poly.loop_indices:
            vi=obj.data.loops[li].vertex_index
            j,i=divmod(vi,resolution+1)
            uv.data[li].uv=((col+(24+i/resolution*464)/512)/4,1-(row+(24+(1-j/resolution)*464)/512)/4)
    return obj


def tire(collection, name, segments, rings, mat):
    verts, faces = [], []
    # Tire lies flat: visible hollow centre, sidewalls and tread.
    for j in range(rings):
        v=2*math.pi*j/rings
        for i in range(segments):
            u=2*math.pi*i/segments
            r=.255+.085*math.cos(v)
            tread=.0015*math.sin(i*2.3) if abs(math.cos(v))>.7 else 0
            verts.append(((r+tread)*math.cos(u),(r+tread)*math.sin(u),.085+.080*math.sin(v)))
    for j in range(rings):
        for i in range(segments):
            faces.append((j*segments+i,j*segments+(i+1)%segments,((j+1)%rings)*segments+(i+1)%segments,((j+1)%rings)*segments+i))
    obj=mesh_object(collection,name,verts,faces,1,mat)
    for p in obj.data.polygons: p.use_smooth=True
    return obj


def bag(collection, name, segments, layers, mat, seed):
    rng=np.random.default_rng(seed)
    verts,faces=[],[]
    for j in range(layers):
        t=j/(layers-1)
        radius=.11*math.sin(math.pi*(.07+.88*t))+.007
        for i in range(segments):
            a=i*2*math.pi/segments
            r=radius*(1+.1*math.sin(i*2+j)+float(rng.uniform(-.04,.04)))
            verts.append((r*math.cos(a),r*.8*math.sin(a),t*.19+.006))
    for j in range(layers-1):
        for i in range(segments):
            faces.append((j*segments+i,j*segments+(i+1)%segments,(j+1)*segments+(i+1)%segments,(j+1)*segments+i))
    faces.extend([tuple(reversed(range(segments))),tuple((layers-1)*segments+i for i in range(segments))])
    obj=mesh_object(collection,name,verts,faces,2,mat)
    for p in obj.data.polygons:p.use_smooth=True
    return obj


def stone(collection,name,segments,mat,seed):
    rng=np.random.default_rng(seed)
    verts=[]
    for j in range(3):
        for i in range(segments):
            a=2*math.pi*i/segments
            r=(.09 if j in [0,2] else .14)*(1+float(rng.uniform(-.17,.17)))
            verts.append((r*math.cos(a),r*.72*math.sin(a),j*.045))
    faces=[tuple(reversed(range(segments))),tuple(2*segments+i for i in range(segments))]
    for j in range(2):
        for i in range(segments):faces.append((j*segments+i,j*segments+(i+1)%segments,(j+1)*segments+(i+1)%segments,(j+1)*segments+i))
    return mesh_object(collection,name,verts,faces,3,mat)


def cable(collection,name,segments,sides,mat):
    verts,faces=[],[]
    for i in range(segments+1):
        t=i/segments
        for j in range(sides):
            a=2*math.pi*j/sides
            verts.append((t*8,.008*math.cos(a),1-.85*4*t*(1-t)+.008*math.sin(a)))
    for i in range(segments):
        for j in range(sides):faces.append((i*sides+j,i*sides+(j+1)%sides,(i+1)*sides+(j+1)%sides,(i+1)*sides+j))
    faces.extend([tuple(reversed(range(sides))),tuple(segments*sides+j for j in range(sides))])
    return mesh_object(collection,name,verts,faces,1,mat)


def main(expected_pid=3224):
    if os.getpid()!=expected_pid or bpy.data.filepath.replace('\\','/')!='C:/dev/baddie-assets/Blender/LagosWorkbench.blend':
        raise RuntimeError('Workbench identity mismatch')
    if bpy.app.is_job_running('RENDER') or bpy.data.scenes.get(OWNER):
        raise RuntimeError('Busy session or existing owned scene')
    if (ROOT/'manifest.json').exists():raise RuntimeError('Immutable kit already exists')
    for directory in ['textures','fbx','receipts']:(ROOT/directory).mkdir(exist_ok=True)
    before={'file':bpy.data.filepath,'scene':bpy.context.scene.name,'objects':[o.name for o in bpy.context.scene.objects]}
    scene=bpy.data.scenes.new(OWNER)
    scene.unit_settings.system='METRIC'
    scene.unit_settings.scale_length=1
    scene['detail_owner']=OWNER
    collection=bpy.data.collections.new(OWNER)
    scene.collection.children.link(collection)
    base,orm,masks,wood=atlas()
    opaque=material('M_LagosDetail_Opaque',base,orm)
    paper=material('M_LagosDetail_Paper',base,orm,True)
    records=[]
    specs=[('SM_WaterSachet_A','litter','sachet',4,419),('SM_WaterSachet_B','litter','sachet',4,423),
           ('SM_SausageWrapper_A','litter','wrapper',5,424),('SM_NylonBag_A','litter','bag',2,425),('SM_NylonBag_B','litter','bag',2,426),
           ('SM_Tire_A','infrastructure','tire',1,427),('SM_PlankShort_A','infrastructure','plank',0,428),
           ('SM_GutterPlanks_A','infrastructure','crossing',0,429),('SM_RoofStone_A','infrastructure','stone',3,430),
           ('SM_RoofStone_B','infrastructure','stone',3,431),('SM_CableSpan8m_A','infrastructure','cable',1,432),
           ('SM_PosterCampaign_A','decals','poster',6,433),('SM_PosterChurch_A','decals','poster',7,434),
           ('SM_PosterObituary_A','decals','poster',8,435),('SM_Warning419_A','decals','poster',9,436),
           ('SM_PostNoBill_A','decals','poster',10,437),('SM_ShopSignRepairs_A','decals','poster',11,438)]
    previous_scene=bpy.context.window.scene
    bpy.context.window.scene=scene
    try:
        for name,batch,kind,tile,seed in specs:
            lods=[]
            for lod in range(3):
                label=name if lod==0 else f'{name}_LOD{lod}'
                mat=paper if kind=='poster' else opaque
                if kind in ['sachet','wrapper','poster']:
                    size=(.135,.18) if kind=='sachet' else ((.07,.18) if kind=='wrapper' else (.43,.60))
                    obj=sheet(collection,label,size,tile,mat,[8,4,1][lod],seed,kind=='poster')
                    if kind!='poster':
                        solid=obj.modifiers.new('Film thickness','SOLIDIFY');solid.thickness=.0004
                elif kind=='bag':obj=bag(collection,label,[16,10,6][lod],[7,5,4][lod],mat,seed)
                elif kind=='tire':obj=tire(collection,label,[48,24,12][lod],[12,8,6][lod],mat)
                elif kind=='stone':obj=stone(collection,label,[14,9,6][lod],mat,seed)
                elif kind=='cable':obj=cable(collection,label,[24,12,6][lod],[6,4,3][lod],mat)
                else:
                    obj=box(collection,label,(.22,1.25,.035) if kind=='plank' else (.68,1.25,.045),0,mat)
                    bevel=obj.modifiers.new('Worn edge radius','BEVEL');bevel.width=.0025;bevel.segments=1
                    if kind=='crossing':
                        # Three supported boards with gaps, not a sealed drain slab.
                        bpy.data.objects.remove(obj,do_unlink=True)
                        verts,faces=[],[]
                        for index in range(3):
                            part=box(collection,label+'_board',( .21,1.25,.045),0,mat)
                            offset=(index-1)*.235
                            start=len(verts)
                            verts.extend([(v.co.x+offset,v.co.y,v.co.z) for v in part.data.vertices])
                            faces.extend([tuple(start+i for i in p.vertices) for p in part.data.polygons])
                            bpy.data.objects.remove(part,do_unlink=True)
                        obj=mesh_object(collection,label,verts,faces,0,mat)
                obj['asset_id']=name
                obj['lod_index']=lod
                scene.view_layers[0].update()
                dg=bpy.context.evaluated_depsgraph_get()
                evaluated=obj.evaluated_get(dg)
                mesh=evaluated.to_mesh()
                mesh.calc_loop_triangles()
                triangles=len(mesh.loop_triangles)
                evaluated.to_mesh_clear()
                for existing in scene.objects:existing.select_set(False)
                obj.select_set(True)
                scene.view_layers[0].objects.active=obj
                collision=None
                if kind=='crossing' and lod==0:
                    collision=box(collection,'UCX_'+name+'_00',(.68,1.25,.045),0,mat)
                    collision.select_set(True)
                path=ROOT/'fbx'/f'{label}.fbx'
                bpy.ops.export_scene.fbx(filepath=str(path),use_selection=True,object_types={'MESH'},global_scale=1,
                                         apply_unit_scale=True,apply_scale_options='FBX_SCALE_UNITS',axis_forward='-Y',axis_up='Z',
                                         use_mesh_modifiers=True,bake_anim=False,use_custom_props=True,path_mode='STRIP')
                if collision:bpy.data.objects.remove(collision,do_unlink=True)
                lods.append({'file':str(path.relative_to(ROOT)).replace('\\','/'),'sha256':digest(path),'triangles':triangles,
                             'dimensions_metres':[round(float(v),5) for v in obj.dimensions]})
                obj.hide_render=lod>0
                obj.hide_set(lod>0)
            records.append({'id':name,'batch':batch,'lods':lods,'material':'M_LagosDetail_Paper' if kind=='poster' else 'M_LagosDetail_Opaque',
                            'collision':'UCX-approved-crossing-only' if kind=='crossing' else 'none',
                            'lod_screen_sizes':[1.0,.12,.035], 'pivot':'contact plane; local origin; metres; Z up',
                            'atlas_tile':tile,'nanite_default':False})
    finally:
        bpy.context.window.scene=previous_scene
    # Library-only scene: no loaded world is changed or saved.
    blend=ROOT/'LagosSecondaryDetailingV1.blend'
    bpy.data.libraries.write(str(blend),{scene,masks},path_remap='RELATIVE',fake_user=True,compress=True)
    textures=[{'file':str(p.relative_to(ROOT)).replace('\\','/'),'sha256':digest(p),'bytes':p.stat().st_size,
                'srgb':p.name=='T_DetailAtlas_BC.png'} for p in sorted((ROOT/'textures').glob('T_*.png'))]
    manifest={'schema_version':1,'kit':OWNER,'status':'exported-unplaced-native-acceptance-pending',
              'blender_version':bpy.app.version_string,'pid':os.getpid(),'author_script_sha256':digest(Path(__file__)),
              'blend':blend.name,'blend_sha256':digest(blend),'coordinate_contract':{'units':'metres','fbx_axis_forward':'-Y','fbx_axis_up':'Z','unreal_import_convert_scene':True,'unreal_import_convert_scene_unit':True},
              'assets':records,'textures':textures,'timber_provenance':wood,
              'performance':{'materials':2,'shared_color_orm_atlas_size':2048,'weather_mask_size':1024,'lod_count':3,'measured_fps':None},
              'workbench_before':before,'workbench_after':{'file':bpy.data.filepath,'scene':bpy.context.scene.name,'objects':[o.name for o in bpy.context.scene.objects]},
              'acceptance':{'unreal_import':'not-run','rendered_geometry':'not-run','atlas_inspection':'pending','niagara':'handoff-only','world_placement':'none'}}
    (ROOT/'manifest.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')
    return {'manifest':str(ROOT/'manifest.json'),'assets':len(records),'fbx_count':len(records)*3,'blend':str(blend),'workbench_unchanged':manifest['workbench_before']==manifest['workbench_after']}


if __name__=='__main__':
    result=main()

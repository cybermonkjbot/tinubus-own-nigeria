"""Bounded artifact inspection in the existing Blender session; no renders."""
import hashlib
import json
from pathlib import Path

import bmesh
import bpy
from io_scene_fbx import parse_fbx

ROOT=Path(__file__).resolve().parent


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def inspect():
    manifest=json.loads((ROOT/'manifest.json').read_text(encoding='utf-8'))
    original_scene=bpy.context.scene
    records=[]
    totals=[0,0,0]
    for asset in manifest['assets']:
        for lod,record in enumerate(asset['lods']):
            path=ROOT/record['file']
            if sha(path)!=record['sha256']:raise RuntimeError('FBX hash mismatch: '+str(path))
            tree,version=parse_fbx.parse(str(path))
            objects=next(e for e in tree.elems if e.id==b'Objects')
            models=[e for e in objects.elems if e.id==b'Model']
            geometry=[e for e in objects.elems if e.id==b'Geometry' and e.props[-1]==b'Mesh']
            label=asset['id'] if lod==0 else asset['id']+'_LOD'+str(lod)
            if len(models)!=1 or len(geometry)!=1 or label.encode() not in models[0].props[1]:
                raise RuntimeError('FBX object mismatch: '+label)
            indices=next(e.props[0] for e in geometry[0].elems if e.id==b'PolygonVertexIndex')
            count=0;triangles=0
            for index in indices:
                count+=1
                if index<0:triangles+=count-2;count=0
            if count or triangles!=record['triangles']:raise RuntimeError('FBX topology count mismatch: '+label)
            obj=bpy.data.scenes[manifest['kit']].objects.get(label)
            bm=bmesh.new();bm.from_mesh(obj.data)
            boundary=sum(e.is_boundary for e in bm.edges)
            nonmanifold=sum(not e.is_manifold for e in bm.edges)
            zero_area=sum(f.calc_area()<1e-12 for f in bm.faces)
            bm.free()
            if boundary or nonmanifold or zero_area:raise RuntimeError('Invalid shell topology: '+label)
            if len(obj.data.materials)!=1 or len(obj.data.uv_layers)!=1:raise RuntimeError('Material/UV mismatch: '+label)
            totals[lod]+=triangles
            records.append({'id':label,'fbx_version':version,'triangles':triangles,'boundary_edges':boundary,
                            'nonmanifold_edges':nonmanifold,'zero_area_faces':zero_area})
    for record in manifest['textures']:
        if sha(ROOT/record['file'])!=record['sha256']:raise RuntimeError('Texture hash mismatch')
    blend=ROOT/manifest['blend']
    if sha(blend)!=manifest['blend_sha256']:raise RuntimeError('Blend hash mismatch')
    with bpy.data.libraries.load(str(blend),link=True) as (source,target):
        catalog={'scenes':list(source.scenes),'objects':len(source.objects),'materials':len(source.materials),'images':len(source.images)}
        target.scenes=list(source.scenes)
    scene=target.scenes[0]
    images={node.image for obj in scene.objects for material in obj.data.materials
            for node in material.node_tree.nodes if node.type=='TEX_IMAGE' and node.image}
    packed=[{'size':list(image.size),'packed':image.packed_file is not None,
             'color_space':image.colorspace_settings.name} for image in images]
    if len(scene.objects)!=60 or len(images)!=2 or not all(i['packed'] for i in packed):
        raise RuntimeError('Saved library dependency mismatch')
    receipt={'status':'artifact-inspection-complete-native-visual-acceptance-pending',
             'manifest_sha256':sha(ROOT/'manifest.json'),'assets':len(manifest['assets']),'fbx':len(records),
             'total_triangles_by_lod':totals,'saved_library':catalog,'packed_images':packed,
             'workbench_scene_unchanged':bpy.context.scene==original_scene,'files':records,
             'limitations':['No rendered geometry review','No v5 Unreal import','No in-world placement',
                            'No target-device performance measurement','No self-intersection or UV-distortion acceptance']}
    (ROOT/'receipts/preparation.json').write_text(json.dumps(receipt,indent=2),encoding='utf-8')
    return {k:v for k,v in receipt.items() if k!='files'}

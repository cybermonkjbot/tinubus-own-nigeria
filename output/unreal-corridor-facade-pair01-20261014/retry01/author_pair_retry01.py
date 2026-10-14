"""One isolated facade composition; immutable inputs, no engine operations/export."""
import hashlib
import json
import os
import math
import struct
from pathlib import Path
import time
import bpy
import bmesh
from mathutils import Vector

OUT = Path(__file__).resolve().parent
ROOT = Path('C:/dev/cribtest-game')
FAILED = OUT.parent
assert OUT == ROOT/'output/unreal-corridor-facade-pair01-20261014/retry01'
A = ROOT/'output/unreal-corridor-facade-variant01-20261014'
B = ROOT/'output/unreal-corridor-facade-variant02-20261014'
INPUT_A = A/'front-contact-finish01/CorridorFacadeVariant01_FrontContactFinish01.blend'
INPUT_B = B/'CorridorFacadeVariant02.blend'
KIT = ROOT/'output/cribtest11-visible-world-2026-10-11/parent-realism-kit/kit-contract.json'
PINS = {INPUT_A:'dce613cebc5efed46930e98aedb3aab7f13d13619c44650b27b5659f6611ffa5',
        INPUT_B:'6e8c340d0050f6c01e4f1b04f5ba81a4d1b62c37a7f9e27f386cf57a7488da4e',
        KIT:'37314e3e16633b7f5edc9b2c11e105561813bfe2e6c3c1bed6ba942b04f991ff'}
JOB = 'corridor-facade-pair01'
STRUCTURE = {'Ground_TrueShopAperture','Upper_TrueWindowAperture','GroundFloorSupport',
             'ForecourtSupport','UpperFloorSlab','RoofSlab','RoofCoping',
             'SectionalReturn','SectionalReturn.001'}

def sha(p):
    return hashlib.sha256(p.read_bytes()).hexdigest()

def write(name, value):
    with (OUT/name).open('x', encoding='utf-8') as f:
        json.dump(value, f, indent=2, allow_nan=False)

def identity():
    return {'host':os.environ.get('COMPUTERNAME'),'pid':os.getpid(),'file':bpy.data.filepath,
            'scene':bpy.context.scene.name,'engine':bpy.context.scene.render.engine,
            'version':bpy.app.version_string,'job':JOB,'renderJob':bpy.app.is_job_running('RENDER')}

def hashes():
    return {str(p.relative_to(ROOT)):sha(p) for base in (A,B,FAILED) for p in base.rglob('*') if p.is_file() and OUT not in p.parents}

def geometry(o, offset=0):
    bpy.context.view_layer.update()
    e = o.evaluated_get(bpy.context.evaluated_depsgraph_get())
    m = e.to_mesh()
    verts = [e.matrix_world@v.co-Vector((offset,0,0)) for v in m.vertices]
    bm = bmesh.new()
    bm.from_mesh(m)
    remaining = set(bm.verts)
    components = 0
    while remaining:
        stack = [remaining.pop()]
        components += 1
        while stack:
            v = stack.pop()
            for edge in v.link_edges:
                n = edge.other_vert(v)
                if n in remaining:
                    remaining.remove(n)
                    stack.append(n)
    result = {'vertices':[list(v) for v in verts],
              'faces':[{'v':list(p.vertices),'normal':list(p.normal),'material':p.material_index} for p in m.polygons],
              'bounds':[[min(v[j] for v in verts) for j in range(3)],
                        [max(v[j] for v in verts) for j in range(3)]],
              'materials':[mat.name for mat in o.data.materials],
              'components':components,'manifold':all(e.is_manifold for e in bm.edges),
              'winding':all(e.is_contiguous for e in bm.edges),'volume':bm.calc_volume(signed=True)}
    bm.free()
    e.to_mesh_clear()
    return result

def pin(row):
    # Translation-normalized float32 coordinates need tolerance, not exact float hashes.
    return {'vertices':[[round(c,5) for c in v] for v in row['vertices']],
            'faces':row['faces'],'materials':row['materials']}

def wait(name):
    deadline = time.monotonic()+1200
    while not (OUT/name).exists():
        if time.monotonic()>deadline:
            raise RuntimeError('Owned inspection timeout; preserve output')
        time.sleep(2)
    value = json.loads((OUT/name).read_text())
    assert value['job']==JOB
    return value

assert bpy.app.background and bpy.app.version[:3]==(5,2,2)
assert Path(bpy.data.filepath).resolve()==INPUT_A.resolve()
assert not any(OUT.glob('*.blend')) and not any(OUT.glob('*.png'))
assert all(sha(p)==h for p,h in PINS.items())
originals = hashes()
assert len(originals)==99
write('preservation-before.json', originals)
write('input-identity.json', identity())
scene = bpy.context.scene
assert scene.name=='CorridorFacadeVariant01' and scene.cycles.device=='CPU' and scene.cycles.samples==24
asset_a = bpy.data.collections['Asset_Facade_OriginXYZ0']
source_a = {o.name:geometry(o) for o in asset_a.objects}
assert STRUCTURE <= source_a.keys()
with bpy.data.libraries.load(str(INPUT_B), link=False) as (src,dst):
    dst.collections = [n for n in src.collections if n in ('Asset_Facade_OriginXYZ0','Variant02_ShadeAndAwning')]
loaded = dst.collections
assert len(loaded)==2
for c in loaded:
    scene.collection.children.link(c)
asset_b = next(c for c in loaded if c.name.startswith('Asset_Facade_OriginXYZ0'))
shade_b = next(c for c in loaded if c.name.startswith('Variant02_ShadeAndAwning'))
# Appending gives suffixes: resolve exact original names from their observed library copies.
def original_name(o):
    if o.name.startswith(tuple(n.split('.')[0] for n in STRUCTURE)):
        bounds=geometry(o)['bounds']
        matches=[n for n in STRUCTURE if all(abs(bounds[i][j]-source_a[n]['bounds'][i][j])<1e-5
                 for i in range(2) for j in range(3))]
        assert len(matches)==1, (o.name,matches)
        return matches[0]
    return o.name

mapping = {original_name(o):o for o in asset_b.objects}
assert STRUCTURE <= mapping.keys()
source_b = {n:geometry(o) for n,o in mapping.items()}
write('source-structure.json', {'A':{n:source_a[n] for n in STRUCTURE},
                               'B':{n:source_b[n] for n in STRUCTURE},
                               'observedBNames':{n:o.name for n,o in mapping.items()}})
# Matching appended material slots may have .001 suffixes but retain actual datablocks.

def local_state(o):
    bpy.context.view_layer.update()
    e=o.evaluated_get(bpy.context.evaluated_depsgraph_get())
    m=e.to_mesh()
    def mesh_state(m):
        return {'vertices':[list(v.co) for v in m.vertices],
                'edges':[list(x.vertices) for x in m.edges],
                'faces':[{'v':list(p.vertices),'normal':list(p.normal),'material':p.material_index} for p in m.polygons]}
    mods=[]
    for mod in o.modifiers:
        fields={}
        for prop in mod.bl_rna.properties:
            if prop.identifier=='rna_type' or prop.is_readonly or prop.type in ('POINTER','COLLECTION'):
                continue
            value=getattr(mod,prop.identifier)
            fields[prop.identifier]=list(value) if prop.is_array else value
        mods.append(fields)
    row={'raw':mesh_state(o.data),'evaluated':mesh_state(m),
         'materials':[{'name':mat.name,'library':mat.library.filepath if mat.library else None} for mat in o.data.materials],
         'modifiers':mods,'matrix':[list(r) for r in e.matrix_world],
         'world':[list(e.matrix_world@v.co) for v in m.vertices],
         'parent':o.parent.name if o.parent else None}
    e.to_mesh_clear()
    return row

# IEEE754 binary32 round-to-nearest u=2^-24; 32 elementary operations bound
# covers two affine evaluations, one location addition and normalization.
# Declared before observations; discrete/local fields remain EXACT.
UNIT_ROUNDOFF=2**-24
GAMMA32=32*UNIT_ROUNDOFF/(1-32*UNIT_ROUNDOFF)
write('precision-policy.json',{'u':UNIT_ROUNDOFF,'gamma32':GAMMA32,
      'formula':'gamma32 * (sum(abs(matrixRow[j]*local[j])) + abs(matrixTranslation) + abs(offset) + 1)',
      'scope':'World-coordinate arithmetic only; raw/evaluated local topology/normals/materials/modifiers exact',
      'noRoundedHashAcceptance':True})
diagnostics=[]
first_legacy=None
def translation_gate(o, offset, before):
    global first_legacy
    after=local_state(o)
    exact={key:before[key]==after[key] for key in ('raw','evaluated','materials','modifiers','parent')}
    matrix_deltas=[]
    for i in range(4):
        for j in range(4):
            expected=before['matrix'][i][j]+(offset if i==0 and j==3 else 0)
            delta=abs(after['matrix'][i][j]-expected)
            bound=UNIT_ROUNDOFF*(abs(expected)+abs(offset)+1) if i==0 and j==3 else 0
            matrix_deltas.append({'row':i,'column':j,'delta':delta,'bound':bound})
    deltas=[]
    for local,a,b in zip(before['evaluated']['vertices'],before['world'],after['world']):
        for axis in range(3):
            expected=a[axis]+(offset if axis==0 else 0)
            scale=sum(abs(before['matrix'][axis][j]*local[j]) for j in range(3))+abs(before['matrix'][axis][3])+abs(offset)+1
            deltas.append({'axis':axis,'delta':abs(b[axis]-expected),'bound':GAMMA32*scale})
    legacy_before={'vertices':[[round(c,5) for c in v] for v in before['world']],
                   'faces':before['evaluated']['faces'],'materials':before['materials']}
    legacy_after={'vertices':[[round(c,5) for c in (Vector(v)-Vector((offset,0,0)))] for v in after['world']],
                  'faces':after['evaluated']['faces'],'materials':after['materials']}
    differing=[k for k in before if before[k]!=after[k]]
    item={'object':o.name,'offsetM':offset,'allDifferentFields':differing,'exactLocalFields':exact,
          'matrixDeltas':matrix_deltas,'worldDeltaCount':sum(d['delta']>0 for d in deltas),
          'maxWorldDeltaM':max(d['delta'] for d in deltas),'maxPredeclaredBoundM':max(d['bound'] for d in deltas),
          'legacyPass':legacy_before==legacy_after,
          'legacyDifferentFields':[k for k in legacy_before if legacy_before[k]!=legacy_after[k]]}
    if not item['legacyPass'] and first_legacy is None:
        first_legacy=o.name
        write('first-legacy-mismatch.json',{'summary':item,'fullBefore':before,'fullAfter':after})
    diagnostics.append(item)
    if not all(exact.values()) or not all(d['delta']<=d['bound'] for d in matrix_deltas+deltas):
        write('invariant-failure.json',{'summary':item,'fullBefore':before,'fullAfter':after})
        raise RuntimeError('Protected translation invariant failed: '+o.name)
    return after

def preserve_check(o, expected):
    actual=local_state(o)
    assert actual==expected, 'Saved/current exact protected state mismatch: '+o.name

protected = {}
for o in list(asset_a.objects):
    if o.name not in STRUCTURE or o.name=='SectionalReturn':
        protected[o.name] = local_state(o)
for n,o in mapping.items():
    if n not in STRUCTURE or n=='SectionalReturn.001':
        before=local_state(o)
        o.name = 'BayB_'+n
        o.location.x += 3.9
        translation_gate(o,3.9,before)
        protected[o.name] = local_state(o)
for o in shade_b.objects:
    before=local_state(o)
    o.location.x += 3.9
    translation_gate(o,3.9,before)
    protected[o.name] = local_state(o)
write('translation-diagnosis.json',{'objects':diagnostics,'firstLegacyFailure':first_legacy,
      'cause':'Legacy rounded world normalization differs while exact local state and bounded intended translation are preserved' if first_legacy else 'Legacy failure not reproduced; no inferred historical cause',
      'allLocalDiscreteFieldsExact':True})
b_threshold=next(o.name for n,o in mapping.items() if n.startswith('ThresholdStone'))
b_outer=mapping['SectionalReturn.001'].name
removed = []
for o in list(asset_a.objects):
    if o.name in STRUCTURE and o.name!='SectionalReturn':
        removed.append(o.name)
        bpy.data.objects.remove(o,do_unlink=True)
for n,o in mapping.items():
    if n in STRUCTURE and n!='SectionalReturn.001':
        removed.append(o.name)
        bpy.data.objects.remove(o,do_unlink=True)
scene.name = 'CorridorFacadePair01'
owned = bpy.data.collections.new('Pair01_ContinuousStructure')
scene.collection.children.link(owned)

def solid(name, xs, ys, zs, inside, materials, bevel=0):
    cells = {(i,j,k) for i in range(len(xs)-1) for j in range(len(ys)-1) for k in range(len(zs)-1)
             if inside((xs[i]+xs[i+1])/2,(ys[j]+ys[j+1])/2,(zs[k]+zs[k+1])/2)}
    verts, faces, indices, lookup = [],[],[],{}
    for i,j,k in sorted(cells):
        x0,x1,y0,y1,z0,z1 = xs[i],xs[i+1],ys[j],ys[j+1],zs[k],zs[k+1]
        sides = [((-1,0,0),[(x0,y0,z0),(x0,y0,z1),(x0,y1,z1),(x0,y1,z0)]),
                 ((1,0,0),[(x1,y0,z0),(x1,y1,z0),(x1,y1,z1),(x1,y0,z1)]),
                 ((0,-1,0),[(x0,y0,z0),(x1,y0,z0),(x1,y0,z1),(x0,y0,z1)]),
                 ((0,1,0),[(x0,y1,z0),(x0,y1,z1),(x1,y1,z1),(x1,y1,z0)]),
                 ((0,0,-1),[(x0,y0,z0),(x0,y1,z0),(x1,y1,z0),(x1,y0,z0)]),
                 ((0,0,1),[(x0,y0,z1),(x1,y0,z1),(x1,y1,z1),(x0,y1,z1)])]
        for delta, coords in sides:
            if (i+delta[0],j+delta[1],k+delta[2]) in cells:
                continue
            ids = []
            for co in coords:
                if co not in lookup:
                    lookup[co]=len(verts)
                    verts.append(co)
                ids.append(lookup[co])
            faces.append(ids)
            indices.append(0 if (x0+x1)/2<3.9 else min(1,len(materials)-1))
    m = bpy.data.meshes.new(name)
    m.from_pydata(verts,[],faces)
    o = bpy.data.objects.new(name,m)
    owned.objects.link(o)
    for mat in materials:
        m.materials.append(mat)
    for p,index in zip(m.polygons,indices):
        p.material_index=index
    if bevel:
        mod=o.modifiers.new('RetainedExternalEdgeRadius','BEVEL')
        mod.width=bevel
        mod.segments=2
    return o

concrete = bpy.data.materials['CastConcrete']
green = bpy.data.materials['MineralGreenPaint']
warm = bpy.data.materials['WarmMaintainedPlaster']
zmid = source_a['UpperFloorSlab']['bounds'][0][2]
zroof = source_a['RoofSlab']['bounds'][0][2]
for name,z0,z1,holes,mat in (
    ('Pair_GroundWall',0,zmid,[(.55,3.35,0,2.5),(4.45,7.25,0,2.5)],green),
    ('Pair_UpperWall',3,zroof,[(1.3,2.6,4.025,5.375),(5.2,6.5,4.025,5.375)],warm)):
    xs=sorted({0,3.9,7.8,*[h[i] for h in holes for i in (0,1)]})
    zs=sorted({z0,z1,*[h[i] for h in holes for i in (2,3)]})
    solid(name,xs,[-.25,0],zs,
          lambda x,y,z,holes=holes:not any(a<x<b and c<z<d for a,b,c,d in holes),[mat])
for n in ('UpperFloorSlab','RoofSlab','RoofCoping'):
    lo,hi=source_a[n]['bounds']
    solid('Pair_'+n,[lo[0],hi[0]+3.9],[lo[1],hi[1]],[lo[2],hi[2]],lambda x,y,z:True,[concrete],.003)
glo,ghi=source_a['GroundFloorSupport']['bounds']
flo,fhi=source_a['ForecourtSupport']['bounds']
solid('Pair_ContinuousGround',[0,3.9,7.8],sorted({glo[1],flo[1],ghi[1],fhi[1]}),
      sorted({glo[2],flo[2],0}),
      lambda x,y,z:(glo[1]<y<ghi[1] and glo[2]<z<0) or (flo[1]<y<fhi[1] and flo[2]<z<0),[concrete],.003)
for floor,lo,hi in (('Ground',0,zmid),('Upper',3,zroof)):
    solid('Pair_PartySpine_'+floor,[3.775,4.025],[-.9,-.25],[lo,hi],lambda x,y,z:True,[warm])

# Actual evaluated convex face patches: intersection in their common contact plane.
def clip(poly, boundary):
    area=sum(boundary[i][0]*boundary[(i+1)%len(boundary)][1]-boundary[(i+1)%len(boundary)][0]*boundary[i][1] for i in range(len(boundary)))
    sign=1 if area>=0 else -1
    for a,b in zip(boundary,boundary[1:]+boundary[:1]):
        def distance(p):
            return sign*((b[0]-a[0])*(p[1]-a[1])-(b[1]-a[1])*(p[0]-a[0]))
        result=[]
        if not poly:
            break
        for p,q in zip(poly,poly[1:]+poly[:1]):
            dp,dq=distance(p),distance(q)
            if dp>=-1e-9:
                result.append(p)
            if (dp<0)!=(dq<0):
                t=dp/(dp-dq)
                result.append([p[j]+t*(q[j]-p[j]) for j in range(2)])
        poly=result
    return poly

def contact(a,b,axis,plane):
    ra,rb=geometry(a),geometry(b)
    patches=[]
    dims=[i for i in range(3) if i!=axis]
    for pa in ra['faces']:
        va=[ra['vertices'][i] for i in pa['v']]
        if abs(abs(pa['normal'][axis])-1)>1e-5 or max(abs(v[axis]-plane) for v in va)>1e-5:
            continue
        for pb in rb['faces']:
            vb=[rb['vertices'][i] for i in pb['v']]
            if pa['normal'][axis]*pb['normal'][axis]>-.999 or max(abs(v[axis]-plane) for v in vb)>1e-5:
                continue
            poly=clip([[v[j] for j in dims] for v in va],[[v[j] for j in dims] for v in vb])
            area=abs(sum(poly[i][0]*poly[(i+1)%len(poly)][1]-poly[(i+1)%len(poly)][0]*poly[i][1] for i in range(len(poly))))/2 if poly else 0
            if area>1e-9:
                patches.append({'areaM2':area,'polygon2D':poly,'signedGapM':vb[0][axis]-va[0][axis],
                                'normals':[pa['normal'],pb['normal']]})
    return {'objects':[a.name,b.name],'axis':axis,'planeM':plane,'patches':patches,
            'totalOpposingAreaM2':sum(p['areaM2'] for p in patches)}

def verify():
    for n,row in protected.items():
        preserve_check(bpy.data.objects[n],row)
    rows={o.name:geometry(o) for o in owned.objects}
    assert all(r['manifold'] and r['winding'] and r['volume']>0 and r['components']==1 for r in rows.values())
    pairs=[('Pair_GroundWall','Pair_UpperFloorSlab',2,zmid),
           ('Pair_UpperWall','Pair_UpperFloorSlab',2,3),('Pair_UpperWall','Pair_RoofSlab',2,zroof),
           ('Pair_PartySpine_Ground','Pair_UpperFloorSlab',2,zmid),
           ('Pair_PartySpine_Upper','Pair_UpperFloorSlab',2,3),
           ('Pair_PartySpine_Upper','Pair_RoofSlab',2,zroof),
           ('Pair_PartySpine_Ground','Pair_GroundWall',1,-.25),
           ('Pair_PartySpine_Upper','Pair_UpperWall',1,-.25),
           ('Pair_GroundWall','Pair_ContinuousGround',2,0),
           ('ThresholdStone','Pair_ContinuousGround',2,0),(b_threshold,'Pair_ContinuousGround',2,0),
           ('SectionalReturn','Pair_GroundWall',1,-.25),(b_outer,'Pair_GroundWall',1,-.25),
           ('SectionalReturn','Pair_UpperWall',1,-.25),(b_outer,'Pair_UpperWall',1,-.25)]
    contacts=[contact(bpy.data.objects[a],bpy.data.objects[b],axis,plane) for a,b,axis,plane in pairs]
    assert all(c['totalOpposingAreaM2']>0 and all(abs(p['signedGapM'])<1e-5 for p in c['patches']) for c in contacts)
    for n in ('Pair_GroundWall','Pair_UpperWall','Pair_ContinuousGround'):
        r=rows[n]
        assert not any(all(abs(r['vertices'][i][0]-3.9)<1e-5 for i in f['v']) for f in r['faces'])
    shade=[geometry(o)['bounds'][0][2] for o in shade_b.objects if 'Shade' in o.name]
    drip=geometry(next(o for o in shade_b.objects if 'FoldedSheet' in o.name))['bounds'][0][2]
    return {'continuousStructure':rows,'opposingContacts':contacts,'noInternalCentralSeamFaces':True,
            'widthM':7.8,'partyThicknessM':.25,'partyCentreXM':3.9,
            'preservedBayBShadeClearanceM':min(shade)-5.375,'preservedBayBDripClearanceM':drip-2.5,
            'aperturesM':{'shops':[[.55,3.35,0,2.5],[4.45,7.25,0,2.5]],
                          'windows':[[1.3,2.6,4.025,5.375],[5.2,6.5,4.025,5.375]]},
            'scope':'Convex evaluated contact polygons; no enclosing multipart AABB solid proof. Existing shade assembly welded overlaps retained.'}

readback=verify()
write('contact-readback.json',readback)
write('protected-details.json',{'exactCurrentStates':protected,'removedCopiedStructuralNames':removed})
write('authored-identity.json',identity())
poses={'front':((3.9,16,3.2),(3.9,0,3)),
       'grazing':((-6,11,4.2),(3.9,0,3)),
       'reverse':((11,-12,4.5),(3.9,-.2,3)),
       'player165':((8,10,1.65),(3.9,0,2.55)),
       'seam':((6.9,4.6,2.3),(3.9,-.1,1.9))}
camera=scene.camera
def render(name):
    path=OUT/(name+'.png')
    assert not path.exists()
    camera.location=poses[name][0]
    camera.rotation_euler=(Vector(poses[name][1])-camera.location).to_track_quat('-Z','Y').to_euler()
    camera.data.lens=22.23 if name=='player165' else 42
    scene.render.resolution_x=scene.render.resolution_y=800
    scene.render.resolution_percentage=100
    scene.render.threads_mode='FIXED'
    scene.render.threads=6
    scene.render.filepath=str(path)
    bpy.ops.render.render(write_still=True)

render('front')
write('preview-ready.json',identity())
assert wait('finish.json')['action']=='finish'
child=OUT/'CorridorFacadePair01.blend'
assert not child.exists()
bpy.ops.wm.save_as_mainfile(filepath=str(child))
saved=identity()
shade_name=shade_b.name
bpy.ops.wm.open_mainfile(filepath=str(child))
scene=bpy.context.scene
camera=scene.camera
owned=bpy.data.collections['Pair01_ContinuousStructure']
shade_b=bpy.data.collections[shade_name]
assert Path(bpy.data.filepath).resolve()==child.resolve() and scene.name=='CorridorFacadePair01'
assert verify()==readback
write('reopened-identity.json',{'before':saved,'after':identity(),'blendSHA256':sha(child),'readbackIdentical':True})
for name in ('grazing','reverse','player165','seam'):
    render(name)
write('all-stills-ready.json',identity())
assert wait('close-job.json')['action']=='close'
assert hashes()==originals
write('preservation-after.json',{'all99Unchanged':True,'hashes':hashes()})
write('asset-contract.json',{'kind':'editable-continuous-kit-composition-unplaced','units':'metres',
      'origin':[0,0,0],'axes':{'run':'+X','street':'+Y','up':'+Z'},'widthM':7.8,
      'bayTranslationM':[3.9,0,0],'storeyZM':[0,3,6],'wallM':.25,
      'sourcePins':{str(p.relative_to(ROOT)):h for p,h in PINS.items()},
      'authoredJoin':'One centered 250mm party spine split only at retained slab bands; continuous walls/slabs/coping/support union',
      'materials':'Source finishes retained; contrast provided by original solid canopy and bronze-blade/charcoal pitched shade assemblies',
      'poses':poses,'renderer':'CyclesCPU24samples6threadsAgX800square',
      'limits':['Source-derived not surveyed','No canonical assignment/rooms/stock/signage/gameplay doors',
                'No Unreal import/collision/navigation/coverage/performance acceptance','Kit contract remains external untracked provenance']})
print('PAIR01_COMPLETE',os.getpid(),flush=True)



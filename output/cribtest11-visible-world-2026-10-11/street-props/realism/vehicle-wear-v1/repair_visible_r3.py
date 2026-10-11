"""One image-led, owned repair; no frozen source or canonical placement writes."""
import hashlib
import json
import math
import os
from pathlib import Path
import bpy
from mathutils import Vector

ROOT = Path('C:/dev/cribtest-game/output/cribtest11-visible-world-2026-10-11/street-props/realism/vehicle-wear-v1')

def repair(vehicles, mats, mesh, box, slab, tube, assign):
    if os.getpid() != 13664 or not bpy.app.background or os.getpid() == 3224:
        raise RuntimeError('Repair must execute in reviewed owned process 13664')
    g = mesh.__globals__
    material_evidence = {
        'actual_saved_names': sorted(m.name for m in bpy.data.materials if m.name.startswith('VW_')),
        'declared_but_not_saved': [k for k, r in g['MAT_REPORT'].items() if not bpy.data.materials.get(r['slot'])],
        'glass_edge_film_source_assignments': 0,
        'cause': 'Unused palette entries have no fake user; omitted on save. No differently named retained GlassEdgeFilm.',
    }
    (ROOT/'saved-material-inspection-r3.json').write_text(json.dumps(material_evidence, indent=2)+'\n')

    def delete(v, obj):
        v['parts'].remove(obj)
        bpy.data.objects.remove(obj, do_unlink=True)

    def shell(v, name, rows, mat, gauge=.0014):
        n = len(rows[0]); verts = [p for row in rows for p in row]
        faces = [(j*n+i,j*n+i+1,(j+1)*n+i+1,(j+1)*n+i)
                 for j in range(len(rows)-1) for i in range(n-1)]
        obj = mesh(v,name,verts,faces,mat,smooth=True)
        mod = obj.modifiers.new('ActualSheetGauge','SOLIDIFY'); mod.thickness = gauge
        return obj

    for v in vehicles[:2]:
        bpy.context.window.scene = v['scene']
        broken = 'Breakdown' in v['id']
        for obj in list(v['parts']):
            role = obj.get('part_role','')
            if (role == 'Danfo_Roof' or role.startswith(('Danfo_RoofPress_', 'Danfo_RoofRail_', 'Danfo_Headlamp'))
                or role == 'Danfo_Bumper_2.21' or role.startswith('BumperAbrasion_')
                or (not broken and role in ('Danfo_FrontPanel','Danfo_Grille'))):
                delete(v,obj)

        # A thin crowned sheet replaces the 150mm slab. Gutter remains below its edge.
        def crown(x,y):
            return 2.079 + .064*(1-(x/.946)**2) - .013*(abs((y+.09)/2.06)**8)
        rows=[]
        for j in range(49):
            y=-2.15+j*4.13/48
            rows.append([(x,y,crown(x,y)) for x in [-.946+i*1.892/32 for i in range(33)]])
        shell(v,'CrownedRoofSheet',rows,'YellowBleached' if broken else 'YellowFaded',.0016)
        for side in (-1,1):
            tube(v,'RoofGutter_%s'%side,[(side*.942,y,2.059) for y in (-2.13,-1.3,0,1.2,1.98)],.009,'YellowRepair',10)
            shell(v,'RoofHeader_%s'%side,
                  [[(side*.925,y,2.001),(side*.925,y,crown(side*.925,y)-.003)]
                   for y in [-2.14+j*4.11/40 for j in range(41)]],'YellowFaded',.002)
        for y in (-2.15,1.98):
            tube(v,'RoofRolledEnd_%s'%y,[(x,y,crown(x,y)-.003) for x in [-.94+i*1.88/40 for i in range(41)]],.009,'YellowFaded',10)
            shell(v,'RoofEndHeader_%s'%y,
                  [[(x,y,2.001),(x,y,crown(x,y)-.003)]
                   for x in [-.925+j*1.85/40 for j in range(41)]],'YellowFaded',.002)
        for x in (-.59,-.29,.29,.59):
            tube(v,'ShallowRoofPress_%s'%x,[(x,y,crown(x,y)+.003) for y in (-1.92,-1.1,0,1,1.76)],.0035,'YellowFaded',8)

        # Recesses are real openings backed by reflector bowls and a grille cavity.
        if not broken:
            verts=[]; faces=[]; nx,nz=96,36
            for j in range(nz+1):
                z=.595+j*.69/nz
                for i in range(nx+1):
                    x=-.938+i*1.876/nx
                    y=2.151+.027*(1-(x/.938)**2)+.009*math.sin((z-.595)/.69*math.pi)
                    verts.append((x,y,z))
                    if i and j:
                        cx=x-.938/nx; cz=z-.345/nz
                        lamp=any((cx-lx)**2+(cz-1.035)**2<.126**2 for lx in (-.64,.64))
                        grille=abs(cx)<.364 and .873<cz<1.068
                        if not lamp and not grille:
                            faces.append(((j-1)*(nx+1)+i-1,(j-1)*(nx+1)+i,j*(nx+1)+i,j*(nx+1)+i-1))
            panel=mesh(v,'CrownedFrontFascia',verts,faces,'YellowMaintained',smooth=True)
            panel.modifiers.new('FasciaGauge','SOLIDIFY').thickness=.0014
            box(v,'GrilleRecess',(0,2.141,.97),(.75,.012,.21),'BlackStripe',.003)
            for j in range(7):
                box(v,'GrilleLouvre_%s'%j,(0,2.199,.884+j*.028),(.711,.014,.008),'RimOld',.002)
            for x in (-.29,.29):
                box(v,'GrilleInnerSupport_%s'%x,(x,2.18,.97),(.009,.02,.187),'BlackStripe',.001)

        for lx in (-.64,.64):
            circle=lambda r,y:[(lx+r*math.cos(a*math.tau/64),y,1.035+r*math.sin(a*math.tau/64)) for a in range(65)]
            tube(v,'HeadlampGasket_%s'%lx,circle(.134,2.219),.008,'Rubber',10)
            tube(v,'HeadlampRolledRim_%s'%lx,circle(.1245,2.23),.005,'Steel',10)
            rows=[]
            for r,y in [(.005,2.177),(.032,2.18),(.063,2.19),(.093,2.205),(.120,2.22)]:
                rows.append(circle(r,y))
            shell(v,'LampReflectorBowl_%s'%lx,rows,'Mirror',.0008)
            # Closed 2mm lens, only 3mm crown: not a pale solid cylinder/sphere.
            vv=[]; ff=[]; rings=9; seg=64
            for back in (False,True):
                for j in range(rings):
                    r=.001+(.119-.001)*j/(rings-1)
                    y=2.232+.003*(1-(r/.119)**2)-( .002 if back else 0)
                    vv += [(lx+r*math.cos(a*math.tau/seg),y,1.035+r*math.sin(a*math.tau/seg)) for a in range(seg)]
            for back in (0,1):
                base=back*rings*seg
                for j in range(rings-1):
                    for a in range(seg):
                        p=base+j*seg+a;q=base+j*seg+(a+1)%seg
                        ff.append((p,q,q+seg,p+seg))
                ff.append(tuple(base+a for a in range(seg)))
            for a in range(seg):
                p=(rings-1)*seg+a;q=(rings-1)*seg+(a+1)%seg
                ff.append((p,q,q+rings*seg,p+rings*seg))
            mesh(v,'ClearHeadlampLens_%s'%lx,vv,ff,'GlassClear',smooth=True)
            for j in range(-7,8):
                dx=j*.014;h=math.sqrt(.113**2-dx**2)
                tube(v,'LensOptic_%s_%s'%(lx,j),[(lx+dx,2.236,1.035-h),(lx+dx,2.236,1.035+h)],.0008,'GlassClear',6)
            tube(v,'BulbSocket_%s'%lx,[(lx,2.181,1.035),(lx,2.203,1.035)],.011,'Steel',12)

        # Rolled channel with returned ends, separate insert/bolts and handling wear.
        verts=[];faces=[];count=49
        profile=[(-.034,-.043),(.025,-.037),(.030,-.025),(.030,.026),(.025,.039),(-.032,.044),(-.032,.041),(.021,.036),(.027,.024),(.027,-.024),(.021,-.034),(-.034,-.040)]
        for i in range(count):
            x=-.967+i*1.934/(count-1);off=-.165*(abs(x)/.967)**8
            for dy,dz in profile:verts.append((x,2.267+off+dy,.565+dz))
            if i:
                n=len(profile)
                faces += [((i-1)*n+j,(i-1)*n+(j+1)%n,i*n+(j+1)%n,i*n+j) for j in range(n)]
        n=len(profile);faces += [tuple(reversed(range(n))),tuple((count-1)*n+j for j in range(n))]
        mesh(v,'RolledFrontBumper',verts,faces,'RimOld',smooth=True)
        for lo,hi in [(-.88,-.34),(-.30,.28),(.34,.77)]:
            tube(v,'BumperInsert_%s'%lo,[(x,2.30-.165*(abs(x)/.967)**8,.57) for x in [lo+(hi-lo)*j/16 for j in range(17)]],.009,'Rubber',8)
        for x in (-.60,-.27,.27,.60):
            tube(v,'BumperBolt_%s'%x,[(x,2.300,.548),(x,2.305,.548)],.006,'Steel',6)
        for i in range(9):
            x=.55+i*.026;y=2.298-.165*(abs(x)/.967)**8
            tube(v,'BumperScuff_%s'%i,[(x,y,.583),(x+.032,y-.003,.580)],.0013,'Primer',5)

        for obj in v['parts']:
            role=obj.get('part_role','')
            if role.startswith('Danfo_SidePanel_'):
                side=-1 if role.endswith('_-1') else 1
                for vert in obj.data.vertices:
                    p=vert.co
                    original_dent=(.035 if broken else .019)*math.exp(-((p.y+.58)/.24)**2-((p.z-.80)/.15)**2)
                    p.x+=side*original_dent
                    dent=.010*math.exp(-((p.y+.64)/.13)**2-((p.z-.81)/.09)**2)
                    dent+=.006*math.exp(-((p.y-.93)/.11)**2-((p.z-.68)/.065)**2)
                    p.x-=side*dent
                obj.data.update()
                for poly in obj.data.polygons:poly.use_smooth=abs(poly.normal.x)>.75
            if role in ('Danfo_FrontPillar_-0.88','Danfo_FrontPillar_0.88'):
                for poly in obj.data.polygons:poly.use_smooth=True
        v['notes'].append('Image-led r3: thin crowned roof, shaped/recessed fascia, clear shallow glass lens/reflector assembly, rolled worn bumper, smaller smooth local dents; original characteristic danfo proportions retained')

    # Actual sedan view shows a broad bonnet/side slot; close the measured panel interface.
    v=vehicles[2];bpy.context.window.scene=v['scene']
    for side in (-1,1):
        for start,end in [(.67,2.25),(-2.25,-1.17)]:
            rows=[]
            for j in range(33):
                y=start+(end-start)*j/32;w=.85-.115*(abs(y)/2.25)**3
                z=.88-.065*(max(0,abs(y)-1.1)/1.15)
                rows.append([(side*(w-.026),y,z),(side*w*.967,y,z+.020+.032*(1-.967**2))])
            shell(v,'DeckShoulder_%s_%s'%(side,start),rows,'SedanFaded',.0012)
    v['notes'].append('Observed bonnet/wing slot closed with gauge-thickness shoulder sheets; older sedan still angular and not a verified branded model')

    # FBX channel zero must be the physically pre-scaled SurfaceUV, not old helper UVMap.
    for v in vehicles:
        for obj in v['parts']:
            if not obj.data.uv_layers.get('SurfaceUV'):raise RuntimeError('Missing physical UV on '+obj.name)
            for layer in list(obj.data.uv_layers):
                if layer.name!='SurfaceUV':obj.data.uv_layers.remove(layer)
            obj.data.uv_layers.active_index=0;obj.data.uv_layers[0].active_render=True

    retained={k:r for k,r in g['MAT_REPORT'].items() if k in mats}
    (ROOT/'material-bindings-r3.json').write_text(json.dumps(retained,indent=2)+'\n')
    original_save=g['save_delivery']
    def inspected_save(folder):
        result=original_save(folder)
        path=folder/'vehicle-manifest.json';manifest=json.loads(path.read_text())
        manifest['material_bindings']='../material-bindings-r3.json'
        manifest['uv']='FBX channel0 is SurfaceUV only; physical coverage predivided; native multiplier1x'
        manifest['execution_sources']={p.name:hashlib.file_digest(p.open('rb'),'sha256').hexdigest() for p in [ROOT/'author_vehicles.py',ROOT/'resume_saved_r2.py',Path(__file__)]}
        for report,v in zip(manifest['assets'],vehicles):
            saved_by_name={o.name:o for o in v['parts']}
            for record in report['parts']:
                obj=saved_by_name[record['name']]
                record['material_slots']=[m.name for m in obj.data.materials if m]
                record['dimensions_m']=list(obj.dimensions)
        manifest['visual_acceptance']='Actual initial views reviewed; largest visible danfo defect repaired. Final images pending review; no realism/native import acceptance.'
        path.write_text(json.dumps(manifest,indent=2)+'\n')
        return result
    g['save_delivery']=inspected_save
    # Resume module holds its function separately; update it through caller locals.
    import inspect
    caller=inspect.currentframe().f_back
    caller.f_globals['module']['save_delivery']=inspected_save

import json, pathlib, sys
repo = pathlib.Path(r"C:\dev\cribtest-game")
sys.path.insert(0, str(repo / "apps" / "companion-unreal" / "Scripts"))
from WorldGeneration.canonical import References, city_plan, destinations
refs = References(); plan = city_plan(refs); places = destinations(refs)
arch = json.loads((repo / '.git/operations/world-generation-20261010/readonly-source/packages/assets/city/lagos-architecture-delivery-v1.json').read_text(encoding='utf-8-sig'))
parcels = {p['id']: p for p in plan['parcels']}
d = next(d for d in plan['districts'] if d['id']=='town-centre')
parcel_ids = [p['id'] for p in plan['parcels'] if p['district']=='town-centre']
sigs = []
for b in arch['buildings']:
    pid=b['parcelId']
    if pid in parcel_ids:
        sigs.append({'job': 'city/signatures/'+pid, 'parcelId': pid, 'name': parcels[pid]['name'], 'style': parcels[pid]['style']})
roads = []
min_x = min(parcels[pid]['x']-parcels[pid]['width']/2 for pid in parcel_ids)
max_x = max(parcels[pid]['x']+parcels[pid]['width']/2 for pid in parcel_ids)
min_z = min(parcels[pid]['z']-parcels[pid]['depth']/2 for pid in parcel_ids)
max_z = max(parcels[pid]['z']+parcels[pid]['depth']/2 for pid in parcel_ids)
for road in plan['roads']:
    if any(min_x - 40 <= pt[0] <= max_x + 40 and min_z - 40 <= pt[1] <= max_z + 40 for pt in road['points']):
        roads.append({'id': road['id'], 'name': road['name'], 'kind': road['kind'], 'points': len(road['points'])})
# keep major connective roads then alphabetical rest
major = [r for r in roads if r['kind'] in ('avenue','boulevard','quay','street','bridge','market','service','residential')]
out = {
    'district': d,
    'bounds': {'minX':min_x,'maxX':max_x,'minZ':min_z,'maxZ':max_z},
    'parcelCount': len(parcel_ids),
    'signatureJobs': sigs,
    'majorRoads': major,
    'destinations': [p for p in places if p.get('city',{}).get('parcelId') in parcel_ids],
}
print(json.dumps(out, indent=2))

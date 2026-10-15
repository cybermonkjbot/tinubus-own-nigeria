import json, pathlib, math, re, sys
repo = pathlib.Path(r"C:\dev\cribtest-game")
sys.path.insert(0, str(repo / "apps" / "companion-unreal" / "Scripts"))
from WorldGeneration.canonical import References, city_plan, destinations
refs = References()
plan = city_plan(refs)
places = destinations(refs)
# map destinations to districts where possible
parcel_by_id = {p['id']: p for p in plan['parcels']}
summary = []
for district in plan['districts']:
    did = district['id']
    parcels = [p for p in plan['parcels'] if p['district'] == did]
    # district bounds from parcels
    min_x = min(p['x'] - p['width']/2 for p in parcels)
    max_x = max(p['x'] + p['width']/2 for p in parcels)
    min_z = min(p['z'] - p['depth']/2 for p in parcels)
    max_z = max(p['z'] + p['depth']/2 for p in parcels)
    road_matches = []
    for road in plan['roads']:
        pts = road['points']
        inside = any(min_x - 40 <= pt[0] <= max_x + 40 and min_z - 40 <= pt[1] <= max_z + 40 for pt in pts)
        if inside:
            road_matches.append({'id': road['id'], 'name': road['name'], 'kind': road['kind']})
    dest_matches = []
    for place in places:
        city = place.get('city')
        if city and city.get('parcelId') in parcel_by_id and parcel_by_id[city['parcelId']]['district'] == did:
            dest_matches.append({'id': place['id'], 'label': place['label'], 'kind': place['kind'], 'parcelId': city['parcelId']})
    summary.append({
        'id': did,
        'label': district['label'],
        'bounds': {'minX': round(min_x,2), 'maxX': round(max_x,2), 'minZ': round(min_z,2), 'maxZ': round(max_z,2)},
        'parcelCount': len(parcels),
        'parcelSample': [p['id'] for p in parcels[:12]],
        'roadCountApprox': len(road_matches),
        'roadSample': road_matches[:20],
        'destinationCount': len(dest_matches),
        'destinations': dest_matches,
    })
print(json.dumps(summary, indent=2))

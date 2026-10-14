"""Pinned Unity Avenue contact proposals only. No Unreal import or output writes."""
import hashlib
import json
import math
from pathlib import Path

MAP = '/Game/BaddieGeneratedWorld/StreetFinish_20261014/Maps/L_TownCentre_LibraryLane'
SOURCE_COMMIT = 'e978763b74acfb0369b65b879af6a3a8c90b110b'
PARCELS = ('unity-avenue-11', 'unity-avenue-13', 'unity-avenue-15', 'unity-avenue-17')
SEGMENTS = tuple(range(6, 11))
# Ground22 adds 30/20 cm to strip length, centred on the source segment.
END_EXTENSION_CM = {'support': 15, 'asphalt': 10}
PINS = {
    '.git/operations/world-generation-20261010/readonly-source/packages/assets/city/lagos-city-v3.json':
        '13a2699ee6ad513616ca25a41bbb05f165e9aeaf3e52b5d89e550ed8b523ad9a',
    'output/unreal-unity11-corridor01-20261014/corridor-contact-survey.json':
        '627879ff11f96147a5a80256750a72915774fbc49a77c185ebf49224864afe3c',
    'output/unreal-unity11-corridor01-20261014/corridor-survey.json':
        'aa515d2fd863010716180096a7f6aca75d15fbb1d8b50ac9a6759af3fb96417d',
    'output/unreal-adjoining-20261014/survey.json':
        'e6cde3e7ae3bb083f32b0cb7bd8a50971597a7587afd2dcc55016888b01d52f7',
    'output/unreal-adjoining-20261014/ground-prepared.json':
        '2a026d3671ed6b8fa6ba2bb2160c61b45e927dfcfa1c5ddf5583a040b84a0a03',
    'output/unreal-adjoining-continuation01-20261014/ground-installed.json':
        'a4c3e7b7370a31a37aefd0bba033261b3b9d2e2a409d4df0c757f783fb5ed828',
    'output/unreal-forecourts-20261014/survey.json':
        'c8716f105d811d9ec9620f5d37431051b18083a5b61b93a84cd60ae40db38ab2',
}


def require(condition, message):
    if not condition:
        raise ValueError(message)


def finite_tree(value):
    if isinstance(value, float):
        require(math.isfinite(value), 'Non-finite input')
    elif isinstance(value, dict):
        for item in value.values():
            finite_tree(item)
    elif isinstance(value, list):
        for item in value:
            finite_tree(item)


def vector(value, size):
    require(isinstance(value, list) and len(value) == size, 'Invalid coordinate dimension')
    require(all(type(v) in (int, float) and math.isfinite(v) for v in value), 'Invalid coordinate')
    return value


def polygon(value):
    require(isinstance(value, list) and len(value) >= 3, 'Missing mask polygon')
    for point in value:
        vector(point, 2)
    area = sum(a[0]*b[1]-b[0]*a[1] for a, b in zip(value, value[1:]+value[:1]))
    require(abs(area) > .01, 'Degenerate polygon')
    return value


def frame(value):
    vector(value['centre'], 3)
    vector(value['extents'], 3)
    require(all(v > 0 for v in value['extents']), 'Nonpositive frame extent')
    require(type(value['yaw']) in (int, float) and math.isfinite(value['yaw']), 'Invalid yaw')
    return value


def local_point(value, x, y):
    angle = math.radians(value['yaw'])
    return [value['centre'][0]+x*math.cos(angle)-y*math.sin(angle),
            value['centre'][1]+x*math.sin(angle)+y*math.cos(angle)]


def probes(value):
    require(isinstance(value, list) and value, 'Missing sparse samples')
    for sample in value:
        vector(sample['xy'], 2)
        require(('supportZ' in sample) != ('blocker' in sample), 'Ambiguous sample')
        if 'supportZ' in sample:
            require(type(sample['supportZ']) in (int, float) and math.isfinite(sample['supportZ']), 'Invalid height')
        else:
            require(isinstance(sample['blocker'], str) and sample['blocker'], 'Missing blocker')
    return value


def load_inputs(repo):
    """Read only exact retained bytes; no authoring module or runtime API imports."""
    root = Path(repo).resolve()
    result = {}
    for relative, expected in PINS.items():
        path = (root / relative).resolve()
        require(path.is_relative_to(root), 'Input path escaped repository')
        raw = path.read_bytes()
        require(hashlib.sha256(raw).hexdigest() == expected, 'Pinned input mismatch: ' + relative)
        data = json.loads(raw, parse_constant=lambda text: (_ for _ in ()).throw(ValueError(text)))
        require(isinstance(data, dict), 'Invalid receipt object')
        finite_tree(data)
        result[relative] = data
    return result


def check_road_frame(value, start, end, half_width, end_extension):
    length = math.dist(start, end)
    require(length > 0, 'Degenerate source segment')
    require(math.dist(value['centre'][:2], [(start[j]+end[j])/2 for j in (0, 1)]) < .1,
            'Source/native centre mismatch')
    require(abs(value['extents'][0] - (length/2 + end_extension)) < .1, 'Source/native length mismatch')
    require(abs(value['extents'][1] - half_width) < .1, 'Source/native width mismatch')
    direction = [(end[j]-start[j])/length for j in (0, 1)]
    extended_start = [start[j]-direction[j]*end_extension for j in (0, 1)]
    extended_end = [end[j]+direction[j]*end_extension for j in (0, 1)]
    endpoints = [local_point(value, x, 0) for x in (-value['extents'][0], value['extents'][0])]
    require(min(max(math.dist(endpoints[0], a), math.dist(endpoints[1], b))
                for a, b in ((extended_start, extended_end), (extended_end, extended_start))) < .1,
            'Source/native direction mismatch')


def serialize_plan(repo):
    """Return deterministic planned-only JSON. Calling/emitting it needs separate authority."""
    data = load_inputs(repo)
    city, contacts, survey, adjoining, ground, continuation, forecourts = [data[p] for p in PINS]
    require(city['units'] == 'metres', 'Unexpected source units')
    for receipt in (contacts, survey, adjoining, forecourts):
        require(receipt['map'] == MAP, 'Wrong survey map')
    require(survey['sourceSHA256'] == next(iter(PINS.values())), 'Wrong survey city pin')
    require(adjoining['sourceCommit'] == SOURCE_COMMIT, 'Wrong source commit')
    require(contacts['nativeMutation'] is False, 'Contact observations are not read-only')
    require(len(contacts['unity11CoarseOccupancy']) == 166, 'Unity11 occupied exclusion is missing')
    require(not contacts['unity11SignatureRecords'], 'Changed signature inventory')
    occupied = contacts['unity11CoarseOccupancy']
    require(len({r['actor'] for r in occupied}) == 166, 'Duplicate occupied actors')
    require(all(r['sourceId'].split(':')[0] == PARCELS[0] for r in occupied), 'Wrong occupied source ID')
    canonical_parcels = {p['id']: p for p in city['parcels']}
    require(len(canonical_parcels) == len(city['parcels']), 'Duplicate canonical parcels')
    parcel_rows = {r['parcel']['id']: r for r in survey['parcels']}
    require(set(parcel_rows) == set(PARCELS) and len(survey['parcels']) == 4, 'Wrong parcel scope')
    for pid, row in parcel_rows.items():
        require(row['parcel'] == canonical_parcels[pid], 'Parcel source changed: ' + pid)
        vector(row['nativeCentreCm'], 2)
        frame(row['nativeFootprint'])
        polygon(row['nativeFootprintCornersCm'])
        probes(row['supportProbes'])
        p = canonical_parcels[pid]
        require(math.dist(row['nativeCentreCm'], [p['z']*100, p['x']*100]) < .1, 'Parcel centre conversion mismatch')
    roads = [r for r in city['roads'] if r['id'] == 'unity-avenue']
    require(len(roads) == 1, 'Missing/duplicate Unity Avenue')
    road = roads[0]
    frames = {r['label']: r for r in contacts['rows']}
    labels = {'TC_Ground22_unity-avenue_' + str(i) + '_' + kind for i in SEGMENTS for kind in ('support', 'asphalt')}
    require(set(frames) == labels and len(contacts['rows']) == 10, 'Wrong native segment scope')
    candidates = []
    for i in SEGMENTS:
        a, b = [vector(p, 2) for p in road['points'][i:i+2]]
        start, end = [a[1]*100, a[0]*100], [b[1]*100, b[0]*100]
        for kind in ('support', 'asphalt'):
            row = frames['TC_Ground22_unity-avenue_' + str(i) + '_' + kind]
            f = frame(row['frame'])
            polygon(row['polygon'])
            expected_polygon = [local_point(f, x, y) for x, y in (
                (-f['extents'][0], -f['extents'][1]), (f['extents'][0], -f['extents'][1]),
                (f['extents'][0], f['extents'][1]), (-f['extents'][0], f['extents'][1]))]
            require(len(row['polygon']) == 4 and all(math.dist(a, b) < .1 for a, b in zip(row['polygon'], expected_polygon)),
                    'Frame/polygon mismatch')
            require(len(probes(row['edgeProbes'])) == 16, 'Incomplete sparse sample record')
            half_width = (road['width']/2 + (road['pavement'] if kind == 'support' else 0))*100
            check_road_frame(f, start, end, half_width, END_EXTENSION_CM[kind])
        support = frames['TC_Ground22_unity-avenue_' + str(i) + '_support']['frame']
        asphalt = frames['TC_Ground22_unity-avenue_' + str(i) + '_asphalt']['frame']
        for side in (-1, 1):
            candidates.append({'sourceRoadId': road['id'], 'sourceSegmentIndex': i, 'side': side,
                'status': 'proposed-contact-review-only',
                'asphaltEdgeXYCm': [local_point(asphalt, x, side*asphalt['extents'][1]) for x in (-asphalt['extents'][0], asphalt['extents'][0])],
                'supportEdgeXYCm': [local_point(support, x, side*support['extents'][1]) for x in (-support['extents'][0], support['extents'][0])],
                'proposedElevationCm': None,
                'requiredReview': ['ground overlap clipping', 'curb/drain grade and outfall',
                    'door/driveway/parcel clearance', 'junction continuity', 'complete richer pre-state'],
                'mutationEligibility': False})
    masks = []
    for origin, receipt in (('adjoining', adjoining), ('forecourts', forecourts)):
        require(isinstance(receipt['protectedGround'], list) and receipt['protectedGround'], 'Missing native masks')
        for mask in receipt['protectedGround']:
            frame(mask)
            require(isinstance(mask['label'], str) and mask['label'], 'Missing mask source identity')
            masks.append({'receipt': origin, 'nativeSourceLabel': mask['label'], 'frameCm': mask,
                          'policy': 'preserve; no overlap permission'})
    surfaces = []
    for origin, receipt in (('adjoining-ground', ground), ('continuation-ground', continuation)):
        require(isinstance(receipt['surfaces'], list) and receipt['surfaces'], 'Missing existing ground')
        for surface in receipt['surfaces']:
            require(surface['pieces'], 'Empty existing ground mask')
            for piece in surface['pieces']:
                polygon(piece)
            surfaces.append({'receipt': origin, 'sourceId': surface['id'], 'surface': surface,
                             'policy': 'preserve; proposed contacts not clipped or authorized'})
    unsupported = [{'nativeSourceLabel': r['label'], 'sample': p} for r in contacts['rows']
                   for p in r['edgeProbes'] if 'blocker' in p]
    unsupported += [{'sourceParcelId': r['parcel']['id'], 'sample': p} for r in survey['parcels']
                    for p in r['supportProbes'] if 'blocker' in p]
    require(any(r.get('nativeSourceLabel') == 'TC_Ground22_unity-avenue_7_support' for r in unsupported), 'Missing segment7 exclusion')
    require(any(r.get('sourceParcelId') == 'unity-avenue-15' for r in unsupported), 'Missing Unity15 exclusion')
    plan = {'schemaVersion': 1, 'kind': 'planned-only-non-authoring', 'map': MAP,
        'sourceCommit': SOURCE_COMMIT, 'inputSHA256': PINS,
        'units': {'canonical': 'metres Y-up', 'native': 'centimetres Z-up', 'conversion': 'UE(X,Y,Z)=100*source(Z,X,Y)'},
        'scope': {'road': road['id'], 'segments': list(SEGMENTS), 'parcels': list(PARCELS)},
        'retainedStripEndExtensionCm': END_EXTENSION_CM,
        'observedSparseSamples': contacts['rows'], 'observedParcelFramesAndSamples': survey['parcels'],
        'occupiedUnity11Exclusion': occupied, 'unsupportedExclusions': unsupported,
        'canonicalParcelAndEntranceProtections': city['parcels'], 'nativeGroundMasks': masks,
        'canonicalRoadProtections': city['roads'],
        'existingGroundSurfaceExclusions': surfaces, 'forecourtParcelAndParkProtections': {'plans': forecourts['plans'], 'parks': forecourts['parks']},
        'proposedContacts': candidates,
        'unverifiedGaps': ['No complete grade/support/clearance survey', 'Masks retained but contacts not clipped',
            'No fitted curb/drain geometry or outfall', 'No continuous pedestrian/door/driveway route proof',
            'No complete richer pre-state; exact BodySetup settings unavailable'],
        'mutationEligibility': False, 'nativeContentCreated': False, 'completeContactAcceptance': False}
    return json.dumps(plan, indent=2, sort_keys=True, allow_nan=False) + '\n'

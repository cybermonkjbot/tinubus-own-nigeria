"""Explicit bounded native Unity Avenue11/corridor phases in the existing editor."""
import hashlib
import json
import os
from pathlib import Path
import runpy
import traceback

import unreal as u

HERE = Path(__file__).parent
PREVIOUS = runpy.run_path(str(HERE / 'continue_adjoining01.py'))
AD = PREVIOUS['AD']
REPO = AD['REPO']
OUT = REPO / 'output/unreal-unity11-corridor01-20261014'
BASE = AD['ROOT'] + '/Unity11Corridor01'
LAYER = AD['ROOT'] + '/Maps/L_TownCentre_Unity11Corridor01'
OWNER = 'Baddie.Unity11Corridor01.20261014'
PID = 10404


def write(name, data):
    OUT.mkdir(parents=True, exist_ok=True)
    with (OUT / name).open('x', encoding='utf-8') as stream:
        json.dump(data, stream, indent=2, allow_nan=False)
        stream.write('\n')


def guard():
    if os.getpid() != PID:
        raise RuntimeError('Editor identity changed')
    AD['guard'](True)
    if u.get_editor_subsystem(u.UnrealEditorSubsystem).get_game_world():
        raise RuntimeError('Game world is active')
    if u.EditorLoadingAndSavingUtils.get_dirty_content_packages():
        raise RuntimeError('Preserve unrelated dirty content')
    if any(c.get_editor_property('generation_in_progress') for a in AD['actors']()
           for c in a.get_components_by_class(u.PCGComponent)):
        raise RuntimeError('Competing PCG writer')


def current_state():
    namespace = PREVIOUS['state'].__globals__
    namespace['OWNER'] = OWNER
    return namespace['state']()


def probe(name):
    guard()
    try:
        snapshot = current_state()
    except Exception:
        write(name, {'success': False, 'pid': PID, 'map': AD['MAP'],
                     'failure': traceback.format_exc(), 'nativeMutation': False})
        raise
    write(name, {'success': True, 'pid': PID, 'map': AD['MAP'],
                 'state': snapshot, 'nativeMutation': False})
    print(json.dumps({'success': True, 'receipt': str(OUT / name),
                      'actors': len(snapshot['actors'])}))


def survey():
    """Canonical/native occupancy and support only; no authoring operations."""
    guard()
    namespace = AD['survey'].__globals__
    namespace.update(OUTPUT=OUT, BASE=BASE, LAYER=LAYER, OWNER=OWNER,
        FOCUS=('unity-avenue-11',), LABEL_PREFIX='Unity11Corridor01: ',
        FOLDER='StreetFinish_20261014/Unity11Corridor01',
        GRAPH_NAME='PCG_Unity11Corridor01FacadeGrammar', MESH_PREFIX='SM_U11_',
        IMPORT_SOURCE='unity11-corridor01-import-source')
    try:
        namespace['survey']()
    except Exception:
        write('parcel-survey-failure.json', {'failure': traceback.format_exc(), 'nativeMutation': False})
    source = PREVIOUS['SOURCE']
    city = json.loads(source.read_text())
    ids = ('unity-avenue-11', 'unity-avenue-13', 'unity-avenue-15', 'unity-avenue-17')
    geometry = PREVIOUS['GROUND']
    records = []
    scene = AD['actors']()
    for parcel in city['parcels']:
        if parcel['id'] not in ids:
            continue
        centre = [parcel['z'] * 100, parcel['x'] * 100]
        # Canonical-to-native conversion is the accepted z/x axis swap.
        footprint = next(a for a in scene if a.get_actor_label() == 'TC_Ground22_' + parcel['id'] + '_footprint')
        box = AD['BIOME']['source_box'](footprint)
        frame = {'centre': AD['STREET']['vec'](box[0]), 'extents': AD['STREET']['vec'](box[1]), 'yaw': box[2].yaw}
        corners = geometry['box_poly'](frame)
        probes = []
        for point in [centre] + corners:
            try:
                probes.append({'xy': point, 'supportZ': AD['STREET']['ground'](u.Vector(*point, 0))})
            except RuntimeError as error:
                probes.append({'xy': point, 'blocker': str(error)})
        occupancy = [{'actor': a.get_path_name(), 'label': a.get_actor_label()}
            for a in scene if not a.get_actor_label().startswith('TC_Ground22_') and any(
                str(t) == 'Baddie.Source:' + parcel['id'] or str(t).startswith('Baddie.Source:' + parcel['id'] + ':')
                for t in a.tags)]
        records.append({'parcel': parcel, 'nativeCentreCm': centre, 'nativeFootprint': frame,
                        'nativeFootprintCornersCm': corners, 'supportProbes': probes, 'occupancy': occupancy})
    write('corridor-survey.json', {'map': AD['MAP'], 'proposedLayer': LAYER, 'proposedAssets': BASE,
        'sourceSHA256': hashlib.sha256(source.read_bytes()).hexdigest(), 'parcels': records,
        'policy': 'Only Unity11 may receive new building; 13/15/17 preserved; native authoring held on richer API gate'})
    print(json.dumps({'survey': str(OUT / 'corridor-survey.json'), 'nativeMutation': False}))


def contacts():
    guard()
    scene = AD['actors']()
    geometry = PREVIOUS['GROUND']
    rows = []
    for actor in scene:
        label = actor.get_actor_label()
        if not label.startswith('TC_Ground22_unity-avenue_'):
            continue
        box = AD['BIOME']['source_box'](actor)
        if not box or not -60500 <= box[0].x <= -37500:
            continue
        frame = {'centre': AD['STREET']['vec'](box[0]), 'extents': AD['STREET']['vec'](box[1]), 'yaw': box[2].yaw}
        polygon = geometry['box_poly'](frame)
        probes = []
        for a, b in zip(polygon, polygon[1:] + polygon[:1]):
            for fraction in (0, .25, .5, .75):
                point = [a[i] + (b[i]-a[i])*fraction for i in (0, 1)]
                try:
                    probes.append({'xy': point, 'supportZ': AD['STREET']['ground'](u.Vector(*point, 0))})
                except RuntimeError as error:
                    probes.append({'xy': point, 'blocker': str(error)})
        rows.append({'label': label, 'actor': actor.get_path_name(), 'frame': frame,
                     'polygon': polygon, 'edgeProbes': probes})
    kit = AD['native_kit']()
    old, _ = kit.BASE.frontage_rows(*kit.BASE.canonical(), kit.BASE.load_frontage_data(), kit.BASE.v3_modules())
    names = {a.get_actor_label(): a for a in scene}
    occupied = [{'sourceId': r['sourceId'], 'actor': names['TC_' + r['module'] + '_' + str(i)].get_path_name()}
        for i, r in enumerate(old) if r['sourceId'].split(':')[0] == 'unity-avenue-11'
        and 'TC_' + r['module'] + '_' + str(i) in names]
    signatures = json.loads((REPO / 'output/cribtest11-world-generation/coverage.json').read_text())['city']['architecture']
    write('corridor-contact-survey.json', {'map': AD['MAP'], 'rows': rows,
        'unity11CoarseOccupancy': occupied,
        'unity11SignatureRecords': [s for s in signatures if s['parcelId'] == 'unity-avenue-11'],
        'retained': PREVIOUS['retained'](), 'nativeMutation': False,
        'limitation': 'Edge ray samples only; not fitted geometry, clearance acceptance or complete contact proof'})
    print(json.dumps({'contactFrames': len(rows), 'unity11CoarseActors': len(occupied),
                      'unsupportedProbes': sum('blocker' in p for r in rows for p in r['edgeProbes'])}))

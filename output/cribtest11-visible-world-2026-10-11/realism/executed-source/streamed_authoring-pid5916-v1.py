"""Native saved streamed-level alternative, no WP relabel or external writers.

Chosen because installed ULevel default external package paths escape the owned
Content/BaddieGeneratedWorld namespace. Empty district assets are structural
placeholders, not generated district geometry or playable destinations.
"""
import gc
import json
from datetime import datetime, timezone
import unreal
from WorldGeneration.Realism import author_slice as common

NS = common.NS + '/StreamedAuthoring_v1'
MAP = NS + '/Maps/L_Lagos_AuthoringPersistent_v1'
DISTRICTS = ['Mainland_Yaba', 'Mainland_Makoko', 'Mainland_Ikeja', 'Mainland_Oworonshoki',
             'Corridor_ThirdMainland', 'Island_LagosIsland', 'Island_Ikoyi',
             'Island_VictoriaIsland', 'FictionalIsland_WesternLekki', 'Interiors', 'OptionalDressing']


def create_children():
    # World only held within this scope; no load/new level while references live.
    levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    children = []
    for name in DISTRICTS:
        package = NS + '/Districts/L_' + name
        streaming = unreal.EditorLevelUtils.create_new_streaming_level(unreal.LevelStreamingDynamic, package, False)
        assert streaming is not None
        loaded = streaming.get_loaded_level()
        assert loaded is not None
        assert str(streaming.get_world_asset_package_f_name()) == package
        streaming.set_editor_property('disable_distance_streaming', True)
        world = loaded.get_outer()
        common.claim(world, common.sha(__file__))
        assert unreal.EditorLoadingAndSavingUtils.save_map(world, package)
        children.append({'id': name, 'package': package, 'streamingObject': streaming.get_path_name(),
                         'streamingClass': streaming.get_class().get_name(), 'files': common.package_files(package, True),
                         'geometryPresent': False, 'worldTransform': 'identity unresolved authoring placeholder',
                         'movementAdmission': False, 'role': 'Explicit native streamed authoring level; not geographic land geometry'})
    assert levels.save_current_level()
    return children


def author(expected_pid):
    common.guard(expected_pid)
    assert not (common.OUT / 'streamed-authoring-v1.json').exists()
    common.fresh(MAP, True)
    for name in DISTRICTS:
        common.fresh(NS + '/Districts/L_' + name, True)
    gc.collect()
    assert unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).new_level(MAP, is_partitioned_world=False)
    # No environment duplication: district content/art maps remain separate.
    children = create_children()
    files = common.package_files(MAP, True)
    for child in children:
        files.update(child['files'])
    common.record('streamed-authoring-v1.json', {'schemaVersion': 1, 'pid': expected_pid,
        'recordedAt': datetime.now(timezone.utc).isoformat(), 'scriptSha256': common.sha(__file__),
        'mapPackage': MAP, 'files': files, 'districtLevels': children, 'partitioned': False,
        'externalActors': False, 'alternative': 'Native LevelStreamingDynamic district levels, saved parent relationships',
        'worldPartitionDecision': 'Installed new_level(true) available, but ULevel::GetExternalActorsPath defaults to /Game/__ExternalActors__/BaddieGeneratedWorld outside owned write path; no safe Python routing confirmed',
        'dataLayers': 'Not configured; chosen nonpartitioned alternative uses explicit district/interior/dressing levels',
        'hlodLayers': 'Not configured on nonpartitioned alternative', 'hlodProxyCount': 0,
        'regionGeometryGenerated': False, 'qualitySlice': 'Independent locally anchored DenseStreet_v1; no district transform applied',
        'geographyReferenceSha256': 'e3088192c43d4e70d3a37873598e699794997f38bf580ff7154ddaaa71769352',
        'canonicalPlacementsChanged': False, 'coastlineMeshes': None, 'bridgeEndpointsRelocated': False,
        'gates': common.CONFIG['gates'], 'playerStreamingImplemented': False, 'playableAcceptance': False})
    print(json.dumps({'persistentMap': MAP, 'nativeDistrictLevelCount': len(children), 'receiptSha256': common.sha(common.OUT / 'streamed-authoring-v1.json')}))

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
from WorldGeneration.Realism import safe_level_change as safe

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
    # CreateNewStreamingLevel makes the child current. Saving current here would
    # save OptionalDressing, not the parent relationships. Explicit parent save.
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    persistent = next(level for level in unreal.EditorLevelUtils.get_levels(world)
                      if level.get_path_name() == world.get_path_name() + ':PersistentLevel')
    assert world.get_path_name().split('.')[0] == MAP
    assert persistent.get_outer() == world
    unreal.EditorLevelUtils.make_level_current(persistent)
    assert unreal.EditorLoadingAndSavingUtils.save_map(world, MAP)
    return children


def author(expected_pid):
    common.guard(expected_pid)
    assert not (common.OUT / 'streamed-authoring-v1.json').exists()
    common.fresh(MAP, True)
    for name in DISTRICTS:
        common.fresh(NS + '/Districts/L_' + name, True)
    safe.prepare_change(expected_pid)
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


RECOVERY_MAP = common.NS + '/StreamedAuthoring_v2/Maps/L_Lagos_AuthoringPersistent_v2'
OPTIONAL_CHILD_FILE = 'apps/companion-unreal/Content/BaddieGeneratedWorld/Realism/YabaSlice_20261011/StreamedAuthoring_v1/Districts/L_OptionalDressing.umap'
OPTIONAL_CHILD_FINAL_SHA = '9bba2e57b4a902b8dddbb9c0c1b4c4c3daaa00d98ddf0ed0a5f4b2c40f815b41'


def attach_saved_children():
    # This helper ends before any reload. Only strings/records escape its scope.
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    assert world.get_path_name().split('.')[0] == RECOVERY_MAP
    children = []
    for name in DISTRICTS:
        package = NS + '/Districts/L_' + name
        streaming = unreal.EditorLevelUtils.add_level_to_world(world, package, unreal.LevelStreamingDynamic)
        assert streaming is not None
        assert str(streaming.get_world_asset_package_f_name()) == package
        streaming.set_editor_property('disable_distance_streaming', True)
        children.append({'id': name, 'package': package, 'streamingObject': streaming.get_path_name(),
                         'geometryPresent': False, 'movementAdmission': False})
    persistent = next(level for level in unreal.EditorLevelUtils.get_levels(world)
                      if level.get_path_name() == world.get_path_name() + ':PersistentLevel')
    unreal.EditorLevelUtils.make_level_current(persistent)
    common.claim(world, common.sha(__file__))
    assert unreal.EditorLoadingAndSavingUtils.save_map(world, RECOVERY_MAP)
    return children


def recover_after_authorized_reopening(expected_pid):
    """Separate new parent; retain old dirty-save receipt and v1 file unchanged."""
    common.guard(expected_pid)
    old_path = common.OUT / 'streamed-authoring-v1.json'
    assert common.sha(old_path) == '7866a03ffeaf6a713bfec43544ede2e1b3cf4781866680d80c4eec4fd0564283'
    old = json.loads(old_path.read_text(encoding='utf-8'))
    # Later save_current_level attempts saved the current OptionalDressing child.
    # Preserve the old receipt; bind actual final bytes instead of silently
    # asserting that its earlier child hash still matches. Parent is unchanged.
    current_files = dict(old['files'])
    current_files[OPTIONAL_CHILD_FILE] = OPTIONAL_CHILD_FINAL_SHA
    common.verify_files(current_files)
    assert not (common.OUT / 'streamed-authoring-v2.json').exists()
    common.fresh(RECOVERY_MAP, True)
    safe.prepare_change(expected_pid)
    assert unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).new_level(RECOVERY_MAP, is_partitioned_world=False)
    children = attach_saved_children()
    common.verify_files(current_files)
    files = dict(current_files); files.update(common.package_files(RECOVERY_MAP, True))
    common.record('streamed-authoring-v2.json', {'schemaVersion': 1, 'pid': expected_pid,
        'scriptSha256': common.sha(__file__), 'mapPackage': RECOVERY_MAP, 'files': files,
        'historicalChildResave': {'path': OPTIONAL_CHILD_FILE, 'sha256': OPTIONAL_CHILD_FINAL_SHA,
                                 'reason': 'Wrong-current-level save attempts before crash; not a new recovery mutation'},
        'districtLevels': children, 'partitioned': False, 'externalActors': False,
        'savedRelationships': 'explicit parent save; requires independent reload/readback',
        'districtGeometryPresent': False, 'hlodProxyCount': 0, 'gates': common.CONFIG['gates'],
        'playerStreamingImplemented': False, 'playableAcceptance': False})


def readback_recovered_parent(expected_pid):
    """Separate bridge call after load_saved returns; no switch in this scope."""
    common.guard(expected_pid)
    path = common.OUT / 'streamed-authoring-v2.json'
    receipt = json.loads(path.read_text(encoding='utf-8')); common.verify_files(receipt['files'])
    assert not safe.dirty_names()
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    assert world.get_path_name().split('.')[0] == RECOVERY_MAP
    relationships = []
    for child in receipt['districtLevels']:
        streaming = unreal.GameplayStatics.get_streaming_level(world, unreal.Name(child['package']))
        assert streaming is not None
        assert str(streaming.get_world_asset_package_f_name()) == child['package']
        relationships.append({'package': child['package'], 'streamingClass': streaming.get_class().get_name(),
                              'disableDistanceStreaming': streaming.get_editor_property('disable_distance_streaming')})
    common.record('streamed-authoring-v2-readback.json', {'pid': expected_pid,
        'mapPackage': RECOVERY_MAP, 'receiptSha256': common.sha(path), 'relationships': relationships,
        'dirtyPackages': safe.dirty_names(), 'partitioned': False, 'districtGeometryPresent': False,
        'playerStreamingImplemented': False, 'playableAcceptance': False})

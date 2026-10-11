"""Finish only the inspected PID1784 partial v2 parent; no authoring replay.

Installed MakeLevelCurrent Python accepts LevelStreaming, not Level. Saving an
explicit World with save_map does not require switching its current sublevel.
No load/new-level operation may occur inside this UObject-owning scope.
"""
import json
from datetime import datetime, timezone
import unreal
from WorldGeneration.Realism import author_slice as common
from WorldGeneration.Realism import safe_level_change as safe
from WorldGeneration.Realism import streamed_authoring as prepared


def finish(expected_pid):
    assert __import__('os').getpid()==expected_pid
    assert unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world() is None
    world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    assert world.get_path_name().split('.')[0]==prepared.RECOVERY_MAP
    assert safe.dirty_names()==[prepared.RECOVERY_MAP]
    assert not (common.OUT/'streamed-authoring-v2.json').exists()
    old=json.loads((common.OUT/'streamed-authoring-v1.json').read_text(encoding='utf-8'))
    files=dict(old['files']);files[prepared.OPTIONAL_CHILD_FILE]=prepared.OPTIONAL_CHILD_FINAL_SHA
    common.verify_files(files)
    relationships=[]
    for name in prepared.DISTRICTS:
        package=prepared.NS+'/Districts/L_'+name
        streaming=unreal.GameplayStatics.get_streaming_level(world,unreal.Name(package))
        assert streaming is not None
        assert streaming.get_editor_property('disable_distance_streaming')
        assert str(streaming.get_world_asset_package_f_name())==package
        relationships.append({'id':name,'package':package,'streamingObject':streaming.get_path_name(),
                              'geometryPresent':False,'movementAdmission':False})
    common.claim(world,common.sha(__file__))
    assert unreal.EditorLoadingAndSavingUtils.save_map(world,prepared.RECOVERY_MAP)
    assert not safe.dirty_names()
    common.verify_files(files);files.update(common.package_files(prepared.RECOVERY_MAP,True))
    common.record('streamed-authoring-v2.json',{'schemaVersion':1,'pid':expected_pid,
        'recordedAt':datetime.now(timezone.utc).isoformat(),'scriptSha256':common.sha(__file__),
        'preparedScriptSha256':common.sha(prepared.__file__),'mapPackage':prepared.RECOVERY_MAP,'files':files,
        'districtLevels':relationships,'partitioned':False,'externalActors':False,
        'recovery':'11children attached before MakeLevelCurrent Level/LevelStreaming mismatch; explicit World save finishes the inspected partial without replay',
        'savedRelationships':'Explicit World save; independent reload/readback pending',
        'historicalChildResave':{'path':prepared.OPTIONAL_CHILD_FILE,'sha256':prepared.OPTIONAL_CHILD_FINAL_SHA},
        'districtGeometryPresent':False,'playerStreamingImplemented':False,'hlodProxyCount':0,
        'gates':common.CONFIG['gates'],'playableAcceptance':False})

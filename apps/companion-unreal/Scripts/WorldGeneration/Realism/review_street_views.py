"""Supplementary walking-height native inspection; no source map mutation."""
import json
import unreal
from WorldGeneration.Realism import author_slice as common
from WorldGeneration.Realism import frontage_v3_native as street

CAMERAS=[
    {'id':'stock-threshold','locationMetres':[-1.2,-1.95,1.65],
     'rotation':{'pitch':-8,'yaw':0,'roll':0},'fov':65},
    {'id':'side-street-depth','locationMetres':[-1.8,-68.5,1.65],
     'rotation':{'pitch':2,'yaw':0,'roll':0},'fov':75}]
RECEIPT='yaba-dense-v2-supplemental-views.json'


def prepare(expected_pid,camera_id):
    common.guard(expected_pid)
    assert unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world().get_path_name().split('.')[0]==street.MAP
    path=common.OUT/RECEIPT
    if not path.exists():
        base=common.OUT/street.RECEIPT
        data=json.loads(base.read_text(encoding='utf-8'));common.verify_files(data['files'])
        data.update({'baseReceiptSha256':common.sha(base),'cameras':CAMERAS,
                     'role':'Supplementary camera-only inspection, same saved source map',
                     'viewScriptSha256':common.sha(__file__)})
        common.record(RECEIPT,data)
    spec=next(c for c in CAMERAS if c['id']==camera_id)
    levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem);key=levels.get_active_viewport_config_key()
    levels.set_level_viewport_camera_info(unreal.Vector(*(v*100 for v in spec['locationMetres'])),common.rotation(spec['rotation']),key)
    levels.set_level_viewport_fov(spec['fov'],key);levels.editor_set_game_view(True,key);levels.editor_invalidate_viewports()

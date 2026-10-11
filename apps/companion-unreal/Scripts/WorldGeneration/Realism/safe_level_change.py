"""Release *all* package wrappers before a level change, not just Worlds.

Read-only dirty inspection returns strings. Never carry UPackage/Level/Actor
wrappers, exception tracebacks containing wrappers, or returned worlds into
load_level/new_level. The bridge owns execution/PID/authentication unchanged.
"""
import gc
import os
import sys
import unreal
from WorldGeneration.Realism import author_slice as common


def dirty_names():
    return [p.get_name() for p in list(unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages())
            + list(unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages())]


def prepare_change(expected_pid):
    assert os.getpid() == expected_pid
    common.guard(expected_pid)  # Guard scope returns before the level transition.
    assert not dirty_names(), 'Do not carry dirty package wrappers through load'
    # Prior caught Python exceptions can retain frames/wrappers. Clear only the
    # interpreter's standard last-exception references; no arbitrary user globals.
    for name in ('last_type', 'last_value', 'last_traceback', 'last_exc'):
        if hasattr(sys, name):
            setattr(sys, name, None)
    gc.collect()


def load_saved(expected_pid, package):
    assert package.startswith(common.NS + '/')
    assert common.native_file(package, True).is_file()
    prepare_change(expected_pid)
    assert unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level(package)

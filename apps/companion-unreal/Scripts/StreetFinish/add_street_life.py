"""Bounded second infill and frontage-prop pass; explicit native editor phases.

Reuses the accepted facade/ground authors in private module instances. Their
default nine-plot pass, saved graphs and layers are never overwritten.
"""
from collections import Counter, defaultdict
import importlib.util
import json
import math
from pathlib import Path

import unreal as u


def module(name, filename):
    spec = importlib.util.spec_from_file_location(name, Path(__file__).with_name(filename))
    value = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(value)
    return value


AD = module('streetlife_buildings', 'add_adjoining.py')
AD.OUTPUT = AD.REPO / 'output/unreal-streetlife-20261014'
AD.BASE = AD.ROOT + '/StreetLife'
AD.LAYER = AD.ROOT + '/Maps/L_TownCentre_StreetLife'
AD.OWNER = 'Baddie.StreetLife.20261014'
AD.LABEL_PREFIX = 'Street life building: '
AD.FOLDER = 'StreetFinish_20261014/StreetLife'
AD.GRAPH_NAME = 'PCG_StreetLifeFacadeGrammar'
AD.MESH_PREFIX = 'SM_SL_'
AD.IMPORT_SOURCE = 'streetlife-20261014-import-source'
AD.FOCUS = ('unity-avenue-25', 'unity-avenue-26', 'unity-avenue-27', 'unity-avenue-28',
            'theatre-street-10', 'theatre-street-12', 'civic-street-7', 'civic-street-9',
            'civic-street-10', 'civic-street-11', 'civic-street-12', 'civic-street-13',
            'civic-street-15', 'civic-street-16', 'market-road-14', 'market-road-15',
            'market-road-16')
GROUND = module('streetlife_ground', 'add_adjoining_ground.py')
GROUND.AD = AD.__dict__
GROUND.BASE, GROUND.ROOT, GROUND.OUTPUT = AD.BASE, AD.ROOT, AD.OUTPUT
GROUND.SURFACE_PREFIX = 'Street life ground: '
GROUND.PLANTING_LABEL = 'Street life planting'
GROUND.PLANTING_GRAPH = 'PCG_StreetLife_GroundPlanting'
GROUND.MESH_PREFIX = 'SM_SL_'
GROUND.IMPORT_SOURCE = 'streetlife-20261014-ground-source'
GROUND.FOLDER = AD.FOLDER
# Existing verge layers remain intact. This pass supplies fitted new plot yards.
GROUND.VERGES = []


def survey():
    AD.survey()
    # Capture all pre-existing scenery counts as well as protected transforms.
    scene = AD.actors()
    AD.write('retained.json', {a.get_path_name(): sum(c.get_instance_count()
        for c in a.get_components_by_class(u.InstancedStaticMeshComponent)) for a in scene})


def save():
    AD.audit()
    if (AD.OUTPUT / 'ground-installed.json').exists():
        GROUND.audit()
    if (AD.OUTPUT / 'props-installed.json').exists():
        audit_props()
    scene = {a.get_path_name(): a for a in AD.actors()}
    for path, count in json.loads((AD.OUTPUT / 'retained.json').read_text()).items():
        if path not in scene or sum(c.get_instance_count() for c in scene[path].get_components_by_class(
                u.InstancedStaticMeshComponent)) != count:
            raise RuntimeError('Pre-existing actor or instance count changed: ' + path)
    dirty = {p.get_path_name() for p in u.EditorLoadingAndSavingUtils.get_dirty_map_packages()}
    if not dirty.issubset({AD.MAP, AD.LAYER}):
        raise RuntimeError('Unrelated dirty maps; preserve them')
    editor = u.get_editor_subsystem(u.LevelEditorSubsystem)
    for path in (AD.LAYER, AD.MAP):
        if not editor.set_current_level_by_name(path.rsplit('/', 1)[-1]) or not editor.save_current_level():
            raise RuntimeError('Could not save ' + path)
    print('Saved second infill and street-life layer and parent')

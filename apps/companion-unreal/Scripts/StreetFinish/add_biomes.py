"""Owned, additive PCG Biome Core authoring for existing canonical park loops.

Uses installed native templates, surface sampling and editable biome splines.
No installation, Landscape conversion, gameplay activation or fixed point scatter.
Call install() once; generate one named core at a time and inspect before saving.
"""
import hashlib
import json
import math
from pathlib import Path
import re
import runpy

import unreal as u

KIT = runpy.run_path(str(Path(__file__).with_name('build_street.py')))
ROOT, MAP = KIT['ROOT'], KIT['MAP']
BASE = ROOT + '/Biomes'
LAYER = ROOT + '/Maps/L_TownCentre_Biomes'
OWNER = 'Baddie.Biomes.20261014'
OUTPUT = Path(u.Paths.project_dir()).parents[1] / 'output/unreal-biomes-20261014'
SITES = ['college-green', 'neighbourhood-park-18']
# Density is points/m2; extents also enforce useful separation before pruning.
LAYERS = {
    'Canopy': (.008, 270, 14101, 18000, 26000),
    'Understory': (.045, 75, 14102, 7000, 10000),
    'Groundcover': (.16, 35, 14103, 4000, 6500),
}
MESHES = [
    ('Broadleaf', 'Canopy', ROOT + '/Foliage2/Meshes/SM_SF2_broadleaf_tree', 4, 1.0),
    ('Palm', 'Canopy', ROOT + '/WorldKit/Meshes/SM_WK_palm_tree', 1, .9),
    ('Shrubs', 'Understory', ROOT + '/Foliage2/Meshes/SM_SF2_shrub_cluster', 1, .85),
    ('Grasses', 'Groundcover', ROOT + '/EyeLevel/Meshes/SM_EL_GrassClump', 2, 1.25),
    ('LowPlants', 'Groundcover', ROOT + '/EyeLevel/Meshes/SM_EL_Groundcover', 1, 1.25),
]


def field(struct, name):
    """Resolve installed Blueprint struct GUID fields, not hard-coded GUIDs."""
    matches = re.findall(r'\b(' + re.escape(name) + r'_\d+_[0-9A-F]+)=', struct.export_text())
    if len(matches) != 1:
        raise RuntimeError('Ambiguous/missing native biome field: ' + name)
    return matches[0]


def put(struct, name, value):
    struct.set_editor_property(field(struct, name), value)


def get(struct, name):
    return struct.get_editor_property(field(struct, name))


def duplicate(source, name):
    path = BASE + '/' + name
    if u.EditorAssetLibrary.does_asset_exist(path):
        raise RuntimeError('Refusing to replace owned/editor-edited asset: ' + path)
    asset = u.EditorAssetLibrary.duplicate_asset(source, path)
    if not asset:
        raise RuntimeError('Native asset duplication failed: ' + path)
    u.EditorAssetLibrary.set_metadata_tag(asset, 'BiomeOwner', OWNER)
    return asset


def save_asset(asset):
    if not u.EditorAssetLibrary.save_loaded_asset(asset):
        raise RuntimeError('Native asset save failed: ' + asset.get_path_name())


def source_box(actor):
    c = actor.get_component_by_class(u.StaticMeshComponent)
    if not c or not c.static_mesh:
        return None
    b = c.static_mesh.get_bounding_box()
    t = c.get_world_transform()
    p = t.transform_location((b.min + b.max) * .5)
    e = (b.max - b.min) * .5
    s = t.scale3d
    return p, u.Vector(e.x * abs(s.x), e.y * abs(s.y), e.z * abs(s.z)), actor.get_actor_rotation()


def fingerprint(actors):
    # Serialize values, never UObject/UStruct display strings with memory addresses.
    records = [[a.get_path_name(), KIT['vec'](a.get_actor_location()),
                KIT['vec'](a.get_actor_scale3d()), str(a.get_actor_rotation().to_tuple())]
               for a in actors if a.get_actor_label().startswith('TC_')]
    return hashlib.sha256(json.dumps(sorted(records)).encode()).hexdigest()


def component_policy(actor):
    actor.tags = list(actor.tags) + [OWNER, 'Baddie.NoRuntimeAuthority']
    actor.set_folder_path('StreetFinish_20261014/Biomes')
    for c in actor.get_components_by_class(u.PCGComponent):
        c.set_editor_properties({'generation_trigger': u.PCGComponentGenerationTrigger.GENERATE_ON_DEMAND,
                                'regenerate_in_editor': False, 'is_component_partitioned': False})
    for c in actor.get_components_by_class(u.PrimitiveComponent):
        c.set_collision_enabled(u.CollisionEnabled.NO_COLLISION)
        c.set_editor_property('can_ever_affect_navigation', False)
        c.set_visibility(False, True)  # Editable boundaries, no solid editor guides.


def spawn(bp, label, point, rotation=None):
    actor = u.get_editor_subsystem(u.EditorActorSubsystem).spawn_actor_from_class(
        u.EditorAssetLibrary.load_blueprint_class('/PCGBiomeCore/Blueprints/' + bp),
        point, rotation or u.Rotator(pitch=0, yaw=0, roll=0))
    actor.set_actor_label(label)
    actor.set_actor_scale3d(u.Vector(1, 1, 1))
    # Native defaults scale Core's box 512x and exclusion actors 5x.
    # Our recorded extents are centimetres, so remove those template scales.
    for box in actor.get_components_by_class(u.BoxComponent):
        box.set_relative_scale3d(u.Vector(1, 1, 1))
    component_policy(actor)
    return actor


def install():
    editor = u.get_editor_subsystem(u.LevelEditorSubsystem)
    if KIT['world']().get_path_name().split('.')[0] != MAP:
        raise RuntimeError('Open the saved additive town centre first')
    if u.EditorLoadingAndSavingUtils.get_dirty_map_packages():
        raise RuntimeError('Resolve unsaved maps before adding biomes')
    if u.EditorAssetLibrary.does_asset_exist(LAYER) or u.EditorAssetLibrary.does_directory_exist(BASE):
        raise RuntimeError('Biome authoring already exists; edit native assets instead')
    actors = list(u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors())
    initial_fingerprint = fingerprint(actors)
    named = {a.get_actor_label(): a for a in actors}
    sites = []
    for site in SITES:
        segments = sorted([(int(m[1]), a) for label, a in named.items()
                           if (m := re.fullmatch('TC_Ground22_' + site + r'-loop_(\d+)_support', label))])
        if len(segments) < 6:
            raise RuntimeError('Missing canonical park loop: ' + site)
        # Ordered support midpoints make a conservative polygon inside the loop.
        points = [source_box(a)[0] for _, a in segments]
        for p in points:
            p.z = 0
        centre = sum(points, u.Vector()) / len(points)
        ex = max(abs(p.x - centre.x) for p in points)
        ey = max(abs(p.y - centre.y) for p in points)
        heights = [KIT['ground'](centre + u.Vector(ex * x, ey * y, 0))
                   for x, y in [(-.35, -.35), (-.35, .35), (.35, -.35), (.35, .35)]]
        if sum(abs(z + 5) <= 1 for z in heights) < 3:
            raise RuntimeError('Park no longer matches inspected support height')
        sites.append({'id': site, 'centre': centre, 'extents': u.Vector(ex + 150, ey + 150, 1500), 'points': points})
    for _, _, path, _, _ in MESHES:
        if not isinstance(u.load_asset(path), u.StaticMesh):
            raise RuntimeError('Missing approved mesh: ' + path)
    # Resolve the installed native struct shape before creating anything.
    template = u.load_asset('/PCGBiomeCore/BiomeAssets/DefaultAsset').get_editor_property('BiomeAssets')[0]
    for n in ['Generator', 'Mesh', 'AssetOptions', 'MeshOptions', 'FilterOptions']:
        field(template, n)
    stream = u.EditorLevelUtils.create_new_streaming_level(u.LevelStreamingAlwaysLoaded, LAYER, False)
    if not stream:
        raise RuntimeError('Could not create additive biome layer')
    u.EditorLevelUtils.make_level_current(stream)
    generators = {}
    for name, (density, extent, seed, _, _) in LAYERS.items():
        graph = duplicate('/PCGBiomeCore/GraphTemplates/TPL_BiomeCore_Generator', 'PCG_' + name)
        h = u.PCGGraphParametersHelpers
        h.set_bool_parameter(graph, 'SampleOnLandscape', False)
        h.set_bool_parameter(graph, 'Reset_Rotation_XY', True)
        h.set_float_parameter(graph, 'PointsPerSquaredMeter', density)
        h.set_vector_parameter(graph, 'PointExtents', u.Vector(extent, extent, 50))
        h.set_float_parameter(graph, 'PointsLooseness', .8)
        h.set_int32_parameter(graph, 'Seed', seed)
        for node in graph.nodes:
            settings = node.get_settings()
            settings.set_editor_property('execute_on_gpu', False)
            if isinstance(settings, u.PCGWorldRayHitSettings):
                q = settings.query_params
                q.trace_complex = True
                q.ignore_pcg_hits = False  # Existing support is native ISM.
                # Native PCG uses object-type queries, not visibility trace channels.
                q.collision_channel = u.CollisionChannel.ECC_WORLD_STATIC
                # Spline owners have shallow physical bounds; cover the ground below.
                q.override_default_params = True
                q.ray_origin = u.Vector(0, 0, 2000)
                q.ray_direction = u.Vector(0, 0, -1)
                q.ray_length = 3000.0
                settings.set_editor_property('query_params', q)
            if isinstance(settings, u.PCGTransformPointsSettings):
                settings.set_editor_properties({'scale_min': u.Vector(.85, .85, .85),
                    'scale_max': u.Vector(1.15, 1.15, 1.15), 'uniform_scale': True})
        save_asset(graph)
        asset = duplicate('/PCGBiomeCore/BiomeGenerators/DefaultGenerator', 'DG_' + name)
        struct = asset.get_editor_property('Generator')
        put(struct, 'GeneratorType', name)
        put(struct, 'GeneratorGraph', graph)
        put(struct, 'GeneratorPriority', list(LAYERS).index(name))
        put(struct, 'GeneratorAllowOverlap', name == 'Groundcover')
        asset.set_editor_property('Generator', struct)
        save_asset(asset)
        generators[name] = asset
    asset_set = duplicate('/PCGBiomeCore/BiomeAssets/DefaultAsset', 'DA_UrbanParkPlants')
    entries = []
    for name, layer, mesh_path, weight, scale in MESHES:
        entry = template.copy()
        put(entry, 'AssetType', name)
        put(entry, 'Generator', generators[layer])
        put(entry, 'Mesh', u.load_asset(mesh_path))
        put(entry, 'Weight', weight)
        opts = get(entry, 'AssetOptions')
        put(opts, 'Scale', u.Vector(scale, scale, scale))
        put(opts, 'SelfPrune', True)
        put(opts, 'OrientUpward', 1.0)
        # Mesh lowest vertex establishes actual ground contact, including stems.
        minimum = u.load_asset(mesh_path).get_bounding_box().min.z
        put(opts, 'Translation', u.Vector(0, 0, -minimum * scale))
        put(entry, 'AssetOptions', opts)
        opts = get(entry, 'MeshOptions')
        for key in ['AllowCollision', 'CanEverAffectNavigation', 'EvaluateWorldPositionOffset',
                    'AffectDistanceFieldLighting', 'AffectDynamicIndirectLighting', 'VisibleInRayTracing']:
            put(opts, key, False)
        put(opts, 'CastShadow', layer != 'Groundcover')
        put(opts, 'StartCullDistance', LAYERS[layer][3])
        put(opts, 'EndCullDistance', LAYERS[layer][4])
        put(opts, 'ComponentTags', OWNER + ',Baddie.NoRuntimeAuthority')
        put(entry, 'MeshOptions', opts)
        filt = get(entry, 'FilterOptions')
        # Native filtering runs after root translation: allow the measured stem
        # correction, while still rejecting the paved/path support at Z=0.
        put(filt, 'HeightMin', -8.0)
        put(filt, 'HeightMax', -3.0)
        put(entry, 'FilterOptions', filt)
        entries.append(entry)
    asset_set.set_editor_property('BiomeAssets', entries)
    save_asset(asset_set)
    receipt = {'map': MAP, 'layer': LAYER, 'source_fingerprint': initial_fingerprint,
               'sites': [], 'generation': 'pending', 'visual_review': 'pending'}
    for index, site in enumerate(sites):
        name = site['id']
        definition = duplicate('/PCGBiomeCore/BiomeDefinitions/DefaultBiome', 'DB_' + name.replace('-', '_'))
        data = definition.get_editor_property('BiomeDefinition')
        put(data, 'BiomeName', name)
        put(data, 'BiomeColor', u.LinearColor(.12 + .15 * index, .45, .1, 1))
        put(data, 'BiomePriority', 100 + index)
        definition.set_editor_property('BiomeDefinition', data)
        save_asset(definition)
        biome = spawn('BP_PCGBiomeSpline', 'Biome: ' + name, site['centre'])
        biome.set_editor_properties({'Definition': definition, 'Assets': [asset_set],
                                    'BlendingRange': 0.0, 'LocalCacheSize': 100.0})
        spline = biome.get_component_by_class(u.SplineComponent)
        spline.clear_spline_points(False)
        for p in site['points']:
            spline.add_spline_point(p, u.SplineCoordinateSpace.WORLD, False)
        for i in range(len(site['points'])):
            spline.set_spline_point_type(i, u.SplinePointType.LINEAR, False)
        spline.set_closed_loop(True, True)
        # Programmatic points need the same edited flag as the spline visualizer;
        # otherwise Blueprint reconstruction restores the template's 50m square.
        spline.set_editor_property('spline_has_been_edited', True)
        local_graph = biome.get_component_by_class(u.PCGComponent).graph_instance
        u.PCGGraphParametersHelpers.set_bool_parameter(local_graph, 'DisallowCollisions', True)
        u.PCGGraphParametersHelpers.set_bool_parameter(local_graph, 'LocalPreview', False)
        u.PCGGraphParametersHelpers.set_bool_parameter(local_graph, 'Debug-BypassFilters', False)
        core = spawn('BP_PCGBiomeCore', 'Biome Core: ' + name, site['centre'])
        core.get_component_by_class(u.BoxComponent).set_box_extent(site['extents'], True)
        pcg = core.get_component_by_class(u.PCGComponent)
        graph_instance = pcg.graph_instance
        u.PCGGraphParametersHelpers.set_bool_parameter(graph_instance, 'DisallowCollisions', True)
        u.PCGGraphParametersHelpers.set_bool_parameter(graph_instance, 'OutputDataForRuntimeGeneration', False)
        u.PCGGraphParametersHelpers.set_bool_parameter(graph_instance, 'ForceCreateTargetActors', False)
        exclusions = []
        for label, source in named.items():
            if not label.startswith('TC_Ground22_') or not label.endswith(('_support', '_entrance', '_footprint')):
                continue
            record = source_box(source)
            if not record:
                continue
            centre, extents, rotation = record
            radius = math.hypot(extents.x, extents.y)
            if abs(centre.x - site['centre'].x) > site['extents'].x + radius or abs(centre.y - site['centre'].y) > site['extents'].y + radius:
                continue
            exclusion = spawn('BP_PCGBiomeExclusionVolume', 'Biome clear: ' + label, centre, rotation)
            exclusion.get_component_by_class(u.BoxComponent).set_box_extent(
                u.Vector(extents.x + 225, extents.y + 225, 2000), True)
            exclusions.append(label)
        receipt['sites'].append({'id': name, 'core': core.get_path_name(), 'biome': biome.get_path_name(),
            'polygon': [KIT['vec'](p) for p in site['points']], 'exclusions': exclusions,
            'centre': KIT['vec'](site['centre']), 'extents': KIT['vec'](site['extents'])})
    if initial_fingerprint != fingerprint(actors):
        raise RuntimeError('Protected actor transforms changed')
    OUTPUT.mkdir(parents=True, exist_ok=True)
    (OUTPUT / 'installed-receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
    print(json.dumps(receipt))
    # Generation, audit, visual review and explicit saves are separate operations.


def restore_saved_boundaries():
    """Repair only confirmed template resets, retaining all generated instances."""
    if KIT['world']().get_path_name().split('.')[0] != MAP:
        raise RuntimeError('Open the saved detailed town centre')
    if u.EditorLoadingAndSavingUtils.get_dirty_map_packages():
        raise RuntimeError('Resolve unsaved maps first')
    receipt = json.loads((OUTPUT / 'installed-receipt.json').read_text())
    scene = list(u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors())
    named = {a.get_actor_label(): a for a in scene}
    before = fingerprint(scene)
    repairs = []
    # Validate every source and current boundary before mutating any component.
    for site in receipt['sites']:
        actor = named['Biome: ' + site['id']]
        spline = actor.get_component_by_class(u.SplineComponent)
        current = [KIT['vec'](spline.get_location_at_spline_point(i, u.SplineCoordinateSpace.WORLD))
                   for i in range(spline.get_number_of_spline_points())]
        segments = sorted([(int(m[1]), a) for label, a in named.items()
                           if (m := re.fullmatch('TC_Ground22_' + site['id'] + r'-loop_(\d+)_support', label))])
        source = [KIT['vec'](source_box(a)[0]) for _, a in segments]
        if len(source) != len(site['polygon']) or any(math.dist(a[:2], b[:2]) > .01 for a, b in zip(source, site['polygon'])):
            raise RuntimeError('Canonical park outline changed: ' + site['id'])
        if len(current) == len(site['polygon']) and all(math.dist(a,b)<.01 for a,b in zip(current,site['polygon'])):
            continue
        centre = actor.get_actor_location()
        template = [[centre.x+x,centre.y+y,centre.z] for x,y in [(-2500,2500),(-2500,-2500),(2500,-2500),(2500,2500)]]
        if spline.get_editor_property('spline_has_been_edited') or len(current)!=4 or any(math.dist(a,b)>.01 for a,b in zip(current,template)):
            raise RuntimeError('Boundary has possible hand edits; refusing to replace: ' + site['id'])
        repairs.append((site,actor,spline,current))
    for site,actor,spline,current in repairs:
        actor.modify(); spline.modify()
        spline.clear_spline_points(False)
        for p in site['polygon']:
            spline.add_spline_point(u.Vector(*p), u.SplineCoordinateSpace.WORLD, False)
        for i in range(len(site['polygon'])):
            spline.set_spline_point_type(i,u.SplinePointType.LINEAR,False)
        spline.set_closed_loop(True,True)
        spline.set_editor_property('spline_has_been_edited',True)
    if fingerprint(scene)!=before:
        raise RuntimeError('Protected actor transforms changed')
    (OUTPUT/'boundary-restoration.json').write_text(json.dumps({'sites':[{'id':s['id'],'before':c,'restored':s['polygon']} for s,a,b,c in repairs],
        'protectedFingerprint':before,'savedReopen':'pending'},indent=2)+'\n')
    print(json.dumps({'restored':[s['id'] for s,a,b,c in repairs],'generation':'retained; not regenerated','save':'pending'}))


def generate(site, stage='local'):
    if site not in SITES or KIT['world']().get_path_name().split('.')[0] != MAP:
        raise RuntimeError('Wrong site or current world')
    actors = u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors()
    if any(c.get_editor_property('generation_in_progress') for a in actors for c in a.get_components_by_class(u.PCGComponent)):
        raise RuntimeError('Wait for the active PCG generation first')
    if stage not in ['local', 'core']:
        raise RuntimeError('Generate local biome data first, then core after completion')
    label = ('Biome: ' if stage == 'local' else 'Biome Core: ') + site
    actor = next(a for a in actors if a.get_actor_label() == label)
    if stage == 'core':
        local_actor = next(a for a in actors if a.get_actor_label() == 'Biome: ' + site)
        if not local_actor.get_component_by_class(u.PCGComponent).generated:
            raise RuntimeError('Generate the local biome before the core')
    actor.get_component_by_class(u.PCGComponent).generate(True)
    print('Requested native biome generation: ' + site + ' / ' + stage)


def audit(receipt_name='generated-state.json'):
    """Read actual output and clearances; never regenerates or changes the world."""
    installed = json.loads((OUTPUT / 'installed-receipt.json').read_text())
    actors = list(u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors())
    named = {a.get_actor_label(): a for a in actors}
    result = {'map': KIT['world']().get_path_name(), 'engine': u.SystemLibrary.get_engine_version(),
              'source_unchanged': fingerprint(actors) == installed['source_fingerprint'],
              'sites': [], 'problems': [], 'instances': 0,
              'dirty_maps': [p.get_name() for p in u.EditorLoadingAndSavingUtils.get_dirty_map_packages()]}
    for site in installed['sites']:
        core = named['Biome Core: ' + site['id']]
        local_actor = named['Biome: ' + site['id']]
        local_pcg = local_actor.get_component_by_class(u.PCGComponent)
        row = {'id': site['id'], 'core': core.get_path_name(), 'instances': 0, 'components': [],
               'local_generated': local_pcg.generated,
               'core_generated': core.get_component_by_class(u.PCGComponent).generated,
               'debug_bypass': u.PCGGraphParametersHelpers.get_bool_parameter(local_pcg.graph_instance, 'Debug-BypassFilters'),
               'local_preview': u.PCGGraphParametersHelpers.get_bool_parameter(local_pcg.graph_instance, 'LocalPreview')}
        exclusion_boxes = [source_box(named[label]) for label in site['exclusions']]
        for component in core.get_components_by_class(u.InstancedStaticMeshComponent):
            mesh = component.static_mesh
            if not mesh:
                result['problems'].append('Missing mesh: ' + component.get_path_name())
                continue
            locations, ground_errors = [], []
            for i in range(component.get_instance_count()):
                t = component.get_instance_transform(i, world_space=True)
                p = t.translation
                locations.append(KIT['vec'](p))
                lowest = t.transform_location(u.Vector(0, 0, mesh.get_bounding_box().min.z)).z
                ground_errors.append(abs(lowest + 5))
                for centre, extents, rotation in exclusion_boxes:
                    a = math.radians(-rotation.yaw)
                    dx, dy = p.x - centre.x, p.y - centre.y
                    x, y = dx * math.cos(a) - dy * math.sin(a), dx * math.sin(a) + dy * math.cos(a)
                    if abs(x) <= extents.x and abs(y) <= extents.y:
                        result['problems'].append('Root on protected path/plot: ' + site['id'] + ':' + str(i))
            materials = [m.get_path_name() if m else None for m in component.get_materials()]
            collision = component.get_collision_enabled() != u.CollisionEnabled.NO_COLLISION
            nav = component.get_editor_property('can_ever_affect_navigation')
            if collision or nav or any(not m or 'WorldGrid' in m for m in materials):
                result['problems'].append('Invalid scenery component: ' + component.get_path_name())
            row['components'].append({'mesh': mesh.get_path_name(), 'count': len(locations),
                'collision': collision, 'navigation': nav, 'materials': materials,
                'maximum_ground_error_cm': max(ground_errors, default=0), 'locations': locations})
            row['instances'] += len(locations)
        if row['debug_bypass'] or row['local_preview'] or not row['instances']:
            result['problems'].append('Incomplete biome: ' + site['id'])
        result['instances'] += row['instances']
        result['sites'].append(row)
    if not result['source_unchanged']:
        result['problems'].append('Protected source transforms changed')
    (OUTPUT / receipt_name).write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps({**result, 'sites': [{**s, 'components': [{k: v for k, v in c.items() if k != 'locations'}
                        for c in s['components']]} for s in result['sites']]}))
    return result

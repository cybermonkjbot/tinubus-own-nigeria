"""Native editor controls for additive town-centre scenery and street review."""
import json
from pathlib import Path
import time

import unreal as u

MAP = '/Game/BaddieGeneratedWorld/StreetFinish_20261014/Maps/L_TownCentre_LibraryLane'
OWNER = 'Baddie.StreetFinish.20261014'
VIEWS = {
    'street': ((-58687.457699, -28530.809150, 165), (-5, -54.402859, 0)),
    'frontages': ((-56800, -30200, 165), (-3, 8, 0)),
    'reverse': ((-56200, -32300, 165), (-4, 125.6, 0)),
    'continuation': ((-55511, -33451, 165), (-3, -57.6, 0)),
    'overview': ((-58600, -32300, 4800), (-70, 33, 0)),
    'side_garden': ((-55100, -32020, 165), (-3, 39, 0)),
}
STREET_VIEWS = json.loads(Path(__file__).with_name('baddie_street_views.json').read_text()) if Path(__file__).with_name('baddie_street_views.json').exists() else {}
BIOME_ROOT = '/Game/BaddieGeneratedWorld/StreetFinish_20261014/Biomes'
BIOME_VIEWS = {
    'college-green': ((-36000, 48200, 165), (-3, 90, 0)),
    'neighbourhood-park-18': ((-65600, 33300, 165), (-3, 90, 0)),
}
FORECOURT_VIEWS = {
    'library-lane-2': ((-80125.882, -27353.454, 165), (5, -27.196207, 0)),
    'library-lane-4': ((-73724.867, -23176.360, 165), (5, -38.367078, 0)),
    'garden-avenue-5': ((-1625.337, 25226.563, 165), (5, 176.608323, 0)),
    'garden-avenue-7': ((-3410.759, 33519.030, 165), (5, 179.928736, 0)),
    'garden-avenue-14': ((-12312.012, 53352.643, 165), (5, 42.329718, 0)),
}
_biome_job = None


def view_forecourt(site):
    if site not in FORECOURT_VIEWS:
        return
    VIEWS['forecourt:' + site] = FORECOURT_VIEWS[site]
    view('forecourt:' + site)


def forecourt_actor(site):
    return next((a for a in generators() if a.get_actor_label() == 'Forecourt: ' + site), None)


def edit_forecourt(site):
    actor = forecourt_actor(site)
    if actor:
        u.get_editor_subsystem(u.EditorActorSubsystem).set_selected_level_actors([actor])
        u.get_editor_subsystem(u.AssetEditorSubsystem).open_editor_for_assets([actor.pcg_component.get_graph()])


def forecourt_visibility(site, visible):
    actor = forecourt_actor(site)
    if actor:
        actor.set_is_temporarily_hidden_in_editor(not visible)
        u.get_editor_subsystem(u.LevelEditorSubsystem).editor_invalidate_viewports()


def regenerate_forecourt(site):
    actor = forecourt_actor(site)
    if not actor:
        return
    scene = u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors()
    if any(c.get_editor_property('generation_in_progress') for a in scene for c in a.get_components_by_class(u.PCGComponent)):
        u.log_warning('Wait for the current PCG generation to finish.')
        return
    actor.pcg_component.generate(True)


def edit_ground_material(name):
    if name not in ['M_ParkMeadow', 'M_CompactedLaterite', 'M_CompoundPlaster']:
        return
    path = '/Game/BaddieGeneratedWorld/StreetFinish_20261014/Forecourts/Materials/' + name
    asset = u.load_asset(path)
    if asset:
        u.get_editor_subsystem(u.AssetEditorSubsystem).open_editor_for_assets([asset])


def biome_actors(site):
    world = u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
    if world.get_path_name().split('.')[0] != MAP or site not in BIOME_VIEWS:
        u.log_warning('Open the detailed town centre first.')
        return []
    labels = ['Biome: ' + site, 'Biome Core: ' + site]
    return [a for a in u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors()
            if a.get_actor_label() in labels]


def view_biome(site):
    VIEWS['biome:' + site] = BIOME_VIEWS[site]
    view('biome:' + site)


def edit_biome(site):
    boundary = [a for a in biome_actors(site) if a.get_actor_label() == 'Biome: ' + site]
    for actor in boundary:
        spline = actor.get_component_by_class(u.SplineComponent)
        spline.set_visibility(True, True)
    u.get_editor_subsystem(u.EditorActorSubsystem).set_selected_level_actors(boundary)


def open_biome_asset(name):
    asset = u.load_asset(BIOME_ROOT + '/' + name)
    if asset:
        u.get_editor_subsystem(u.AssetEditorSubsystem).open_editor_for_assets([asset])


def biome_visibility(site, visible):
    for actor in biome_actors(site):
        if actor.get_actor_label() == 'Biome Core: ' + site:
            actor.set_is_temporarily_hidden_in_editor(not visible)
    u.get_editor_subsystem(u.LevelEditorSubsystem).editor_invalidate_viewports()


def regenerate_biome(site):
    """Generate dependency then spawner, without blocking the editor or auto-saving."""
    global _biome_job
    if _biome_job:
        u.log_warning('Wait for the current biome generation to finish.')
        return
    actors = {a.get_actor_label(): a for a in biome_actors(site)}
    if len(actors) != 2:
        u.log_warning('This biome is not installed in the open map.')
        return
    all_actors = u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors()
    if any(c.get_editor_property('generation_in_progress') for a in all_actors
           for c in a.get_components_by_class(u.PCGComponent)):
        u.log_warning('Wait for the active PCG generation to finish.')
        return
    local = actors['Biome: ' + site].get_component_by_class(u.PCGComponent)
    core = actors['Biome Core: ' + site].get_component_by_class(u.PCGComponent)
    _biome_job = {'local': local, 'core': core, 'stage': 'local',
                  'started': time.monotonic(), 'next_poll': time.monotonic() + .5}

    def finish():
        global _biome_job
        if _biome_job:
            u.unregister_slate_post_tick_callback(_biome_job['handle'])
            _biome_job = None

    def tick(_delta):
        job = _biome_job
        if not job or time.monotonic() < job['next_poll']:
            return
        job['next_poll'] = time.monotonic() + .5
        try:
            if time.monotonic() - job['started'] > 180:
                u.log_warning('Biome generation has not completed; inspect the PCG component before retrying.')
                finish()
                return
            component = job[job['stage']]
            if component.get_editor_property('generation_in_progress'):
                return
            if not component.generated:
                u.log_error('Biome generation did not complete. Inspect the PCG log; the map was not saved.')
                finish()
                return
            if job['stage'] == 'local':
                job['stage'] = 'core'
                job['core'].generate(True)
            else:
                u.log('Biome generation completed: ' + site + '. Review before saving.')
                finish()
        except Exception as error:
            u.log_error(str(error))
            finish()

    _biome_job['handle'] = u.register_slate_post_tick_callback(tick)
    try:
        local.generate(True)
    except Exception:
        finish()
        raise


def open_street():
    # Never implicitly save or discard a user's unsaved map work.
    if u.EditorLoadingAndSavingUtils.get_dirty_map_packages():
        u.log_warning('Save or resolve unsaved maps before opening Library Lane.')
        return
    if not u.EditorAssetLibrary.does_asset_exist(MAP):
        u.log_warning('The Library Lane map is not installed in this checkout.')
        return
    u.get_editor_subsystem(u.LevelEditorSubsystem).load_level(MAP)
    view('street')


def view(name):
    world = u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
    if world.get_path_name().split('.')[0] != MAP:
        u.log_warning('Open the Library Lane map first.')
        return
    location, rotation = VIEWS[name]
    editor = u.get_editor_subsystem(u.LevelEditorSubsystem)
    key = editor.get_active_viewport_config_key()
    editor.set_level_viewport_camera_info(u.Vector(*location),
        u.Rotator(pitch=rotation[0], yaw=rotation[1], roll=rotation[2]), key)
    editor.set_level_viewport_fov(78, key)
    editor.editor_invalidate_viewports()


def view_street(street):
    camera = STREET_VIEWS.get(street)
    if camera:
        VIEWS['street:' + street] = (camera['location'], camera['rotation'])
        view('street:' + street)


def street_generators(street):
    return [a for a in generators() if 'Baddie.Street:' + street in [str(t) for t in a.tags]]


def edit_street(street):
    actors = street_generators(street)
    if actors:
        u.get_editor_subsystem(u.EditorActorSubsystem).set_selected_level_actors(actors)
        u.get_editor_subsystem(u.AssetEditorSubsystem).open_editor_for_assets([a.pcg_component.get_graph() for a in actors])
    else:
        u.log_warning('This street has no added eye-level generator; its clear route is retained.')


def street_visibility(street, visible):
    for actor in street_generators(street):
        actor.set_is_temporarily_hidden_in_editor(not visible)
    u.get_editor_subsystem(u.LevelEditorSubsystem).editor_invalidate_viewports()


def regenerate_street(street):
    for actor in street_generators(street):
        if not actor.pcg_component.get_editor_property('generation_in_progress'):
            actor.pcg_component.generate(True)


def generators():
    world = u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
    if world.get_path_name().split('.')[0] != MAP:
        u.log_warning('Open the Library Lane map first.')
        return []
    subsystem = u.get_editor_subsystem(u.EditorActorSubsystem)
    return [a for a in subsystem.get_all_level_actors()
            if OWNER in [str(t) for t in a.tags] and isinstance(a, u.PCGVolume)]


def select_generators():
    actors = generators()
    if actors:
        u.get_editor_subsystem(u.EditorActorSubsystem).set_selected_level_actors(actors)


def open_graph(name):
    matches = [a for a in generators() if a.pcg_component.get_graph()
               and a.pcg_component.get_graph().get_name() == 'PCG_LibraryLane_' + name]
    if not matches:
        u.log_warning('This dressing generator is not installed in the open map.')
        return
    graph = matches[0].pcg_component.get_graph()
    if graph:
        u.get_editor_subsystem(u.AssetEditorSubsystem).open_editor_for_assets([graph])


def visibility(visible):
    # Preview-only comparison: no actor deletion, graph cleanup or map save.
    for actor in generators():
        actor.set_is_temporarily_hidden_in_editor(not visible)
    u.get_editor_subsystem(u.LevelEditorSubsystem).editor_invalidate_viewports()


def regenerate():
    world = u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
    if world.get_path_name().split('.')[0] != MAP:
        u.log_warning('Open the Library Lane map first.')
        return
    for actor in generators():
        component = actor.pcg_component
        if not component.get_editor_property('generation_in_progress'):
            component.generate(True)


def register():
    menus = u.ToolMenus.get()
    menu = menus.extend_menu('LevelEditor.MainMenu.Tools').add_sub_menu(
        'BaddieStreetTools', 'Baddie', 'BaddieWorld', 'Baddie World')
    entries = [
        ('OpenLibraryLane', 'Open detailed town centre', 'open_street()'),
        ('StreetView', 'Street view', "view('street')"),
        ('FrontageView', 'Frontage view', "view('frontages')"),
        ('ReverseView', 'Reverse view', "view('reverse')"),
        ('ContinuationView', 'Street continuation', "view('continuation')"),
        ('Overview', 'Street overview', "view('overview')"),
        ('SideGarden', 'Side-return garden', "view('side_garden')"),
        ('PlantingGraph', 'Edit planting and shade', "open_graph('PlantingAndShadeFull')"),
        ('StreetEdgesGraph', 'Edit planted street edges', "open_graph('StreetEdges')"),
        ('EverydayGraph', 'Edit frontage props', "open_graph('Everyday')"),
        ('SurfaceGraph', 'Edit road surface repair', "open_graph('SurfaceSeparation')"),
        ('SelectGenerators', 'Select street generators', 'select_generators()'),
        ('RegenerateStreet', 'Regenerate street dressing', 'regenerate()'),
        ('HideDressing', 'Hide added scenery', 'visibility(False)'),
        ('ShowDressing', 'Show added scenery', 'visibility(True)'),
    ]
    for name, label, command in entries:
        entry = u.ToolMenuEntry(name=name, type=u.MultiBlockType.MENU_ENTRY)
        entry.set_label(label)
        entry.set_string_command(u.ToolMenuStringCommandType.PYTHON, '',
                                 'import baddie_street_tools; baddie_street_tools.' + command)
        menu.add_menu_entry('LibraryLane', entry)
    streets = menu.add_sub_menu('BaddieStreetTools', 'TownCentre', 'EyeLevel', 'Street-by-street detail')
    for street in STREET_VIEWS:
        submenu = streets.add_sub_menu('BaddieStreetTools', 'Streets', street.replace('-', '_'), street.replace('-', ' ').title())
        actions = [('View', 'Walking-height view', f'view_street({street!r})')]
        if not street.endswith('-bridge'):
            actions += [('Edit', 'Edit detail graph', f'edit_street({street!r})'),
                        ('Generate', 'Regenerate this street', f'regenerate_street({street!r})'),
                        ('Hide', 'Hide added eye-level detail', f'street_visibility({street!r}, False)'),
                        ('Show', 'Show added eye-level detail', f'street_visibility({street!r}, True)')]
        for name, label, command in actions:
            entry = u.ToolMenuEntry(name=name, type=u.MultiBlockType.MENU_ENTRY)
            entry.set_label(label)
            entry.set_string_command(u.ToolMenuStringCommandType.PYTHON, '',
                                    'import baddie_street_tools; baddie_street_tools.' + command)
            submenu.add_menu_entry('Actions', entry)
    forecourts = menu.add_sub_menu('BaddieStreetTools', 'TownCentre', 'Forecourts', 'Building forecourts')
    for site in FORECOURT_VIEWS:
        submenu = forecourts.add_sub_menu('BaddieStreetTools', 'Sites', site.replace('-', '_'), site.replace('-', ' ').title())
        for name, label, command in [
                ('View', 'Walking-height view', f'view_forecourt({site!r})'),
                ('Edit', 'Edit compound graph', f'edit_forecourt({site!r})'),
                ('Generate', 'Regenerate compound', f'regenerate_forecourt({site!r})'),
                ('Hide', 'Hide compound', f'forecourt_visibility({site!r}, False)'),
                ('Show', 'Show compound', f'forecourt_visibility({site!r}, True)')]:
            entry = u.ToolMenuEntry(name=name, type=u.MultiBlockType.MENU_ENTRY)
            entry.set_label(label)
            entry.set_string_command(u.ToolMenuStringCommandType.PYTHON, '',
                'import baddie_street_tools; baddie_street_tools.' + command)
            submenu.add_menu_entry('Actions', entry)
    for name, label in [('M_ParkMeadow', 'Edit park ground'), ('M_CompactedLaterite', 'Edit forecourt ground'),
                        ('M_CompoundPlaster', 'Edit compound finish')]:
        entry = u.ToolMenuEntry(name=name, type=u.MultiBlockType.MENU_ENTRY)
        entry.set_label(label)
        entry.set_string_command(u.ToolMenuStringCommandType.PYTHON, '',
            'import baddie_street_tools; baddie_street_tools.edit_ground_material(' + repr(name) + ')')
        forecourts.add_menu_entry('Materials', entry)
    biomes = menu.add_sub_menu('BaddieStreetTools', 'TownCentre', 'Biomes', 'Park biomes')
    for name, label in [('DA_UrbanParkPlants', 'Plant palette'), ('PCG_Canopy', 'Canopy density'),
                        ('PCG_Understory', 'Understory density'), ('PCG_Groundcover', 'Groundcover density')]:
        entry = u.ToolMenuEntry(name=name, type=u.MultiBlockType.MENU_ENTRY)
        entry.set_label(label)
        entry.set_string_command(u.ToolMenuStringCommandType.PYTHON, '',
            'import baddie_street_tools; baddie_street_tools.open_biome_asset(' + repr(name) + ')')
        biomes.add_menu_entry('Assets', entry)
    for site in BIOME_VIEWS:
        submenu = biomes.add_sub_menu('BaddieStreetTools', 'Sites', site.replace('-', '_'), site.replace('-', ' ').title())
        for name, label, command in [
                ('View', 'Walking-height view', f'view_biome({site!r})'),
                ('Boundary', 'Edit boundary', f'edit_biome({site!r})'),
                ('Generate', 'Regenerate biome', f'regenerate_biome({site!r})'),
                ('Hide', 'Hide vegetation', f'biome_visibility({site!r}, False)'),
                ('Show', 'Show vegetation', f'biome_visibility({site!r}, True)')]:
            entry = u.ToolMenuEntry(name=name, type=u.MultiBlockType.MENU_ENTRY)
            entry.set_label(label)
            entry.set_string_command(u.ToolMenuStringCommandType.PYTHON, '',
                'import baddie_street_tools; baddie_street_tools.' + command)
            submenu.add_menu_entry('Actions', entry)
    menus.refresh_all_widgets()

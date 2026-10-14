"""Native editor navigation helpers for the additive Library Lane detail pass."""
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
    matches = [a for a in generators() if a.get_actor_label() == 'Library Lane: ' + name]
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
        ('OpenLibraryLane', 'Open Library Lane', 'open_street()'),
        ('StreetView', 'Street view', "view('street')"),
        ('FrontageView', 'Frontage view', "view('frontages')"),
        ('ReverseView', 'Reverse view', "view('reverse')"),
        ('ContinuationView', 'Street continuation', "view('continuation')"),
        ('Overview', 'Street overview', "view('overview')"),
        ('SideGarden', 'Side-return garden', "view('side_garden')"),
        ('PlantingGraph', 'Edit planting and shade', "open_graph('PlantingAndShade')"),
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
    menus.refresh_all_widgets()

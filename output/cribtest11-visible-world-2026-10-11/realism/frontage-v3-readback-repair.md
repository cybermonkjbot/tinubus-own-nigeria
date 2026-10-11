October 11, 2026. Native imports and reviewed street authoring succeeded. The first separate readback failed without scene mutation because installed ISM Python exposes `static_mesh` as an editor property, not `get_static_mesh()`. The capture guard then correctly rejected the unprepared camera; no PNG was written.

Only the readback accessor was repaired:

```diff
- assert c.get_static_mesh().get_path_name().split('.')[0]==row['mesh']
+ assert c.get_editor_property('static_mesh').get_path_name().split('.')[0]==row['mesh']
```

Immutable import/map receipts retain their executed script identity. No import or map generation is replayed. Reversing this single-line patch reconstructs the executed authoring source for its recorded hash. Subsequent readback uses the repaired accessor. The previous bridge exception references are cleared by `safe.prepare_change` before any future level transition.

# Entrance crossing import recovery

We inspected PID 13584 on the exact Town Centre map. The first import failed its UV readback before actor placement. Only SM_market_road_4_0 and the parent map's current-level selection were dirty; no TC_Crossing37 actors existed. Existing street geometry remained unchanged.

Actual native UV0 was U=Xcm/271, V=1-Ycm/271, reflecting the installed OBJ importer's V flip. Native vertex bounds matched the expected centimetre/axis conversion. Automatic collision produced zero simple collision hulls for the shallow wedge. We corrected the UV contract and selected static triangle collision with CTF_USE_COMPLEX_AS_SIMPLE. This supplies static walking-surface collision input, not dynamic-physics or gameplay acceptance.

Recovery reuses only the inspected owned in-memory mesh, checks immutable OBJ bytes, and verifies all imported native vertices and UVs before saving and placement. No repeated import, reset, map discard or restart was used.

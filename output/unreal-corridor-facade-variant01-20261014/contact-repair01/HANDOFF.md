# Contact Repair01 Failure Evidence

October 14, 2026. Cribtest11, `C:/dev/cribtest-game`. Start HEAD `18bbe0f47bb9b7cbcbaf48dad9297f6a6169deee`; endpoint is the commit introducing this child handoff. Reviews remain deferred, not accepted or resumed. Requested gpt-6.1-sol/medium; host-selected model/effort not independently verified/switched. Same owner/chat. Child directory was absent before creation; all parent16 files remain immutable.

## Measured Cause

One authorized isolated Blender5.2.2LTS background process7424 loaded the original `CorridorFacadeVariant01.blend` read-only from disk, scene `CorridorFacadeVariant01`, Cycles CPU/24samples/six threads. Exact input SHA256 `612c543060865b4f4bb73cbc11e88c5f7bff5d8de74b49ed4a70ed4a9d10387e`. Identity before mutation is retained in `input-identity.json`. Production/control/realism/qa instructions and source were read before launch; no original generator executed.

`contacts-before.json` contains evaluated meshes, bounds, outward normals and material slots. At X0 and X3.900000095m, each return shares an outward-facing planar rectangle with both UpperFloorSlab and RoofSlab. Measured YZ overlap0.643999964m by0.174000025m at midstorey, area0.112056010m2; roof0.643999964m by0.174000263m, area0.112056163m2. Both normals are -X on left and+X on right. Four coplanar side overlaps are observed, not merely inferred from appearance. AABB intersections are separately recorded and are not general solid-intersection proof.

## Failed Repair

After inspecting those measurements, `repair-authorized.json` permitted splitting only the return solids at measured slab bottom/top, preserving X/Y extents, material slots and other objects. Two return meshes were rebuilt in memory as closed component boxes with recalculated normals; their all-edge bevels disabled to retain exact butt-joint caps. The post-repair coplanar-side assertion was reached and passed, but this intermediate state was NOT persisted and is not acceptance evidence.

The next assertion incorrectly required the overall multi-component return AABB to have zero intersection with slab AABBs. A split return's enclosing AABB still spans the removed slab intervals; that assertion cannot establish solid overlap. It failed at repair_contacts.py163 before contacts-after receipt, protected readback, save/reopen or first preview. `production-error.log` retains the exact traceback. Process7424 exited; no corrected blend, AFTER images or complete measured contact repair exists. No original was saved or changed. No extra process/retry was launched under the one-process limit. Do not rerun this child over its exclusive receipts.

Needed continuation, not performed: separately authorized one-process retry in a fresh exclusive child; measure each connected component's contact intervals rather than an enclosing AABB, record butt-contact gaps/areas and evaluated manifold/normals, preserve failed receipts. Then save/reopen and inspect matched grazing/reverse/player165 plus wide images. The incomplete source here retains the exact failure, not an unexecuted correction disguised as a successful job.

## Preservation And Limits

Original hashes before are in `original-hashes-before.json`; after exit all16 parent files were individually SHA256-compared to it and all matched, including original blend/hash above. `original-preservation.json` records the comparison result without claiming reconstructed geometry. Existing Blender6116 read-only MCP identity remained `LagosWorkbench.blend`, `BaddieAssetWorkbench`, Cycles,5.2.2LTS,idle; Unreal10404/session2 process remained unchanged. No Unreal API, map mutation/import, BodySetup audit, collision/navigation or guards touched.

All dimensions/axes/source distinctions remain in unchanged parent contract:3.9m bay,3m storey,two storeys,.25m wall,1.3x1.35m window,2.8x2.5m shop aperture. No parcel assignment, materials/light/exposure/camera concealment or invented room. Visible black bands remain unresolved in the original rendered evidence. This deliverable is failure/measurement evidence only, not corrected art/native coverage/gameplay/performance/release acceptance.

Checkpoint only new child script,instruction,identity,original hash record,measured before geometry,logs,preservation record and handoff. Hooks inspected(samples only/no configured hooksPath), disabled for commit along with maintenance/GC. Unrelated dirty uproject/Config/groundhelper/nativegenerator/portdoc preserved. No tests/lint/typecheck/build/C++/HLOD/PIE/bake/film/benchmark/export/conversion/install/restart/server/download/release/Mac implementation/new chat/writer.

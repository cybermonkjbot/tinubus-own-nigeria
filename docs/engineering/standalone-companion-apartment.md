# Connected Home apartment

October 10, 2026. Joshua's updated Home direction and source implementation record.

## Required layout

Each user's own saved rooms open onto a hallway. The hallway leads into the living room, and the living room leads outside the apartment. The living room gets abundant daylight through large exterior-facing windows. Carry daylight toward the hallway entrance and keep an outside view from the seating area. This is a continuous walking route, not a menu that teleports between disconnected rooms.

This is one private apartment per user. It does not connect different accounts' Homes or grant visitors access. The exterior threshold leads to the apartment's landing or outside approach; entering the Lagos world still requires its existing game admission.

Preserve saved room IDs, names, furnishings, support heights, appearance and design rotations. Assemble complete room instances around the hallway with native transforms. Do not rewrite furniture coordinates, regenerate decor, stretch objects or apply the room's saved quarter turn twice. Kitchen and bathroom design extensions enlarge only their empty front strip.

If the supplied inventory includes the canonical `home-living` room, it is the main living room. Keep its saved contents rather than adding a duplicate lounge. Do not infer a room's identity from its user-editable name. An absent `home-living` in the current one-room grant does not prove that the account has no living room. Resolve the complete authorised inventory before assigning or creating a common-space living room.

## Unreal implementation boundary

Use native level instances for authored room/common-space modules, with doorway anchors and whole-instance transforms. Use native character movement and navigation across those doorways. The apartment's adjacency data is Baddie-specific; it is not a replacement scene loader, pathfinder or collision solver.

Use native window geometry, sky/sun lighting and exposure for the bright living room. Do not substitute a bright background or white unlit material for exterior light entering the room. Select the actual engine version and inspect the authored assets before binding version-specific lighting features. Final materials, exterior view and daylight need player-height rendered inspection.

The measured bedroom and studio doorway sources are in `packages/utils/src/companionHomeDoor.ts`. That function reports no authored doorway for the kitchen, bathroom or legacy layouts. Its fallback standing point is not a physical doorway. Those native shells need authored openings before a walk-through connection can be claimed. Navigation clearance and safe fallback spawn remain unresolved.

## Current source

The apartment mapping at Mac `a377f2ecbcd548a4a0b9cf2e9e755309368378f2` and support-policy follow-up reached Windows `3162fdf7f0fc07bb69c4b460c074b350cc451dd0`. The reviewed native corrections, apartment session acceptance and explicit controls reached Windows together at `87295378a1ce93db180a879b87909dc7de3d050f`. The parent independently verified all 81 delivery/reference hashes and a clean Unreal worktree at 13:29:59 UTC on October 10. The connected environment is not complete.

`BaddieCompanionLayoutMapping.cpp` extends the existing saved-room plan to Pat's studio, bedroom v01, kitchen, bathroom, legacy studio and terrace. Bedroom v02 keeps its measured mapping. It preserves floor/support anchors, walls/openings, camera references, fixed camera housings, preferred spawn, extension metadata and quarter turns. Legacy rooms retain catalog body dimensions and their original placement envelope. Terrace retains horizontal supports without indoor walls.

`ReadSnapshotSelection` now retains all rooms actually supplied in the snapshot. `BuildHomePresentationPlan` checks every supplied room, reconciles Home/active selections, rejects duplicate or divergent identities, and includes the room array in exact-input caching. Existing selection-only C++ callers retain their known Home/active rooms without invented inventory. The controller's existing validity boundary clears these plans with the rest of the private snapshot.

`BaddieCompanionApartment.h/.cpp` adds the required two-way adjacency to that existing plan. Supplied rooms connect to the hallway; the hallway connects to the living room; the living room connects to the exterior landing. A supplied `home-living` is reused as that living-room endpoint. Native doorways, common-space modules, lighting, complete inventory and exterior admission remain unverified, and `IsWalkThroughReady()` returns false. This is connected-apartment data, not a connected rendered environment.

The existing `companion_home_read` grant supplies exactly one pinned Home room. Its server projection and Unreal session's Home-scope one-room check are unchanged. The original apartment plan added no grant or authority. The separate source follow-up in `standalone-companion-apartment-access.md` adds explicit apartment consent, a pinned all-room projection, Unreal session acceptance and controller/Slate controls. Only a validated apartment response marks complete inventory in the existing presentation plan. The Unreal source is delivered; backend/mobile source remains with the existing Mac owners. Neither scope adds room writes, extra room entitlements, world admission or enabled live access.

## Authored opening follow-up

Source inspection after the combined delivery found that every saved-room entrance still had to pass `CopyCanonicalNativeDoorway`. That made kitchen, bathroom and legacy entrances impossible to bind even with real native openings, because the canonical source has measurements only for the bedroom/studio layouts.

The native owner now accepts an explicit `AuthoredLocalPose` for those entrances and for common-space connections and the living-room exterior exit. The pose records real door/approach actor positions in native level-local centimetres. It already includes import basis and the saved shell design, but not the whole-apartment transform. The binding still requires the exact supplied presentation and real actor names. It never derives an opening from a source standing candidate or rewrites saved furniture coordinates.

Measured bedroom/studio entrances retain their canonical coordinates and quarter-turn handling. They require `SourceFloorYMetres` and reject authored overrides, including when canonical validation fails. All other openings require the authored pose and reject the canonical-height field. Both paths clear output on failure. Preparation rejects missing/non-finite/coincident poses and transformed overflow. Loaded-actor inspection checks every binding against the expected local pose, regardless of whether Unreal has already applied the instance transform. Existing duplicate-anchor and cross-endpoint agreement checks remain intact.

This follow-up is Mac source pending guarded Windows review/delivery. Source review traced all five authored layouts, legacy studio/terrace, reused `home-living`, invalid measured input and both native transform states. No native content, tests, compilation or walk-through inspection ran. Real openings and navigation clearance remain outstanding; saved-render readiness and gameplay admission remain closed.

## Source review

Parent source review traced the five authored layouts and both legacy environments against `companionRoomLayouts.ts`, `companionHomeDesign.ts`, `companionPlacement.ts`, `companionRoomItems.ts`, `companionAssets.ts` and the authored/personal/Home-design environment sources. It covered source-floor anchor reuse, camera-only housings, extension-before-rotation ordering, absent portrait framing, legacy body sizes and unassigned-prop bounds. Bedroom v02 retains its existing measurements and conversion, with room-wide type/member names.

The apartment review covered the requested connection order, reuse of `home-living`, selection/list identity consistency, case-sensitive duplicates, same-revision room changes and inherited access-loss clearing. The session's single-room response guard, retry expression, movement/protocol/clock source and configuration are unchanged. Source and whitespace inspection ran; no tests, compilation or rendered verification ran. The current preview does not render these new plans.

## Remaining work

1. Prepare the existing compressed assets for the installed Unreal importer without changing their saved identities or the production renderer. Use `standalone-companion-unreal-assets.md` for the exact sources and import prerequisites. The native corrections and apartment controls/session handoff are complete; live access remains disabled.
2. Author native hallway, living-room and exterior modules. Bind supplied saved-room content and real doorway openings, preserving furniture and room-local coordinates.
3. Use the existing Unreal camera, movement and navigation owners for room-to-hallway, hallway-to-living-room and living-room-to-exterior traversal. Bind exterior game admission separately.
4. During the authorised engine/content pass, inspect connected spaces, doorway clearance, daylight, exterior views and all saved room variants. No installation, import, engine launch, compilation or release is implied by this source patch.

The production mobile/web renderer, asset catalog and backend contracts remain unchanged. No packaging or What's New update is required for these standalone source changes. Compilation, rendered verification and release remain outstanding.

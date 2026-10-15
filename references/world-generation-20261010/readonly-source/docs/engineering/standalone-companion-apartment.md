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

The coordinator reviewed Mac `86315dba6dd6a95490c0d7586f9a4ed6e9f79d8f` and delivered it to Windows `fe64f51d937aaee85aae5303b59543f89abe94c5`. Its record at Mac `889f3cf4b` reports 82 matching working/committed file hashes and a clean Windows tree. The parent has read that record but has not independently repeated this follow-up's remote readback. Source review traced all five authored layouts, legacy studio/terrace, reused `home-living`, invalid measured input and both native transform states. Real openings and navigation clearance remain outstanding; saved-render readiness and gameplay admission remain closed.

## Saved furniture continuation

The native apartment owner now has a separate saved-furniture path. Authored room levels supply shells and fixed fixtures, not a baked copy of each user's item arrangement. A trusted native mesh binding identifies each item's existing geometry recipe. The owner uses Unreal async loading and creates one transient native actor per saved item in its room's streamed level. Each actor keeps the saved ID, support-derived position, wall-bottom pivot, yaw and object scale. Before Unreal's level-transform phase, it uses saved room-local coordinates and retains the expected apartment pose. After that phase, the owner reconciles component transforms once through native setters. It never repeats the saved shell turn or snaps a saved item to avoid collision.

The actor uses Unreal static mesh, point-light and dynamic-material components. Existing lamp positions, intensity units, range and saved `lit` values are retained. Floor/desk lamps require one unambiguous native bulb material slot and a real scalar parameter. Saved props keep their own materials rather than receiving the room palette. Fans remain static and candles unlit, matching the current geometry factories. Scene power/outages and object interactions are not implemented by this slice.

Missing meshes, recipe mismatches, failed asset loads or invalid material bindings clear preparation. Access loss, replacement input and teardown cancel pending loads and clear actors, lights and private IDs before native level unload. Furniture remains hidden and non-colliding; this source path does not open admission or render readiness. Native mesh/material content, placement collision and visible activation are still missing. Source review and exact installed-engine references are recorded in `cribtest11-native-furniture-api-2026-10-10.md`. The furniture continuation reached Windows `801fe8f773c2da70783d25185aea2fcf116e29f3`; `cribtest11-queued-source-delivery-2026-10-10.md` records its 22-file handoff with the other queued slices. Those sources remain uncompiled.

Parent source review traced item creation against `personalEnvironment.ts`, `studioGeometry.ts`, `shopGeometry.ts` and the existing furniture mapper. It covered all catalog kinds, saved lamp state, duplicate material slots, asset-handle retention, both level-transform phases and inherited access-loss clearing. The research found the pre-registered component transform risk; the implementation now handles it with native transform updates. No tests, compilation, imports or rendered inspection ran. The existing controller, session, movement source, account flags and production renderer are unchanged by this continuation.

## Saved character continuation

The apartment owner now has one private `CharacterActor` in the saved Home's streamed level. The source uses an explicit, trusted authored native class for the skeletal/static hierarchy, a shared source key/hash and real named material bindings. Unreal's asset manager retains the class/mesh/material references until preparation finishes. `CharacterMaterials` applies the existing colour requirements to the completed actor's own components. No GLB hierarchy is rebuilt in C++ and no single-skeleton assumption is made.

The actor uses the source preferred Home pose and character scale. Native transform reconciliation handles spawning before or after the level's instance-transform phase. It stays hidden, non-colliding and without gameplay tick, input or possession. Replaced input, failed preparation, unregister and teardown cancel loads and discard private content. Material restoration and presentation callbacks defer unsafe mesh disposal and actor destruction through a weak native core ticker. Pending destruction is not reported as completed destruction. Reentrant owner clearing invalidates the outstanding spawn; it cannot adopt that actor into a newer selection.

The accessory continuation applies the saved necklace and hip-chain choices through this same actor. Real native mesh bindings and an accepted per-occurrence enabled mask preserve hidden descendants without toggling an entire attachment subtree. Native equipment and generated eyewear remain unimplemented. `unreal-accessory-visibility-reuse-2026-10-10.md` records the source precedence, binding contract and installed visibility behavior. The corrected actor/accessory source is pending review and handoff after the initial actor transfer was held for unsafe restoration-time cleanup.

This source continuation is pending review/handoff. The actual native class, import/morph/hierarchy acceptance, material graphs, wardrobe/face behavior, animation, safe spawn and admitted movement remain unfinished. The existing closed readiness/admission checks remain before loading or spawning. Read `unreal-native-avatar-ownership-2026-10-10.md` for the source review and installed API evidence. No tests, compilation, imports, editor launch or rendered verification ran for this continuation.

## Source review

Parent source review traced the five authored layouts and both legacy environments against `companionRoomLayouts.ts`, `companionHomeDesign.ts`, `companionPlacement.ts`, `companionRoomItems.ts`, `companionAssets.ts` and the authored/personal/Home-design environment sources. It covered source-floor anchor reuse, camera-only housings, extension-before-rotation ordering, absent portrait framing, legacy body sizes and unassigned-prop bounds. Bedroom v02 retains its existing measurements and conversion, with room-wide type/member names.

The apartment review covered the requested connection order, reuse of `home-living`, selection/list identity consistency, case-sensitive duplicates, same-revision room changes and inherited access-loss clearing. The session's single-room response guard, retry expression, movement/protocol/clock source and configuration are unchanged. Source and whitespace inspection ran; no tests, compilation or rendered verification ran. The current preview does not render these new plans.

## Remaining work

1. Prepare the existing compressed assets for the installed Unreal importer without changing their saved identities or the production renderer. Use `standalone-companion-unreal-assets.md` for the exact sources and import prerequisites. The native corrections and apartment controls/session handoff are complete; live access remains disabled.
2. Author native hallway, living-room and exterior modules. Bind saved-room shells, fixed fixtures, real doorway openings and the separate saved-furniture mesh catalog, preserving room-local coordinates.
3. Use the existing Unreal camera, movement and navigation owners for room-to-hallway, hallway-to-living-room and living-room-to-exterior traversal. Bind exterior game admission separately.
4. During the authorised engine/content pass, inspect connected spaces, doorway clearance, daylight, exterior views and all saved room variants. No installation, import, engine launch, compilation or release is implied by this source patch.

The production mobile/web renderer, asset catalog and backend contracts remain unchanged. No packaging or What's New update is required for these standalone source changes. Compilation, rendered verification and release remain outstanding.

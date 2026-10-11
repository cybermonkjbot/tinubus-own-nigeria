# Native Home invitations and guest-read source assignment

October 11, 2026. Existing owner `01a126d5-064b-7703-a27e-31991c45a302`, host local. Use `gpt-6.1-sol` explicitly and preserve reasoning effort.

## Allocation

Translate the existing `companion-homes.ts` local state, endpoint values and explicit invitation intents. This is part of the full port, not a replacement for the active world/publication work. Source/dependency base is `de632cb19d5916318b3a45494863b625e2342207`.

Only these new paths are writable, all verified absent before allocation:

```text
null  apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionHomesState.h
null  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionHomesState.cpp
null  docs/engineering/unreal-homes-native-handoff-2026-10-11.md
```

All previous domains and handoffs remain frozen. Parent owns assignment, tracker, acceptance and serialization. The coordinator alone owns Windows writes. The publication owner exclusively retains its thirty source paths, new publication pair and handoff. StateJson, Snapshot, room/appearance/furniture mapping, GameplaySession, controller, presentation, Build.cs and renderer inputs remain read-only. No native source or consumer outside the three new paths is allocated.

Use Unreal's existing JSON reader/writer, containers and the immutable shared StateJson helpers. Reuse `FRoomSelection` from Snapshot.h as the preserved room carrier, not `FSnapshotSelection` or its private apartment provenance. Keep this deep module's caller-driven state independent of actor/geometry mapping. An invited room must later use the reviewed mapper; this assignment adds no geometry, collision, transforms, assets, streaming or native permission. It must not depend on the publication owner's evolving implementation.

## Canonical contract and fidelity

Read the pinned account store, backend endpoint, room schema, saved-room types and actual Home consumers. `invitations` returns required `received` and `sent` arrays of `{id, handle, roomId, roomName}`, each at most fifty. Preserve source order and the owner's `Unavailable account` / `Removed room` placeholders so revocation remains possible. Missing data is not an empty success. Reject over-cap or malformed values atomically, without publishing a prefix. Identities are exact, case-sensitive and never trimmed or truncated. Do not require an outgoing placeholder to be a valid contact handle.

`room` returns exactly `{room: PersonalRoom, revision, ownerHandle}` for the selected invitation. Retain the complete saved room JSON, room identity/name/environment and optional layout. Preserve all five layouts, both environments, legacy layout/design omissions, palettes, light, sharing, four optional colors, version-one turns/extension and all twenty-three furniture kinds. Preserve item order/IDs, exact source x/z/rotation, optional y/surface/lit and absent versus explicit values. Keep primitive/enum/array and optional-field checks at the endpoint-value seam. Structural acceptance is not geometric placement, entitlement or render acceptance; do not duplicate placement/layout solvers. The full room object, not a synthesized default, supplies RoomJson. No private library, Companion appearance, unrelated room or entitlement record belongs in a guest response.

Use explicit decoder limits before UE deserialization, bounded array/string checks and finite/safe-number checks appropriate to the canonical contract. Document each bound and its relationship to existing limits. Preserve values rather than narrowing valid canonical payloads to a convenient subset. Optional null is not omission. Malformed/oversized input clears decoder output; preserve an aliased input before clearing old output. Decode into a fresh candidate and publish only after the entire response succeeds. Root/null, unknown-key and error behavior must be explicit. Reuse shared `ReadBoundedArray`, `Only`, `EnumValue`, integer/number/string/boolean and raw-null helpers where applicable, without editing them.

`setInvitation` describes `{handle, roomId, enabled}` and `revokeInvitation` describes `{invitationId}`. Both return null. Encode through UE serialization with bounded argument strings; disabled invitations still use setInvitation, not an invented operation. Store no sessionToken/device proof or permission boolean. Endpoint strings are `companionHomes:invitations`, `companionHomes:room`, `companionHomes:setInvitation`, `companionHomes:revokeInvitation`. These original account endpoints require the original non-TV account/session/device adapter. Current Home/apartment/gameplay/world grants cannot dispatch these descriptions. Backend contact, sharing, owner/guest, rate, count and entitlement checks remain authoritative.

## Ownership and local lifecycle

Provide a single-thread caller-driven state with private generation receipts and immutable captured intents, following the existing native state conventions. No actual subscriptions, HTTP requests, timers, credentials, transport retries or readiness inputs. Captured intents are local descriptions only, not proof of dispatch, authorization or action acknowledgement.

Every BeginOwner replaces the original account/session lifetime, including the same user ID. Separate list-subscription/read generation from selected-visit subscription/read generation. Capture read receipts before request or ordered subscription ingress. Only the exact current receipt may publish or fail; older callbacks cannot repopulate a replaced account, list or visit, clear newer errors, or release newer busy work. Stop before external unsubscribe. Last-observer removal, explicit feature disablement and Dispose invalidate relevant reads and clear private list/guest-room state. Caller owns actual abort/unsubscribe. Dispose remains inert until a deliberate new owner lifetime.

Selecting another invitation, selecting the same invitation again, leaving a visit or stopping its observation immediately retires and clears the old guest response before any new result. Expose loading/error states for list and visit distinctly. A list-read failure clears that list; a visit-read failure clears the guest room. Never substitute the user's own room while a guest read is pending or failed. Keep list replacement independent of visit subscription ordering. A local invitation list is descriptive and is not an authorization proof for a selected visit; the server's current room read supplies that decision. No list or room receipt grants world travel or furniture editing.

Exactly one account mutation intent is busy at a time. Both invitation operations are account-bound, not world/room-visit-bound, and must survive travel or list/visit replacement while the original owner remains current. Exact null success releases only that intent. Do not optimistically insert/remove invitations, rewrite guest data or infer that a refreshed list has arrived. Command failure or malformed success retains read values and exposes a bounded fixed command error. Exact cancellation releases only its pending request; no automatic retry. Owner replacement/disposal invalidates outstanding command receipts and clears busy/errors. A late old completion cannot settle a new command. Preserve query/command error visibility with separate fields rather than losing one to an unrelated callback.

Home visits expose one saved Personal Room read only. They are distinct from shared-call room copies and the owner's complete apartment. Private rooms still lead through that user's private hallway to the daylight living room and outside in the separately owned apartment work. No invitation list, matching handle, path or local receipt connects accounts physically, enables outside travel, care, capture, payment, editing, movement or other gameplay.

## Immutable references

All twenty-two reference hashes matched working and committed bytes at allocation. These pins include actual consumers, shared helpers and the private-scope contract. Additional indispensable read references may be inspected and recorded, but added writable paths require parent allocation.

```text
bba0c912518dbb8c579145ede3a0ef95320ed918422e6f717d2ca96016615e1a  AGENTS.md
4d8455c0c76e08d7243670e6c97f277e083dd13daf8c9ac59b30d55d1250912d  CONTEXT.md
cc36f0126c617babb0e259523102f6398551146b407e71aed9a3f6ae38d4ad74  packages/api/src/companion-homes.ts
3395257ec7d9d82c5696f48af939aad8cc2f165079740c62904000fcf3e7b9a1  apps/backend/convex/companionHomes.ts
8bba23bf9d6e79d10d802fd82a02f320bd46c22f69cd0a930a714ff22bb51475  packages/utils/src/companion.ts
2a2309d984d9ffc36d264f386c413fc6a2fa6695c6e0459dd30ebcd7dace7d67  packages/utils/src/companionHomeDesign.ts
9d3a5cd46d9a5c13b54cf3a1402a6e62f166cb5c2a74679c41b889bba83e9c2e  packages/utils/src/companionRoomLayouts.ts
885be516c068f732f56ea93cdcfed1b9230b87ae7aebafd2d047455ec27e1a4b  packages/utils/src/companionRoomStyle.ts
e384593cb48434e7681dd1f2c0a3f7ceb75a1e02e1aa187177c19ccac942c1d3  packages/utils/src/companionRoomItems.ts
1c46ba2566d0f806974a0617e685e2020b100d39c3a005a94ff71cc2864ee6d9  apps/backend/convex/lib/companionModel.ts
c62a4568d42547274e7d1291a4c452aba7c04cf435d6df23d7c550f2e6075791  apps/backend/convex/lib/companionRoomInventory.ts
cc5c44c5e03386bc871959c03568b0af71e96935a73108dcca748b6a262d292f  apps/mobile/src/services/companionGameService.ts
335328dffc76a3a89766f192f6fe78ede06d71323548df4cbe77211b07bf40e4  apps/mobile/src/features/companion/CompanionSpaceView.tsx
ec1b9857411b8963f084de7706c0994075af81fb76d5cc0d36912a3fbc2c9a67  apps/mobile/src/features/companion/CompanionLifeSheet.tsx
13ceb1b13bee4b0793ddb117e03559bfb8ea77f8bdb8dc81baa1ffa8e70bb618  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionStateJson.h
df02a8dbf13c9f23ce11f57d0584c96c6a31c2cbe682926ae10be5bb6f3021ce  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionStateJson.cpp
1b41894ec0ee2c500a7078690cf74460f136240ef1a212715c580c3a92b53061  apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionSnapshot.h
785554493a6558828db827c8e338b3db281e66514f53908f3815402274fa533c  apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionContentHouseState.h
b52330c17310d442a7e0eaa0f9a8a032fd290b47a1c8d1d474680ba4312304a7  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionContentHouseState.cpp
d18b2205fe0d07c7c399c723a12f24be1c38cb2c692a821125e900eee0590c37  docs/engineering/standalone-companion-account-linking.md
e11c569c762cd86a54bf98cbd38f964f0b76e6456c858a1b2a99cdf67cb9a31e  docs/engineering/standalone-companion-unreal.md
7034c41a675fce8da7a6733547d4743a6ecbafb0a44bec6c41db1d409adfd2ea  docs/engineering/unreal-world-publication-assignment-2026-10-11.md
```

## Source completion evidence

Use codebase-design, writing-for-agents and unslop. Read global/root instructions and applicable linked contracts completely. Use apply_patch. Inspect complete source/diff and the shared index, checkpoint only the two owned source paths with hooks disabled, then write/checkpoint only the new handoff. Preserve unrelated publication, generated API, project configuration and media/output work.

Walk all four endpoint values/intents; empty/populated/fifty/over-cap lists; placeholders; malformed roots/fields/nulls; every optional room/design/furniture field and legacy omission; source order/exact values; output aliasing; owner/list/visit same-ID replacement; last-observer removal; travel; concurrent read and command; one busy command; stale callbacks; null success, failure, cancel and disposal. Record reference matches/drift, output hashes, exact source/end/handoff commits, limits, call graph, independent account/list/visit lifetimes and any unimplemented adapter seam. Parent reviews the complete three-path range on both axes before delivery.

No tests/fixtures, business-code execution, lint/typechecks, builds/compilation/codegen, guards, server/engine/simulator launches, installations, assets/generation/import, live calls, configuration/deployment, remote operations, new chats/agents or other-chat messages. This native-only source slice changes no mobile/shared renderer input and triggers no packaging or What's New update. All gates remain false. Source implementation, independent review, Windows delivery, transport/UI/mapping integration, content, compilation, in-game acceptance and release stay separate. Do not declare Home visits or the full port complete.

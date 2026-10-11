# Native Home invitations and guest-read handoff

October 11, 2026. Source-only delivery for owner `01a126d5-064b-7703-a27e-31991c45a302`. Parent owns acceptance and allocation; the coordinator owns Windows delivery.

## Delivery pins

- Assignment `docs/engineering/unreal-homes-native-assignment-2026-10-11.md`, commit `6a365b302792e84fc3fa666ab914f007769d2622`, SHA-256 `47f97ddfaa3103811c67aa452340acd320c01df2435f540e60d1d175a0844cee`.
- Source/dependency base `de632cb19d5916318b3a45494863b625e2342207`.
- Source and source-end commit `c202a253cc95aa84df7654c8e83fdeffeb86fa8a`. This commit contains only the new Homes header and implementation.
- This handoff is a separate, single-path commit after the source commit. Its exact commit and SHA-256 accompany the delivery response rather than creating a self-referential document hash.

All three allocated paths were absent before implementation. Only those paths were written. Shared StateJson, Snapshot, mappings, GameplaySession, presentation, controller, Build.cs, publication work and previous domain handoffs remain unchanged by this assignment.

Source output SHA-256 values:

```text
33998d67d1427ca2840b01d8196e76088fdb1c20fab4ed4540e56175050b814a  apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionHomesState.h
3e3a4103ca86e53e85db0e4f1e6ff17370ce5ac1eb9978edf8ce4de27a971773  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionHomesState.cpp
```

## Module and adapter seams

`FHomesState` is single-threaded and caller-driven. Its private GUID receipts bind an owner lifetime, independent list and visit subscriptions, their latest captured reads, and one account command. Published endpoint values use `TSharedPtr<const ...>`. Intents expose only operation kind, endpoint string and serialized non-auth arguments. Their `IsValid()` means a description was captured, not that it remains current or that dispatch is permitted.

| Entry | Local description or result |
| --- | --- |
| `BeginList` then `Read` | `companionHomes:invitations`, `{}` |
| `BeginVisit` then `Read` | `companionHomes:room`, `{invitationId}` |
| `SetInvitation` | `companionHomes:setInvitation`, `{handle, roomId, enabled}` |
| `RevokeInvitation` | `companionHomes:revokeInvitation`, `{invitationId}` |
| `AcceptRead` | Atomic list or saved-room endpoint-value decoding |
| `AcceptReadFailure` | Retire the exact read and clear only its value |
| `AcceptNullResult` | Accept raw JSON null for the exact account command |
| `AcceptCommandFailure` or `Cancel` | Settle only the matching pending receipt |

The future caller creates an owner receipt for the original account/session lifetime, creates each needed subscription, and captures a fresh `Read` receipt before a request or ordered subscription ingress is processed asynchronously. Each receipt settles once. Later subscription ingress needs a new capture. Capturing a read replaces only that read's previous request, without clearing accepted data or the other read's error. Initial observation and new visit selection set their distinct loading flags.

Local flow is `BeginOwner -> BeginList/BeginVisit -> Read -> AcceptRead/AcceptReadFailure`. Command flow is `SetInvitation/RevokeInvitation -> AcceptNullResult/AcceptCommandFailure/Cancel`. Stop the relevant receipt before external abort or unsubscribe. Last-observer removal and feature disablement use `StopObserving`. These operations have no actual observer counter, transport, abort handle or listener callback inside the module.

Concrete integration remains unimplemented:

- The original non-TV account/session/device adapter must provide authenticated subscription ingress and command dispatch for these four account endpoints. No session token or device proof is stored here. Existing native Home, apartment, gameplay and world grants cannot authorize these descriptions.
- The account caller must invoke `BeginOwner` on every session replacement, even when the textual user ID stays the same. It must serialize ingress onto one thread and retain the correct captured receipt.
- A reviewed guest-room mapper and presentation caller must consume `FHomeVisit::Room`. This module reuses only `FRoomSelection`, never `FSnapshotSelection` or its private apartment provenance. It does not invoke a mapper or depend on the publication owner's evolving implementation.
- Home UI must consume separate list, visit and command errors and preserve an unavailable guest state while pending or failed. UI, navigation, confirmations and actual subscriptions are outside this patch.

Backend contact, sharing, ownership, enabled-invitation, rate, count and entitlement checks remain authoritative. A local invitation list is descriptive, not authorization. One guest response contains one entitlement-filtered saved PersonalRoom, its revision and owner handle, not a private library or complete apartment. It is distinct from shared-call room copies. No local receipt, matching handle or saved path connects accounts physically or enables editing, care, capture, movement, outside travel, payments or other gameplay.

## Decoder contract and bounds

Both value decoders copy bounded source input before clearing their old output, including an input that aliases a previous output string. They decode into a fresh candidate and publish only after every field succeeds. Oversized input avoids the copy and clears output. UE JSON handles parsing and serialization; immutable StateJson helpers handle preflight, bounded arrays, exact keys, enum/string/number/boolean checks and safe integers.

| Endpoint value | Input characters | Nesting depth | Commas and colons |
| --- | ---: | ---: | ---: |
| Invitations | 524,288 | 4 | 1,024 |
| Guest room | 262,144 | 6 | 256 |
| Command null | 4,096 | 2 | 8 |

These are native `FString` preflight units, not wire-byte limits. The list ceiling covers 100 entries at the field limits even if all 44,800 string code units use six-character JSON escapes. The complete list uses at most 801 separators and depth 3. A room with all optional fields, design and eight complete items uses 162 separators and depth 4. The room ceiling leaves room for escaped strings and number serialization. Null allows bounded surrounding JSON whitespace and accepts only UE-validated raw null, never `{}`, an envelope, an array, a quoted string or another primitive. Excess padding still fails the preflight ceiling.

String lengths also count UTF-16 code units to match JavaScript limits across native TCHAR widths. Values retain case and whitespace. Saved room IDs/names and furniture IDs require nonblank text using the ECMAScript whitespace set without trimming the retained value. Invitation and command identities remain exact and case-sensitive.

- Each required `received` and `sent` array accepts zero through fifty entries. Every entry requires exactly `id`, `handle`, `roomId`, `roomName`. IDs are nonempty and at most 256; room IDs are nonempty and at most 80; handles are at most 80 and room names at most 32, including empty values. Handle/name bounds retain the backend's outgoing `Unavailable account` and `Removed room` placeholders without contact-handle validation. The 256-unit account/invitation ID ceiling is a local opaque-identity allocation bound, not a claim that Convex's `v.id` publishes that maximum. No additional duplicate-invitation-ID restriction changes list semantics.
- Set arguments bound handle and room ID at 80. An empty handle can remain a backend validation decision; room ID must be nonempty. Revoke and visit IDs are nonempty and at most 256. Owner input is nonempty and at most 256, is not retained as an identity record, and creates a fresh generation. No argument is normalized or truncated. Disabled set intents preserve `enabled: false` on the original endpoint.
- A guest response requires exactly `room`, `revision`, `ownerHandle`. Revision is an integer from zero through `9,007,199,254,740,991`, using shared StateJson's safe-number bound. Owner handle is nonempty and at most 80. Null, missing fields, unknown fields, private-library additions and malformed roots fail.
- Room ID is nonblank and at most 80, name nonblank and at most 32. Both environments, `studio` and `terrace`, all three palettes, all three lights and all three sharing values are structurally accepted. A private sharing value does not imply server access approval.
- Optional layout is absent on legacy rooms or one of all five studio layouts, `pat-studio-v01`, `pink-bedroom-v01`, `pink-bedroom-v02`, `modern-kitchen-v01`, `bathroom-v01`. Present layouts require studio. Present design requires a layout and studio, exactly version 1, integer turns 0 through 3 and extension exactly 0, 0.6 or 1.2. Nonzero extension requires kitchen or bathroom. Missing design stays missing; it is not synthesized.
- Each of the four optional room colors must be `#` plus six ASCII hex digits when present. Case stays unchanged. Items are a required array of zero through eight, with nonblank IDs at most 80 and exact duplicate-ID rejection. All 23 canonical furniture kinds and seven surfaces are included. Finite x/z retain their source values with absolute limit 4; finite rotation retains its value with absolute limit 10,000. Optional y is finite from 0 through 4; optional surface is an exact enum; optional lit is a boolean.
- Optional explicit null fails for layout, design, colors, y, surface and lit. Absence succeeds where canonical omission is valid. Unknown keys fail at response, room, design, invitation and item objects. Required fields never default from absence.

`RoomJson` serializes the complete validated source room object, including item order and every present optional value. It does not reconstruct a typed subset, migrate coordinates, apply turns, normalize colors, substitute an own room or add omitted defaults. UE can change JSON formatting and numeric spelling; the contract preserves parsed values and optional-field presence, not source byte spelling. Structural acceptance intentionally omits placement and standing-point solvers, entitlement filtering, mapping and render admission.

## Lifecycle source review

These are source-inspected branches, not executed tests or runtime acceptance.

| Case | Inspected behavior |
| --- | --- |
| Empty, populated and fifty-entry lists | Required arrays decode in source order; accepted empty differs from null/unavailable |
| Over-cap list or malformed later entry | Fresh candidate never publishes a prefix; output clears |
| Placeholders, spaces and case | Retained without trim, truncation or outgoing contact validation |
| Root/field/null/unknown-key failures | Shared parsing and required typed readers reject; no defaults |
| Every room/design/item optional field and legacy omission | Full choice tables, bounds and omission/null branches precede complete room serialization |
| Aliased input/output | Bounded source copies precede decoder output reset; command arguments copy before intent output reset |
| Same-ID owner replacement | New GUID invalidates all prior subscriptions, reads and commands, clearing values, loading, busy and errors |
| List replacement | Clears only list lifetime; selected visit and pending account command remain independent |
| Another or same invitation selection | Clears old guest before new read; no local-list-membership requirement |
| Leave, invalid next visit or visit stop | Clears guest immediately; no own-room fallback |
| Last observer or feature disablement | `StopObserving` retires both read lifetimes; account command remains pending |
| Travel and read replacement during a command | No world binding; command remains tied to original account |
| Concurrent read and command | Query callbacks change only their own value/loading/error, never command busy/error |
| Second command while busy | Refused without replacing the active request |
| Stale, duplicate or wrong-operation callback | Current owner/subscription/request/kind checks reject before any state changes |
| Null success | Releases only matching command, with no optimistic invitation edits or inferred refresh |
| Failure or malformed command success | Releases matching command and exposes fixed bounded error while retaining read values |
| Exact cancellation | Retires only matching pending read or command, with no retry; stale cancel cannot settle newer work |
| Dispose and deliberate restart | Clears all receipts/state and remains inert until explicit new `BeginOwner` |

`ListError`, `VisitError` and `CommandError` remain separate. Fixed `HomesErrorText` messages never expose arbitrary backend exception text. Cancellation clears only the matching error. A stale stop, failure or completion cannot clear a newer error or release newer busy work.

## Reference reconciliation

All 22 pinned references matched both working bytes and blobs at source/dependency base `de632cb19d5916318b3a45494863b625e2342207`. Assignment bytes also matched their pin. No reference drift was found.

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

## Operations and acceptance

I read the complete owned source and diff, reconciled hashes, and inspected the shared index before the scoped source commit with hooks disabled. The handoff uses a separate scoped commit with the same index safeguard. Concurrent publication, generated API, project configuration, mobile, media and output changes remain outside both commits.

No tests, fixtures, business-code execution, lint, typechecks, builds, compilation, codegen, guards, server/engine/simulator launches, installations, asset generation/import, live calls, configuration, deployment or remote operations ran. No new chats, delegated agents or other-chat messages were created. This standalone source pair changes no mobile/shared renderer input; renderer packaging and What's New do not apply.

All gates remain false. Parent's independent standards/correctness review, Windows delivery, original-account transport integration, UI/mapping integration, content, compilation, in-game acceptance and release are deferred separately. This delivery does not declare Home visits or the full port complete. The parent should review only these three allocated paths across the source and handoff commits before accepting them.

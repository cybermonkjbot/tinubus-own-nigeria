# Native combat and recovery state handoff

October 10, 2026. Typed response decoding and local receipt acceptance are implemented. This is source work, not playable combat or recovery.

## Ownership and integration boundary

The assigned paths were absent and had no tracked or untracked changes at the initial inspection. The original implementation changed only:

- `apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionCombatState.h`
- `apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionCombatState.cpp`
- `apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionRecoveryState.h`
- `apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionRecoveryState.cpp`
- `docs/engineering/unreal-combat-recovery-handoff-2026-10-10.md`

A concurrent workspace checkpoint, `95654a7fb1d1283d92e03b78fb9cf39078e609e3`, captured the first four files before this chat finished review. The subsequent scoped commit contains the remaining corrections and this handoff. Use the final committed versions together, not that intermediate checkpoint alone. Other chats' session, backend, mission, equipment and documentation work was left untouched.

There is no caller, endpoint request, credential, grant scope, enable flag, pawn binding, UI binding or command admission. The existing backend remains authoritative. Receipt capture records an operation admitted elsewhere; its return value never permits that operation. Entry/presence, an active fighter, a decoded snapshot and a ready receipt do not grant gameplay control.

The implementation reuses UE JSON readers, shared pointers, arrays, optionals, GUIDs and multicast delegates already used by this client. It requires no new module, plugin or replacement engine system. `FWorldScenePoint`, `FWorldNavigation`, `FWorldNavigationObstacle`, `FWorldNavigationVertex`, `EWorldFieldPresence` and `TWorldPlayerField` come from the existing WorldPlayer header without editing it. All positions remain source metres with Y up and X/Z on the ground; angles remain source radians. No movement route is synthesized from combat positions or gunfire paths.

WorldPlayer's drawn-weapon enum is not reused because its wire union has only bat and pistol. Combat's exact union also contains revolver, smg, shotgun and rifle. The new enum retains all six without adding weapon assets, inventory behavior or firing logic.

## Exact decoded public fields

`DecodeCombatResponse` consumes the endpoint value, not a Convex response envelope. The subscription projection is `{ state, playerId, lifeId }`; mutation replies add `serverTime`. The decoder retains optional serverTime, while mutation receipt acceptance requires it. Required nullable fields must be present. Optional fields retain Missing separately from Null where the schema permits null.

| Combat object | Retained fields |
| --- | --- |
| Response | Required state as null or match, required playerId string, required lifeId as null or string, optional non-null serverTime. |
| Match | id, instanceId, placeId, layoutId, required nullable callSessionId, mode melee/arena, phase playing/finished/cancelled, revision, simulatedAt, endsAt, navigation, spawns, fighters, objective, events, tuning. missionRunId and gunfire are optional and non-null. |
| Navigation | revision, bounds, obstacles, radius, floorY and optional non-null gridSize. Each bounds/obstacle retains minX, maxX, minZ, maxZ, optional non-null yaw and polygon. Polygon points are exact X/Z pairs; spawns are exact XYZ triples. The combat projection strips streetGraph and venue metadata, so these are not accepted or reconstructed. |
| Fighter | id, kind player/agbero, characterId, XYZ position, yaw, pitch, health, maxHealth, stamina, ammo, reserveAmmo, required nullable reloadAt, status active/defeated/left, joinedAt, lastInputSequence, lastAttackAt, protectedUntil and kills. Optional streetWeaponId and robbery allow null. Optional targetId and skillLevels do not. skillLevels retains fighting, selfDefence and fitness. |
| Robbery | id, toutId, phase, unitKey lagos-credit, amountCap, createdAt, respondsAt, expiresAt, XYZ origin, required nullable escapeSince/resolvedAt, surrenderedAmount, revision and optional purpose recovery. Every phase is retained: demanding, fleeing, defending, surrendered, escaped, repelled, defeated, interrupted and expired. |
| Objective | target and required nullable winnerId. |
| Event | id, kind attack/hit/defeat/reload/finish/left, at, actorId, required nullable targetId, optional non-null XYZ shotPoint and optional non-null blocked boolean. Empty actorId remains valid because canonical terminal events use it. False blocked stays false; it is not treated as missing. |
| Tuning | health, stamina, speed, meleeDamage, meleeRange, meleeCooldownMs, bulletDamage, bulletRange, fireCooldownMs, magazine, reserveAmmo, reloadMs, npcSpeed, npcWindupMs, blockMultiplier, spawnProtectionMs, durationMs and target. No proposed defaults replace supplied values. |
| Gunfire | patrolOffsets and incidents arrays. Each patrol offset retains id and milliseconds. Each incident retains id, actorId, optional non-null lifeId, XYZ point, startedAt, lastShotAt and officers. |
| Gunfire officer | id, XYZ from, ordered XYZ route/returnRoute arrays, responseAt, arrivesAt, patrolOffsetMs and optional non-null redirectedAt. No officer motion, hearing, dispatch or search interval is advanced locally. |

`privateCombatView` removes other fighters' robbery information, then `combatStateValue` emits null for absent robbery. Both the schema's omitted form and explicit null remain representable. A non-null robbery on a fighter other than the response player is rejected rather than exposing information outside this projection. Mission combat uses a separate storage lookup key but retains the occupied instance ID in `state.instanceId`; acceptance compares that public ID, not the storage key.

`DecodeRecoveryResponse` takes an explicit response kind. A subscription value is `CompanionHealth | null`. A mutation value is `{ health: CompanionHealth, serverTime: number }`; mutation health cannot be null.

| Recovery object | Retained fields |
| --- | --- |
| Health | lifeId, revision, health, maxHealth, status active/hospital/detained, required nullable deathKey, protectedUntil and required nullable recovery. |
| Recovery destination | placeId, instanceId, exact XYZ position and readyAt. No hospital/station destination or countdown is invented. |
| Mutation response | Required non-null health and serverTime. The account identity is not in this wire value; the future transport must bind it to the selected owner. |

No public field from these projections is intentionally omitted. Stored seats, timed inputs, session IDs, private robbery receipts, resident bindings, death ledger rows, recovery economy and equipment inventory are not these public projections and are not decoded.

## Decoder behavior and limits

Both decoders reset their output before reading and publish a candidate only after every scoped field succeeds. A default, failed or moved-from populated response cannot be accepted as a null snapshot. Published match/health pointers are const; nested data contain no mutable shared pointer or JSON tree. The caller can retain a copy, so clearing the owner cannot revoke copies already held elsewhere.

Unknown object fields, unknown enum values, wrong types, missing required fields and disallowed nulls fail. Empty arrays remain empty, order is retained, and no record is sorted, merged, dropped or deduplicated. Ordinary strings are preserved exactly within the document budget. Match ID, instance ID, response player ID and non-null life ID must be nonempty. The empty event actor sentinel is preserved.

All numbers must be finite. Combat, robbery and health revisions and event IDs are safe nonnegative integers. lastInputSequence is a safe integer starting at -1, preserving a newly created fighter. Other numeric wire fields remain doubles without clamping, normalization or integer coercion, including skill levels, amounts, health, ammunition, tuning, geometry and source times. Server time and locally supplied mutation timestamps must be nonnegative safe-range finite numbers. The midpoint calculation preserves the canonical wall-clock offset; this is not the world clock, an authority clock or a timing sample usable for admission.

Combat JSON is limited to 2,097,152 TCHAR code units, nesting depth 32 and 131,072 commas/colons outside strings before UE builds its JSON tree. Typed arrays are capped at 43,691 entries, except navigation obstacles at 16,384. The latter ceiling follows the established WorldPlayer budget. Recovery JSON is limited to 32,768 TCHAR code units, depth 8 and 128 separators. These bounds reject oversized data rather than truncating it. They do not impose a streaming transport receive-memory cap. Valid future or unusually large backend data may exceed them and remains unsupported.

The geometry types are reused, but this reader deliberately does not invoke WorldPlayer's stricter geometry acceptance. The combat backend validator permits numeric boxes and polygon arrays without those extra geometry rules. Combat decoding checks finite scalars and tuple lengths, not positive radius, ordered box extents, polygon convexity/containment/yaw, grid suitability, obstacle clearance, route lengths or reachable spawns. Likewise it does not validate health/status consistency, tuning ranges, monetary rules, event ordering, unique IDs or gunfire time ordering. A decoded value is never collision, combat or economy authority.

The JSON tree and syntax handling use the existing UE serializer. Duplicate object-key handling follows that serializer; this patch adds no custom duplicate-key parser. The future admitted transport remains responsible for its own envelope, endpoint, origin, size and trust checks.

## Receipt owners and canonical acceptance

Both owners start empty and inactive. A future owner selects its externally established identity, starts a subscription generation, and captures that receipt for all callbacks from that subscription. Starting/restarting or ending the subscription retires old-generation callbacks and clears busy state, while retaining the last known data, errors, readiness and offset. Merely adding a native notification listener does not start a subscription. Clear drops selected identities and all private state; Dispose does the same permanently and removes notification listeners.

Combat selection includes the expected player and selected instance. Recovery selection includes an owner ID because the health projection has none. Changing either selection clears state. A new session in the same selection must call Clear before selecting again; an unchanged selection alone intentionally does nothing. Access loss requires Clear or Dispose, not EndSubscription, which represents canonical temporary loss of listeners and retains stale data.

Receipt generations are local opaque GUIDs, not server revisions or credentials. Match/life scope and input generations are distinct. Public views provide a const read and native change notification. Combat exposes selected instance/expected player, observed player/life, match, readiness, busy, error and server offset. Recovery exposes owner, health, readiness, busy, error and server offset. StatePresence and HealthPresence distinguish not yet observed from authoritative null. A subscription error can set ready while presence remains Missing; ready alone does not establish data availability.

| Receipt | Acceptance and retained behavior |
| --- | --- |
| Combat subscription | Current subscription generation is required. Player and non-null state's instance must match the selected identities. An older revision in the same match is rejected before changing life, match or error. Equal revisions are accepted. A different match or null state is accepted independently of the previous revision. Accepted match/life changes retire input receipts and advance the scope generation. Successful subscription reads clear the error and set ready. serverTime, if supplied, is ignored for subscription timing as in the canonical store. |
| Combat command | Capture records busy, clears the current error and invalidates earlier input receipts, matching command's stop boundary without sending a stop. It captures match/life, subscription generation and scope generation. If scope changed during flight, success is accepted only when the returned match and life now equal the current match and life. If scope did not change, a returned new match/life or null state may be accepted. Same-match older revisions still retain the prior data. Completion clears busy even if scope or revision made the reply stale. |
| Combat input | Capture records current match/life and input generation. Before accepting a response, all three plus subscription generation must still match current state. The response itself can carry a life transition, matching canonical submit behavior; it then retires the old input scope. This does not acknowledge an input sequence or prove an attack succeeded. Backend duplicate/rate-limited input can return unchanged state. |
| Combat neutral release | Uses the same captured-scope success rules as input. Its failures remain silent and retain earlier errors. An old-place release cannot be retargeted to a newer selection. No release is sent or queued by this slice. |
| Combat timing/error | Mutation serverOffset is recorded before the same-match older-revision check, preserving the canonical ordering even when state stays unchanged. A command error only changes the error for its original unchanged scope, then clears busy. An input error requires its original current input scope. Subscription failures set ready and retain the last match/life/player. |
| Recovery subscription | Current subscription generation is required. The supplied health/null replaces the previous value unconditionally, including a different life or lower revision. This preserves the canonical store rather than importing combat's revision filter. It sets ready and clears the error. |
| Recovery mutation | Capture requires an observed health value and no pending recovery receipt, records its life and clears error/set busy. Completion is current-generation only. It accepts health if there is no current health or the returned revision is at least the current revision, including across life IDs. A successful death recovery creates a new life, so matching returned life to captured life would be incorrect. Rejected older revisions retain both health and offset. Accepted mutation data update health/offset but do not independently set ready or clear an intervening subscription error. Completion clears busy. |
| Recovery mutation failure | The error changes only if current health still has the captured life ID. If health became null or changed life, the old error is suppressed. Busy still clears for the current pending receipt. |
| Cleared, replaced or disposed owner | Old success and failure callbacks do nothing. A callback from another owner's generation does nothing. Neither can revive a selection or clear a newer pending operation's busy state. |

Accepted means accepted read state, not successful gameplay. In particular, the canonical recovery store can accept a current-generation subscription with a lower revision after a mutation. It can also accept a late recovery mutation across life IDs when its revision meets the current global health revision. Those behaviors are preserved, not repaired by inferring permissions. Any future stronger ordering contract needs an explicit integration decision and authoritative evidence.

## Deliberate differences from the TypeScript store

These differences are local safety or integration constraints, not new game rules:

- Decoding is explicit, bounded and strict. Unsafe counters, malformed tuples, unsupported object fields and invalid identities reject instead of flowing through TypeScript's unchecked generic cast. Incomplete required nullable fields are not silently treated as null.
- Combat checks expected player/instance and the projection's robbery privacy rule. The canonical account-authenticated subscription establishes these at the server. An unmounted native reader does not inherit that trust from entry/presence, so it checks the selected read identity again. This is still not authorization.
- Failure notifications use fixed text and never accept raw backend exception messages. Errors remain alongside retained data. Detailed server-error classification is deferred to the future transport rather than exposing private payloads.
- Pending command/recovery receipt IDs make terminal completion one-shot. Duplicate terminal callbacks cannot finish a newer operation. The canonical Promise supplies one terminal outcome; this preserves that assumption at a native callback boundary.
- Notifications publish a coherent completed update rather than mirroring every intermediate emit inside a Promise's try/finally. A combat mutation that only changes serverOffset after older-revision rejection emits a change here; the canonical input path can change that offset without emitting. No observer should depend on TypeScript's notification count.
- Subscription lifecycle and mutation capture are explicit local bookkeeping methods. They neither count listeners nor create backend subscriptions. Clear and Dispose notify after dropping state so future native consumers can remove presentation; the canonical dispose clears listeners directly. A native consumer must keep the owner alive for the duration of each synchronous call/broadcast.

## Remaining work and integration obligations

No network or command hook is installed. The backend's existing account-authenticated endpoints cannot be called with entry/presence credentials. The concurrent backend chat's account-control guards were inspected as unrelated work and preserved. This slice adds no native counterpart, consent or grant activation. All gates remain unchanged and disabled.

The future admitted transport must bind account, session, grant/lease lifetime, game, visit and current domain before using these owners. It must dispatch callbacks on one owning thread, retain the captured receipt instead of reconstructing one at callback time, pass the actual request-start/receipt wall times for mutations, and clear on authority loss. Subscription receipt timestamps are ignored; callers may pass zero for that argument. Neither owner performs retries, polling, disconnect cleanup, persistence or error-envelope interpretation.

Combat and recovery remain separate canonical stores. The combat wire reports its current life ID; the WorldPlayer public projection does not. This slice does not infer life from a character, reconcile health across the two stores or grant controls from their agreement. Such reconciliation needs an admitted owner and backend evidence.

The following behavior remains outside this slice:

- Combat join/leave/robbery-action eligibility, argument encoding, command dispatch, input validation, monotonic sequence allocation, rate limiting, one-flight scheduling and input coalescing.
- Canonical stop's captured neutral input and pending-stop queue. The TypeScript store can send an old-place neutral release after selection/disposal. This slice only rejects its late response; a future authorized sender must retain the original outbound scope and never retarget it. InvalidateInputReceipts does not issue a release.
- Prediction, collision, damage, stamina, ammunition, reload timers, NPC AI, objectives, scoring, animation, weapons/art, auto-fire, gunfire hearing and police dispatch. Raw observation fields are not implemented game systems.
- Robbery choices, surrender, credits, receipts, recovery of stolen property, equipment ownership and all economic mutations.
- Medic approach, hospital readiness, treatment, death settlement, new-life allocation, banked-credit voucher, detention release, notifications and world relocation. ReadyAt is data, not permission to recover.
- World-clock integration, mission/combat control precedence, native pawn/camera/UI presentation, supported content selection, device interaction and visual acceptance.

There is no new unresolved public field-shape gap in these two scoped response contracts. Parser limits and the semantic/admission checks listed above remain explicit limits; they must not be described as complete combat or recovery support.

## Source review and verification status

Source review traced the two API stores, shared combat/recovery types, all nested public validators, privateCombatView/combatStateValue, mission match lookup, input soft declines, leave returning null, hospital new-life recovery and same-life treatment. It covered required-null versus missing values, optional false/zero values, all six street weapon IDs, every robbery/event/status enum, retained gunfire paths, the empty finish actor sentinel, failed-output clearing, selected identity changes, callback generations, equal/older/cross-match revisions, cross-life commands, input invalidation, neutral-release failures and recovery's distinct mutation/subscription ordering.

Review corrected an initial attempt to reuse WorldPlayer's narrower weapon enum, protected selection parameters that can alias a prior view, placed exported decoder declarations before their friend declarations, and made moved-from populated decoded responses invalid rather than apparent null observations. These are source-review corrections, not executed test results. The unslop skill guided the handoff's concise status language and separation of implemented source from remaining work.

No tests, typechecks, lint, builds, compilation, code generation, asset conversion/import, engine/server launches, installation, deployment, live configuration, remote transfer or messaging ran. No production renderer input, shared scene/map data, registry, mobile code or bridge contract changed, so renderer packaging was not triggered. No Windows delivery, runtime lifecycle verification, in-game verification or release is claimed. The parent owns independent review/integration, and the coordinator owns Windows delivery.

## Shared JSON and mission error correction

The parent's full-range review of `2ba875d...1ed6ec04c`, including the intermediate workspace checkpoint, found no combat/recovery Spec defect. It identified duplicated JSON preflight/scalar readers across combat, recovery and mission. The combined correction changes the three private State `.cpp` files, adds private `BaddieCompanionStateJson.h/.cpp`, and updates this handoff and the mission handoff. Public headers remain unchanged. Policing, social, teams and haul owners are outside this correction.

`BaddieCompanion::StateJsonPrivate` owns the identical helpers. Non-template implementations live in one `.cpp`; the required-nullable template lives in the private header. Callers import individual names into their distinct private namespaces, retaining unity-build isolation. Both helper files must accompany any later transfer of these three state implementations.

| Shared helper | Exact contract |
| --- | --- |
| `Parse` | Object root with explicit character, nesting and separator limits. Retains the quote/escape-aware preflight and UE JSON serializer. Rejects invalid or over-budget input without truncation. It does not clear output on every failure; public decoders retain atomic publication ownership. |
| `Object`, `Field` | Object-only access and field lookup. Missing remains an invalid JSON pointer; explicit null remains a valid null-typed pointer. |
| `String`, `Number`, `Boolean` | Exact JSON type checks without coercion. Strings have no shared trimming/length policy. Numbers must be finite, with no sign/range restriction. Object-field Number delegates to the value reader. |
| `Integer`, `ValidTime` | Integer requires a finite whole number no greater than 9,007,199,254,740,991, with minimum zero unless supplied. Only combat lastInputSequence supplies -1. ValidTime requires a finite, nonnegative safe-range double without rounding. Callers choose which fields use these restrictions. |
| `Same`, `Only` | Case-sensitive comparison and opt-in unknown-key rejection. Only does not enforce required fields. UE shared-string keys are copied by explicit length, following the mission reader. Combat/recovery retain their allowlists; mission still ignores unknown fields. |
| `Nullable` | Required field into a fresh TOptional candidate. Missing fails; null succeeds without assignment; a value is assigned only after its supplied reader succeeds. Optional presence/null policy remains domain-owned. |

Parser limits remain explicit and unchanged:

| Contract | TCHARs | Nesting | Separators |
| --- | ---: | ---: | ---: |
| Combat | 2,097,152 | 32 | 131,072 |
| Recovery | 32,768 | 8 | 128 |
| Mission | 262,144 | 16 | 16,384 |

Recovery retains its size-checked raw-null subscription branch before object parsing. Mission retains its 8,192-character text limit, 240-character IDs, whitespace/control-character rejection, case-sensitive reward map and field-presence policies. Combat/recovery retain exact strings, including the empty event actor sentinel. Arrays, schemas, enums, numeric field restrictions and all mutation/subscription ordering remain domain-owned. No generic schema framework was introduced.

The same correction makes an active, selected, idle mission owner's invalid mutation-start time set `InvalidReceiptTime` and return false with a cleared output receipt. Wire, readiness, busy, offset and request counters stay unchanged. Disposed, busy and unselected refusals remain no-ops apart from clearing the supplied output. Successful capture and completion ordering are unchanged.

Source/diff review compared preflight branches, scalar predicates, key comparison, null handling, parser budgets and changed callers. It traced invalid-time refusal with populated/empty mission data, prior errors, busy/unselected/disposed owners and valid capture. The two optional mission style heuristics remain deferred. No public signature, admission rule, native gate or runtime owner changed. No executable validation or remote delivery ran; parent correction review and native transfer remain pending.

## Original canonical source pins

Canonical SHA-256 values read during the original source review follow. Backend endpoint files include the other chat's in-progress account-control guards at that read; no such guard was edited here. Recheck those files at the parent's integration pin.

```text
3c8fe8b68481774c6a886c9fa1c964c8c307ac40acb5d48322f8cf9adbb5e320  packages/api/src/companion-combat.ts
bbbf79d23adc51bd8d7ef693a4ff7c8b63caf190db4affeaa631f6b7d1d2fd52  packages/api/src/companion-recovery.ts
56da7b73be7622d6d0d7954c6ada30ffc7cf10d04bb3ddbfbb29559b4839078d  packages/utils/src/companionCombat.ts
e4ec7d77076df9713fa4177e4abc0b71968231d8f4538660665652c9a2882d79  packages/utils/src/companionRecovery.ts
a3f2f01f9d50da5e7fbf271c2b94bee45104c89620684b497ac2f7ccdaf92325  packages/utils/src/companionCombatEquipment.ts
5eaef9a7e59850a61ce887e3fe96ee0b66d6824e409376813898597d78fe3c32  packages/utils/src/companionCombatRobbery.ts
3476fcb21ba9a4dff96bbcf3b81d3d8b1b309cfaa5ce1d1c025892666d2b3d2e  packages/utils/src/companionGunfire.ts
19e1ef63adeeb51fde2d5c798e33a260906b261ae791c3f3fe9de5ed84d1f1c2  apps/backend/convex/companionCombat.ts
a91c61bbc8fe532cc9da512103b9a612cfdaa5353a72e900d452435eaa436040  apps/backend/convex/companionRecovery.ts
68c4352f1d5730a35f199efe5c0e8cc839e5fb8781dad5c0a308b75c213ef238  apps/backend/convex/lib/companionCombatModel.ts
df76e2a3b8db23c5ebb3cec9f8d8d760016ced75963e9ea81f2a65452f9dc2c0  apps/backend/convex/lib/companionRecoveryModel.ts
6780ac889dc5d4b8a6c3f519a4b887b5d92e648a68fb774fe34828b53b16ef30  apps/backend/convex/lib/companionCombatRobberyModel.ts
55cdddbcb92a1983b98585c8feebfb831468e53e79e45d01370bc23898dc35b0  apps/backend/convex/lib/companionCombatEquipmentModel.ts
90d230cf0233458c46324fd87d6f077c6f54bd7dd691030ba5a2a89e8607a17d  apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionWorldPlayer.h
```

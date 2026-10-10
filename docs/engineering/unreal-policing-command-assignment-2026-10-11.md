# Policing command serialization and dispatch ownership

October 11, 2026. Source-only continuation of the existing policing owner.

## Ownership and pins

Existing chat `01a12728-212d-73a3-b060-9f80daa836d3`, host local, owns only PolicingState.h/.cpp and new `docs/engineering/unreal-policing-command-handoff-2026-10-11.md`. The accepted read/consent implementation and original handoff remain the baseline. Extend it; do not introduce another policing store. The original handoff is read-only. Parent owns StateJson, assignments, review acceptance and the tracker. Coordinator alone owns Windows writes.

Canonical references, dependencies and source preimages are pinned at `7900099f44c8e08fbe447fe750cc74a171867f90`. Parent matched all fourteen working hashes below and confirmed the new handoff absent. Recheck before editing. Use complete unchanged StateJson from `b4bc1b7d7dbc9552ad1451062a404fc872a13224`. CombatState's newer header is a read-only dependency, not permission to change combat or replay its accepted review.

## Existing implementation to retain

Read the canonical policing account store, shared demand confirmation and relevant endpoint validators/returns, plus the complete current native pair. It already decodes encounters, captures exact demand consent, prepares all four actions and handles subscription/command generations. Preserve these behaviors and compatible legacy PrepareCommand/Accept/AcceptFailure APIs. This assignment fills serialization and explicit one-time dispatch ownership only.

Keep the native receipt-driven caller model. Use Unreal JSON and the existing private shared helper. No custom network client, policing simulation, local settlement, timer, UI or engine replacement is needed.

## Complete command packets

Serialize immutable non-credential packets for all four existing actions:

| Action | Endpoint | Arguments |
| --- | --- | --- |
| Cooperate | companionPolicing:cooperate | encounterId |
| RequestRelease | companionPolicing:requestRelease | encounterId |
| PayExtortion | companionPolicing:respondExtortion | encounterId, response pay |
| RefuseExtortion | companionPolicing:respondExtortion | encounterId, response refuse |

Capture exact strings before external exposure and use UE escaping. No confirmation amount, outing, entry slices, revision, price, credentials or new fields travel in these packets. Preserve retry:false and invalidateQueryCache:false as packet/adapter policy. All endpoint successes require actual root JSON null through the existing decoded-response contract. A response is not a local encounter update or permission.

Reuse existing limits where applicable. Make any new native structural bound explicit in the handoff. Do not trim, case-fold, invent identifiers or confuse the bound with server eligibility.

## Dispatch and completion

- Retain canonical early order: disposed, no subscribers and busy refuse silently; pay checks readiness and exact confirmation before the ordinary ready/encounter return. Keep existing fixed demand errors. Non-pay actions gain no new phase, deadline or balance predicates.
- Capture generation and encounter ID, set busy and clear error, then let the external caller publish the canonical busy notification. Recheck the exact request after that notification. Selection/access replacement, last-observer loss or disposal must prevent an unsent packet from becoming dispatchable.
- Same-generation encounter changes during the busy notification do not retarget or cancel the captured request. Canonical dispatch rechecks generation/disposal, not encounter ID or refreshed consent. Error publication after completion still compares the captured encounter ID.
- Add an explicit once-only mark for the new serialized-command path. Default, foreign, retired and duplicate marks must fail without issuing work. The adapter must send immediately after marking, without yielding or notifying. This mark establishes local ownership, never native authorization.
- Permit explicit abandonment only while the new request is genuinely unsent. It releases only that request's busy ownership and does not simulate a response or apply an action. A marked request cannot be silently abandoned or repeated.
- A new-path completion must belong to a marked request and settle it once. Reject unmarked, duplicate, wrong-owner, wrong-kind and old-generation completion without clearing newer work. Preserve compatible legacy receipt-only completion, with a distinction that prevents legacy APIs from bypassing a new owned request's marking rule.
- Reuse the existing atomic null-response decoder and scope/error/finally behavior. A changed/null current encounter suppresses old action errors but still releases the original busy owner within its generation. Success does not replace encounter or clear an intervening read error. Old generation/disposal completion never revives private state.
- Keep notifications external and synchronous. Document which results require the caller's existing notification, when busy is released and how notification reentry avoids touching a newer request. Do not create an automatic retry or retained background queue.

## Completion and boundaries

Inspect source and the complete owned-path diff, including preparation notification reentry, same/different/null encounter changes, pay confirmation ordering, quotes/control characters in strings, root-null acknowledgements, unsent abandonment, marked duplicate completion and legacy/new overlap. Checkpoint exactly owned paths with hooks disabled. Write the new handoff with immutable source, preimage, output/dependency hashes, all four packets, lifecycle coverage, legacy compatibility and remaining integration. Parent will review the full three-path range independently on Standards and Spec axes.

Keep account/readiness gates and world-control interlock disabled. Entry/presence, Home reads, a decoded encounter and demand confirmation grant no policing, movement, payment, combat or editing permission. Native transport/consent and notification/subscription producers remain absent. Backend enforcement, debt, release and settlement are unchanged.

No tests, typechecks, lint, compilation/builds, codegen, asset conversion/import, engine/server/Blender launch, installation, remote writes, live calls, deployment or configuration. Preserve renderer/mobile inputs and unrelated work. Source acceptance, Windows delivery, compilation, in-game inspection and release remain separate.

## SHA-256 pins

```text
1383bd9678e5014495413b6a281381bbde91daa8afea60f1613d2a71a6141942  packages/api/src/companion-policing.ts
b1f6e06d26feb906cf5c5e1d5591311d868929f512a70e4bf4ed3c56781d3719  packages/utils/src/companionPolicing.ts
2e729e6abeba8e28c7b7d328f985451162d8ba036174da86636adac0c9f02ab4  packages/utils/src/companionHaul.ts
a3f2f01f9d50da5e7fbf271c2b94bee45104c89620684b497ac2f7ccdaf92325  packages/utils/src/companionCombatEquipment.ts
4a7d9c284ba8d1acc4e75b88634fbfc62f973504bdc0f7782edad7614001f59b  apps/backend/convex/companionPolicing.ts
1bf485bea06bd3ebcb49f67a93e28dc5d3f6ad433711196b01996d37e9a06863  apps/backend/convex/lib/companionPolicingModel.ts
b992b581d0fa60852f0819bfab2071376be4d99af88b65fea203c51311f0f7bb  docs/engineering/unreal-policing-state-handoff-2026-10-10.md
51134f54334a05335374576dbf9b04f6a467a128a6aa1166e177eb309ad09748  apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionPolicingState.h
fc740172318d0b42766804e55956a2d2371088ef341d03e52385ec4123922a80  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionPolicingState.cpp
bbdebfa73a9d2e00579031711a640435f4d9b88dde4d81d2cee0fb3b60b54e4c  apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionCombatState.h
90d230cf0233458c46324fd87d6f077c6f54bd7dd691030ba5a2a89e8607a17d  apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionWorldPlayer.h
13ceb1b13bee4b0793ddb117e03559bfb8ea77f8bdb8dc81baa1ffa8e70bb618  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionStateJson.h
df02a8dbf13c9f23ce11f57d0584c96c6a31c2cbe682926ae10be5bb6f3021ce  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionStateJson.cpp
536bf39a4089acea43be36448ad3d02d3cd7f94a65015d3975f761941b802328  apps/companion-unreal/Source/BaddieCompanion/BaddieCompanion.Build.cs
```


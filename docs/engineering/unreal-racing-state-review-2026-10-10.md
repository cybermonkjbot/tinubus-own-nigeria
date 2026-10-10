# Racing read source acceptance

October 10, 2026. Parent-owned acceptance. This is not vehicle gameplay or compiled compatibility.

## Reviewed range

The complete three-path review is fixed to `dbac2133fa25f111c6ed0745ba632e551c2a63c5...29797dc5405b398a586c8473ee067b0f692d17f9`, with 367 added lines. Source commit is `7bce7763ee33e2b58f3b55c0f2a06cc96516a92a`; handoff endpoint is `29797dc5405b398a586c8473ee067b0f692d17f9`. Spec is `unreal-racing-read-assignment-2026-10-10.md`, committed at `21b7a8e1ce3a8fcdad7a29062e73393b0308ff86`.

Reviewed paths are RacingState.h/.cpp under `apps/companion-unreal/Source/BaddieCompanion/Public/` and `Private/`, plus `docs/engineering/unreal-racing-state-handoff-2026-10-10.md`. The six canonical references and shared StateJson dependencies are pinned in that handoff. Parent verified the nonempty range before dispatching the two independent reviewers. Both reviewers are now closed.

| Output at review endpoint | SHA-256 |
| --- | --- |
| Public/BaddieCompanionRacingState.h | `493db64cf9ee02f713ae82d95464a16dd168e0aaa636c42bb1b7e9104922b4e5` |
| Private/BaddieCompanionRacingState.cpp | `dd54bbc605c97f037e9cb36c6ea30713c5103bc806088c4a3700cece48bb5001` |
| unreal-racing-state-handoff-2026-10-10.md | `45be4baf91578930669721d829a475e6202c19b03ba4996537640ebe754898ed` |

## Standards

Reviewer `01a12796-4ac0-78f3-afcd-459cc536ef96` found zero documented-standard breaches and one P3 judgment.

Possible Middle Man at RacingState.cpp:15: `bool Text(const FValue& V, FString& Out) { return String(V, Out); }`. The alias adds no validation, conversion or domain meaning. Its callers could use the shared String operation directly. This does not apply to Numeric, which disambiguates Number overloads for Nullable. Parent accepts the source and defers this optional cleanup; it does not change behavior or the delivery dependencies.

The review confirmed shared StateJson reuse and the separation of caller lifecycle from decoding. Source acceptance does not include executable validation.

## Spec

Reviewer `01a12796-4b79-7d80-855d-ab6fc4797193` found zero missing requirements, scope creep or incorrect scoped behavior. All six recorded canonical hashes matched.

The complete incoming snapshot is validated before retention. Retention uses the prior race's instance and strictly greater updatedAt, not race ID, and retains the whole prior snapshot. Equal ticks and accepted null races replace. Absent field, outer null, serialized null and invalid inputs remain distinct. Car ordering, case-sensitive uniqueness, the 32,768-character/eight-car bounds, optional recoveredAt, nullable times and receive-specific numeric rules match the assignment.

The handoff accurately traces world acceptance, command generations and account/session/visit/life clearing. It names missing native consumers and records the intentional structural strictness differences. No runtime authority is inferred from decoded identity, race state or selected-car eligibility.

Summary: Standards has zero documented breaches and one deferred P3 judgment. Spec has zero findings. Both axes are accepted for source delivery.

## Delivery and remaining work

The coordinator may deliver RacingState.h/.cpp and matching StateJson.h/.cpp from the review endpoint, together with the racing assignment, handoff and this acceptance record from the subsequent parent document checkpoint. Verify already-matching helpers without rewriting them. Guard fresh destination preimages and preserve earlier source, setup, configuration, binaries and world-worker records.

No native world/command owner, vehicle content, simulation, input queue, clock integration, rendering or gameplay permission is supplied. Imported content, compilation, in-game acceptance and release remain unverified. No tests, typechecks, lint, compilation/builds, codegen, imports, launches, installs or live changes ran. Windows delivery is tracked separately in the port record.

# Unreal social and teams read state

October 10, 2026. Source implementation and source review only. No native consumer is mounted, no permission is added, and account/readiness gates remain unchanged. This is not playable completion.

## Owned patch

We added only these files under `apps/companion-unreal/Source/BaddieCompanion` and this handoff:

- `Public/BaddieCompanionSocialState.h`
- `Private/BaddieCompanionSocialState.cpp`
- `Public/BaddieCompanionTeamsState.h`
- `Private/BaddieCompanionTeamsState.cpp`

The implementation uses the existing native value-owner pattern, Unreal strings/arrays/shared pointers and UE JSON readers/serializers. It adds no UObject, replication, networking, credential storage, timer, actor, controller, HUD or engine configuration. The two private namespaces keep helper names separate in unity compilation units. Initial domain source is checkpointed at `b921f5308caf865f3995556ef4cb83b2be4b744c`. Integration `e37deb93f` shares the existing readers; parent `2dd54bfe5` shares array preflight, and correction `691a60a98ac276d3c9a59793965b5b1bb47a69d1` handles scalar-null responses with the installed UE reader. Full review plus correction review now accepts the source with that matching helper. Windows delivery and runtime integration remain pending.

## Implemented reads

| Canonical value | Native representation |
| --- | --- |
| Social offers | Every encounter ID, call session, peer ID/name, place, room revision and expiry. The server's array order is retained. |
| Player access | User ID, handle, name, nullable thread and accepted/pending/declined status, canMessage, isMessageRequest and isPendingOutboundRequest. No request is accepted locally. |
| Reputation | All seven value/exact metrics, observed/window timestamps and the exact signals-only, disabled-rewards and unassigned-privileges markers. No derived score, reward, privilege or inference. |
| Conversation context | Nullable place/qualifiedAt result. No conversation content or discovery. |
| Team view | Group ID/name, canHost/canInvite/canJoin, every member's ID/name/role/isYou/available/joined, complete nullable room and invitation. |
| Team room | Room/thread/call/host identity, name, setting, status, capacity, startedAt, expiresAt and isHost. |
| Team invitation | Invitation/call identity, expiry and nullable joined/declined response. |
| Team call entry | Nullable room ID, thread ID and startedAt. Decoding does not join a call. |

Required nullable fields distinguish a supplied null from absence. Absence, unexpected fields, unsupported enums or invalid types fail the entire decode. Failed decodes clear outputs rather than exposing partially populated data. Accepted numeric timestamps remain unchanged, including fractional server values. Revisions and capacity require safe integers. The public city's empty host ID, capacity 50, distant deadline and separate room thread are retained. Only the outer team thread must match the selected group.

Each JSON input is capped at 262,144 TCHARs before deserialization, eight nested containers and 4,096 separators outside strings. IDs are bounded to 240 characters without controls or surrounding whitespace; public text is bounded to 8,192 characters. Offer arrays are bounded to the endpoint's 12 and rosters to the canonical 50. Unknown or oversized data fails, not truncates. Transport byte limits, envelope acceptance and authenticated request scope still belong to the external owner.

## State and lifecycle

`SetIdentity` takes a non-secret account/session-lifetime key, never a credential. Changing that key clears all state and callback ownership. `Clear` also retires the same identity's existing callbacks and removes selection. Call it on access loss or a replacement session for the same account; then set the new lifetime key and select again. `Dispose` is terminal. Separate owner instances and later instances cannot accept each other's tokens.

Private shared identity/generation objects make callback lifetimes distinct without a wrapping integer counter. Tokens retain those objects, so allocator address reuse cannot revive a retired callback. State owners cannot be copied or moved. Snapshot getters return value copies with no shared mutable JSON objects. Reads do not expire, reorder or mutate anything.

The external account host owns listener counts, subscription cancellation, notifications and ordered callback delivery. Select a real authorized place/group, then call the relevant `Begin...Observation` immediately before installing its subscription. Stop observation when its last subscriber leaves. Selection invalidates the previous observation; an unchanged selection is a no-op. Calls to `Begin...Observation` replace the subscription epoch without starting a request. Old successes, failures and clock observations are ignored before decoding.

The canonical responses have no common response-version watermark. We preserve their delivery order rather than sorting by expiry, member identity, room revision or receipt time. A future asynchronous transport must serialize each subscription's callbacks in the canonical order. Offer payloads must match the selected call session; team payloads must match the selected outer group.

Social selection clears offers and restores not-ready, including deselection. Offers reconnect/reload retains the current data/error until the next response. `BeginOffersObservation` performs that reload invalidation. Team selection clears the view and error, retains command busy state, and sets ready on explicit deselection. Initial team state remains not-ready. Team `Reload` retires observation and clears only the error; `BeginObservation` is then needed to bind the replacement callbacks. Stopping observation retains displayed data and errors in both owners.

Accepted reads set ready and clear the relevant error. Subscription failure clears offers or the team view and sets ready with an error. Malformed current responses, mismatched response scope or invalid clock observations also clear the relevant data. Public errors are fixed enum categories, with fixed decoder diagnostics; raw server exception text is deliberately not retained. The integration still needs an approved mapping for actionable endpoint errors.

### Independent player observations

`BeginPlayerObservation` records its own user/call pair, independently of the selected place and offers epoch. Two observers of the same pair remain independent. Offers selection/reload/stop does not invalidate player observers. Only stopping that observer, identity clearing or disposal retires it. Returned user IDs must match their recorded peer. The endpoint does not return a call-session ID, so the transport must attach the token captured for that exact query.

The native owner retains at most 64 concurrent player observations as a local resource bound. Failure to begin is explicit and does not evict another observer. Stop each observation when its consumer releases it. A one-shot player read can use the same lifecycle and release after reading its value copy. Invalid responses and errors clear that observer's retained player access, preventing old messaging flags from surviving a failure. This is local retention for the canonical callback interface, not an additional player cache or discovery API.

### Explicit display time

`Observe...` and `ObserveClock` accept an explicit nonnegative integer Unix-millisecond clock observation within JavaScript's date range. No timer or polling starts. The caller supplies the same kind of wall-clock display observation as the canonical stores, not elapsed game seconds or inferred server authority. A clock update needs a live observation token. Pure `ExpireTeamView` also supports in-place use by copying before replacement.

Offers with `expiresAt <= now` disappear, preserving the relative order of survivors. A later clock alone cannot recreate removed offers. A subsequent ordered server response can replace the list, matching the canonical store's wall-clock behavior.

Room expiry takes precedence over invitation expiry. At the room deadline, the room and invitation become null, canInvite/canJoin become false and every member's joined flag becomes false. Member identity, role, availability, isYou and canHost stay unchanged. Invitation expiry leaves a joined invitation intact. Any other expired invitation becomes null; canJoin becomes `room.isHost || previousCanJoin && members.some(isYou && joined)`. No other membership or permission is inferred.

Deadlines remain doubles and are never narrowed to a signed 32-bit timer interval. The canonical scheduler's long-deadline overflow guard is unnecessary here because this owner schedules nothing. Integrating an external wakeup must retain that guard rather than turning a distant public-city deadline into a short polling loop.

### Command observations, not commands

Teams exposes busy/error bookkeeping only. Begin captures an identity-owned pending request and selected group, rejects overlap, and clears the error. Selection, reload and loss of observers do not clear busy. Finish clears busy only for the matching request. A failure changes the error only when the selected group still matches the group captured at begin, following the canonical selectedAtStart comparison. Identity replacement invalidates the pending request. Finishing successfully neither modifies the team nor produces admission.

Only call this bookkeeping around a separately admitted, explicitly requested command after its canonical preflight succeeds. Transport failure must finish as failure. This patch does not implement those preflights, command dispatch, call attachment or reservation cleanup.

## Deferred integration and action contracts

The shared-helper follow-up changes only the two owned `.cpp` files and this handoff. Both now use shared object-root parsing with explicit limits of 262,144 characters, depth eight and 4,096 separators, plus shared object access, field access, exact string comparison, scalar string/number/boolean readers and safe-integer validation. Revision retains minimum zero; capacity retains minimum one without adding the legacy limit of 12. Number wrappers retain the nonnegative safe range and fractional timestamp acceptance. Domain wrappers retain exact field counts, all required field reads, identifier/whitespace/text limits and enum choices. Shared `Only` is not needed because the existing field-count checks and required reads already reject unknown fields. No object keys are converted, truncated or normalized.

The shared nullable reader operates on a fresh `TOptional` inside each domain wrapper. Only a successful decode maps it back to the existing Missing/Null/Value representation. Required absence still fails; null keeps a fresh default payload; the public decoder still clears output on failure. Non-null object roots use the shared object parser and a local JSON object-value wrapper. Top-level null handling is corrected below after installed-engine inspection confirmed that the original value reader rejects scalar roots.

The worker's integration at `e37deb93fd35d897d0bfd12364f5660d2246f061` retained one duplicate preflight because the helper's `Parse` accepts object roots and offers are an array. Parent `2dd54bfe57574b9e281ee4846fa10d51bb0ce9e2` extracted the existing shared scanner into `WithinParseLimits`, called by both object `Parse` and the social array branch. Size, depth, separators, string/escape handling and balanced-container checks remain unchanged. This preflight is not JSON syntax validation or root admission; both branches still invoke their original UE serializer overload and validate the decoded shape. No object wrapper, trimmed-input parsing or additional offers-array limit was introduced. That extraction preserved the faulty raw-null branches; it did not cause the scalar-root defect.

Independent full review of the thirteen-path integration found no documented Standards breaches and one optional call-session naming judgment. Spec accepted the non-null field and state behavior, but held scalar-null compatibility for installed-source evidence. The parent read UE 5.8.3's JsonReader.h, JsonSerializer.h and JsonSerializer.cpp on Cribtest11. `ReadStart` requires an object or array, and the value policy accepts only those root types. Consequently, `DecodeSocialConversationContext` and `DecodeTeamCallEntry` could not accept their valid null endpoint responses.

The correction adds private `StateJsonPrivate::ParseNull` and uses it only in the existing social/team raw-null branches. It keeps the 262,144-character limit and exact lowercase null dispatch. The helper checks the original input budget, then encloses that unchanged input in one array for UE validation. It accepts exactly one valid `EJson::Null` value and publishes only after success. Empty input, other scalar types, quoted null, additional values and malformed input still fail. Surrounding whitespace is checked by UE, not removed before parsing. Object and offers-array branches, nested nullable fields, public headers, callback scope and state ordering remain unchanged. Matching helper and both consumers must travel together. Independent Standards and Spec re-review accepted `691a60a98` with zero new findings; the original optional call-session naming judgment remains deferred.

Installed source paths, hashes and line evidence are recorded in `unreal-native-state-review-2026-10-10.md`. No engine code changed and no executable validation ran.

The parent owns integration; the existing coordinator owns Windows delivery. The account host must still bind identity/access loss, real selection, listener lifetime, callbacks, display-clock observations and native UI. No native social/team endpoint permissions or trusted transport wiring are supplied. The existing disabled configuration stays disabled.

The on-demand reputation, conversation-context and call-entry helpers have typed decoders only. Their request identity, late-result acceptance and cache ownership remain external. The canonical reputation query buckets its supplied time to a minute and uses a 60-second cache; player, conversation-context and call-entry reads bypass the query cache. Do not turn them into background polling or automatically show private context. Reputation data remains private to its account consumer.

The following action and host contracts remain intentionally untranslated, not removed:

- Social `offer` and `respond`, including encounterId, offered/qualified/declined status and nullable threadId results; local player-selected/encounter/content-tool event routing; direct/group conversation selection.
- Credit transfer, including recipient/credits/optional contextPlace/note input and success, credits, kobo, nairaLabel, recipientHandle, recipientName, threadId and messageId output. There is no debit, credit, mutation encoder or success substitute here.
- Inbox message awareness, including scope, readiness, unread/request counts, optional preview with conversation/name/text, preview permission and intense-gameplay state. The existing Inbox owner must continue to supply these; the game must not discover conversations or infer unread counts.
- Team create with thread/setting/capacity, invite with room/start/user IDs, and join/decline/leave with room/start identity. Full admission remains roomId, threadId, nullable callSessionId, startedAt, expiresAt and literal startMuted true. No admission decoder or action-result installation is provided in this read patch.
- Canonical team command preflights, same-game/current-call checks, explicit muted call attachment, matching-call leave, exact reservation release after failed/disposed joins, and abandoned-reservation server cleanup. Hiding a view or disposing read state must never leave a chat or terminate a call.
- Spatial voice, automatic presence, world/control admission and permission from crew membership. None is implemented or enabled.

## Source review and evidence

We reviewed every public read field against its utility contract and backend response projection. Source review traced exact-deadline removal, joined-invitation retention, room-expiry precedence, the host/self-joined canJoin branches, stable array order, outer-versus-room thread identity, empty public-city host, capacity 50, initial versus explicit-empty readiness, retained data on reload/stop, independent player observers, same-group command errors, identity replacement, malformed-data clearing and immutable reads. These are inspection findings, not executed tests.

The initial follow-up compared each reused operation against shared-helper commit `ab535c29cedd4e893a62539f247a03488495b3e9`. The later parent scanner extraction and accepted scalar-null correction are recorded above. Public headers, domain field mappings, state lifetimes, expiry and response ordering remain unchanged. `unreal-native-state-review-2026-10-10.md` contains both complete-domain and correction reviews; all reviewers are closed.

No tests, typechecks, lint, builds, compilation, code generation, servers, editors, imports, installs, asset conversion, live configuration, deployment or Windows handoff ran. No production renderer, mobile, backend, session, transport, controller, HUD, Config or Build.cs file changed. Native source remains uncompiled and in-game behavior unverified. Renderer packaging is not triggered by these unmounted standalone state files.

Canonical source SHA-256 values at review:

| Source | SHA-256 |
| --- | --- |
| `packages/api/src/companion-game-social.ts` | `fed80430466cc42f622aeac0bfa7d06a173ed3952b21296baf6f42ea5f50d293` |
| `packages/api/src/companion-game-teams.ts` | `3865698f96a385dfe2e5c2ebb311c72eb1b6d423451fac3e693ac285dc8b4a21` |
| `packages/utils/src/companionGameSocial.ts` | `c1833ae412a501aceec19c45cf0c90b9c9e18fda4f61ed14d1000d2611b3892a` |
| `packages/utils/src/companionGameTeams.ts` | `7b48fd0125415d7210e8042c87f4136fc05f9c3c8faa032750d4dd7190f32146` |
| `packages/utils/src/companionGames.ts` | `8c375da18a2df74a76fb2b758bd2ec87c28be1734a82ccc06d9443ea854a3b2a` |
| `apps/backend/convex/companionGameSocial.ts` | `e01e2d899ac2359023f585de7155b913dc80c1cf58d00aac6afc32ff0aec3711` |
| `apps/backend/convex/companionGameSocialReputation.ts` | `9fd8bcc2b53ed6d4c5ab3df78fc0f0e1e92a045ef4ca053278196cb9e5d7414c` |
| `apps/backend/convex/companionGameTeams.ts` | `abaf86d039ce4a6947f75950003cc4d3b773259d8d4115c27a20a39740a83ccf` |
| `apps/backend/convex/lib/companionGameTeamsModel.ts` | `0c82dc0e21f9590926283ea5610f5699a9a838f64bece6dc9838ce0bf4d5f58e` |
| `apps/backend/convex/lib/companionGamePortalModel.ts` | `f8cd46b61fd1fc6237c5359e6c075ec4d100eae32c871fc951667eef5f43d512` |

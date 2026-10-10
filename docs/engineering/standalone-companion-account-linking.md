# Standalone Companion account linking

October 10, 2026. Backend and client source contract. Not deployed or exercised against a running backend.

## Scope and activation

`apps/backend/convex/companionGameAccess.ts` implements the pairing, grant and read-only snapshot operations. All are Convex mutations. This makes each request check server time and commit rate-limit decisions; snapshot requests do not write Companion, Home, care, inventory or economic state.

Creation, preview, approval, exchange and snapshots return `disabled` unless `STANDALONE_COMPANION_ENABLED` is exactly `true`. Its optional declaration is in `convex.config.ts`; the local generated environment type has the matching field. No deployment setting was changed. Enable access only in a separately authorized verification or release pass after client consent and session handling pass acceptance.

The only scope is `companion_home_read`. Pairings and grants store it explicitly. Grant credentials belong to `companionGameGrants`, never `userSessions`, and do not authenticate existing account APIs. The game never receives the approving account's session token.

## Pairing and consent

The approving client must use an existing, valid, device-bound Baddie `mobile_app` session. This includes the existing Baddie web client when it uses that session class. TV, admin and advertising sessions cannot approve. Account identity comes from the verified session, not an account ID supplied by the game.

All operation names below have the `companionGameAccess:` prefix. Arguments marked as secrets must stay out of URLs, logs, analytics, renderer objects and Blueprint state.

| Operation | Caller and input | Result |
| --- | --- | --- |
| `begin` | Game: `deviceId`, `deviceName` | `pending`, public `pairingToken`, private `deviceSecret`, comparison `displayCode`, scope, device name, expiry and poll interval. |
| `preview` | Baddie: `pairingToken`, account `sessionToken`, approving `deviceId` | Pending device details, scope, saved library revision, Companion ID/name and Home ID/name for the approval sheet. |
| `approve` | Baddie: preview arguments plus `scope`, `expectedRevision`, `characterId`, `homeRoomId` from that preview | Records consent only if those values still match that account's active saved Companion and Home. Returns `{ ok: true }`. |
| `reject` | Baddie: preview arguments | Cancels a pending request. |
| `exchange` | Game: private `deviceSecret`, original `deviceId` | `pending` with a poll interval, or `connected` with private `gameToken`, `grantId`, scope and expiry. |
| `cancel` | Game: private `deviceSecret`, original `deviceId` | Cancels the pairing and revokes its issued grant, if any, while the pairing remains available and unexpired. |

Pairings expire after five minutes. The eight-character display code is for comparing the two screens, not a lookup or exchange credential. The canonical link is `baddie://companion/connect?token=<64-character-lowercase-hex-token>`. It contains only the public pairing token. The Baddie parser rejects extra parameters, alternate encodings, credentials, ports, fragments and noncanonical addresses. Game-side QR artwork and login presentation are part of the separate Windows gameplay work, not this transport patch.

The approval sheet must show the server's device, scope, Companion and Home, and call `approve` only after an explicit action. Clear its preview when the approving account or session changes. `selection_changed` requires a fresh preview and consent; it must not trigger automatic approval of the replacement selection. Cancelling a sheet can simply close it; `reject` is available for an explicit rejection.

An account without a saved, valid library returns `unavailable`. The game does not manufacture or persist a default Companion. Save the Companion and Home in Baddie first.

## Grant lifetime and retries

Exchange pins the approved Companion ID and Home ID. It rechecks the approving session, account moderation and saved selection. The grant lasts at most 15 minutes, capped by the approving session's expiry at issuance. There is no refresh or persistent sign-in flow.

The server stores hashes of the public pairing token, private device proof and game token. It derives `gameToken` with HMAC-SHA-256 using the device proof and a per-grant random nonce under a fixed scope/version prefix. It stores the nonce, not the recoverable game token or proof. Retrying exchange with the original proof and device ID recovers the same credential and expiry while the pairing is unexpired. A revoked or expired grant cannot be reissued through that pairing. Concurrent exchanges update the same pairing transactionally.

Requests must carry the correct device proof, not just a claimed device ID. This is credential possession, not hardware attestation. A device that loses the begin response starts a new pairing. A device that loses its proof or misses the exchange recovery window pairs again; it cannot recover credentials through the public approval token.

Timed responses include `serverNow` and `expiresAt`. A client should conservatively bound its local grant lifetime by the server's remaining duration, accounting for request elapsed time. Server checks remain authoritative on every snapshot request.

## Read-only snapshot

`snapshot` requires `gameToken`, `deviceSecret` and the original `deviceId`. It rechecks the grant, approving account session and moderation before reading the current saved library. It returns:

```text
status: ready
scope: companion_home_read
grantId, expiresAt, serverNow
revision: current saved library revision
library:
  version: 1
  characters: [the approved saved Companion]
  rooms: [the approved saved Home]
  activeCharacterId: approved Companion ID
  activeRoomId: approved Home ID
  homeRoomId: approved Home ID
```

This projection uses the existing `companions:snapshot` entitlement rules through `lib/companionLibraryProjection.ts`. The app query delegates to the same helper and retains its existing response fields and default-library behavior. The game receives no other saved characters, rooms, account metadata or entitlement lists.

Current appearance and furniture edits are reflected on the next read, including revoked wardrobe and room-item entitlements. Switching Baddie's active Companion does not silently change the grant's approved Companion. Removing that Companion, changing Home or losing the saved library returns `selection_changed` without private snapshot data. Re-pair to select a different Companion or Home.

The returned `library` and `revision` fit the existing Unreal snapshot reader. This does not authorize gameplay entry, movement, visits, library saves, care, purchases, rewards or economic writes. It does not establish Unreal asset support for a saved appearance or room layout.

## Revocation and client state

| Operation | Authority | Behavior |
| --- | --- | --- |
| `connections` | Current Baddie account session and device | Lists at most ten own unexpired, unrevoked issued grants, without tokens or proofs. Issuance metadata is not a guarantee that the parent session or selection still permits a snapshot. |
| `revoke` | Current Baddie account session and device, plus `grantId` | Checks ownership and revokes that grant. Another session belonging to the same account can revoke it. |
| `disconnect` | Game token, device proof and device ID | Revokes the game grant. The game must clear local credentials and private state even if this request fails. |

Revocation and cancellation remain available while the feature is disabled. Signing out or revoking the approving Baddie session also prevents further grant reads without refreshing or extending that account session. Account bans and deactivation are enforced through the existing authentication helper.

There are at most ten unexpired, unrevoked issued grants per account. An invalidated parent session does not itself delete a grant; it can still be listed and revoked. Begin requests perform bounded cleanup of expired pairings and grants. Existing centralized rate-limit buckets govern creation, account actions and device requests. On `slow_down`, wait `retryAfterMs`; normal exchange polling uses the returned interval and stops after connection.

The Unreal session owner clears private state on `disabled`, `denied`, `expired`, selection loss, sign-out and configuration changes. It discards late responses from older session generations. Only a transport failure, temporary service error or throttling can retain stale presentation within the same locally unexpired grant. A successful response for a previous session cannot repopulate the current one.

## Baddie client

`apps/mobile/src/features/companion-linking` owns the approval sheet, canonical-link parser and connection list. `packages/api/src/companion-game-access.ts` provides the typed account operations through the existing Convex client, with automatic retries and query-cache invalidation disabled for these requests. Consent and connection records are not persisted or queued offline.

The native link host is mounted inside the existing platform providers. The camera recognizes Companion links without changing TV approval. You's Security and permissions menu opens Companion connections on non-TV devices; the new section is registered in browser navigation. On native and web, a person can paste the game link into that section and review it in the same dedicated approval sheet. Approval shows the account, device, comparison code and saved Companion/Home, plus the read-only scope and maximum duration.

Account changes clear previews, connection rows and pending actions. Approval checks the current stored session again immediately before submission. A changed selection requires another preview and explicit consent. Failed approval or revocation keeps its sheet open with an actionable error. Cancellation closes the consent sheet without approving or issuing a rejection on the person's behalf.

The connection list reads on opening, explicit refresh, reconnect and foreground return. It retains same-account rows after a transport error and distinguishes an unavailable read from an empty list. The `connections` response includes `serverNow`; clients subtract request elapsed time when calculating expiry. Expired rows fall out locally without a backend polling loop. Revocation remains available when creation and snapshots are disabled.

## Unreal client

`UBaddieCompanionSession` in `apps/companion-unreal/Source/BaddieCompanion` is a game-instance subsystem. The HTTP request follows the installed Convex client's `convex_encoded_json` mutation envelope and accepts only a configured HTTPS `<deployment>.convex.cloud` origin. It checks HTTP and envelope status before consuming the operation result. Requests time out after 15 seconds; decoded responses are size-limited before the existing snapshot reader runs. Custom backend domains and persistent sign-in are not implemented.

The subsystem reads `[BaddieCompanion]` keys `ConvexOrigin` and `bAccountLinkingEnabled` from game configuration. The latter defaults to false; no enabled configuration or live origin is committed. `Configure` is for trusted C++ application setup, never an untrusted game link. No request runs just because the subsystem exists.

The gameplay owner calls `BeginPairing`, renders only `GetConnectionView`, and uses `Disconnect` when leaving or switching accounts. `OnConnectionChanged` signals state changes; `CopySnapshot` supplies the scoped saved selection and clears its output when access is unavailable or locally expired. Consumers must remove retained private presentation on that failure. These C++ interfaces expose no game token, proof or full-account credential. A new pairing creates a new device identifier and session generation.

Exchange uses the returned polling interval. Snapshot refresh is limited to once per five seconds; server throttling delays subsequent reads. A throttled begin request blocks another begin for the returned interval and exposes `RetryAfterSeconds` in the public view. Deadline checks use both monotonic and wall clocks and subtract request duration. Snapshot responses may shorten, never extend, the original grant. The client requires exactly one character and Home, case-sensitive selection/grant IDs and nondecreasing revisions. Lost exchange responses retry with the original proof while the pairing remains valid. Sign-out clears local state immediately and makes a separate bounded best-effort cancellation/revocation request.

Credentials and private snapshots exist only in process memory. Restarting the game requires a new pairing. No credential is written to an INI file, SaveGame, log, analytics event or Blueprint property. Asset mapping and gameplay admission still belong to their separate owners; a readable snapshot does not grant permission to move in a server game.

## Remaining work

The Baddie approval/connection controls and Unreal transport/session owner are implemented in source. Persistent credential storage is intentionally absent. Game-side login presentation, engine asset mapping and gameplay admission are being handled separately on Windows and have not been verified by this patch. No tests, compilation, code generation, deployment or engine work ran in this account-linking pass. Regenerate normal Convex metadata during an authorized generation pass.

Native and web rendered verification is incomplete. The existing local servers belonged to the landing site and an unrelated video project, not the mobile app; simulator inspection timed out. No new preview was started. Review still needs narrow/wide layouts, all themes, larger text, keyboard and overlays, plus loading, error, expired, signed-out and populated states. The What's New source records this disabled work without replacing the current public highlights. Embedded renderer code, assets and protocol are unchanged, so renderer packaging is not triggered.

Source review and the scoped diff whitespace check completed. The required What's New guard passed against the backend checkpoint `da544420d`. This does not establish TypeScript, Unreal compilation or runtime correctness.

The verification pass must cover concurrent exchange, lost responses, wrong proof/device, approval revision changes, cross-account revocation, parent-session expiry/revocation, moderation, feature disabling, entitlement revocation, Home changes and stale client callbacks. This list is a handoff requirement, not a claim that those scenarios have passed.

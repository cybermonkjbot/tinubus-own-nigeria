# Standalone gameplay entry

October 10, 2026. Backend source checkpoint `447fe8937a7d32e0e457165ff6926b6586f87909`. Account access remains disabled. The native caller and Baddie gameplay-approval controls are not implemented by this checkpoint.

## Capability and consent

`companion_gameplay` is a separate, version-one capability in the existing pairing/grant tables. It is not an account session. The original `companion_home_read` and `companion_apartment_read` validators, default begin request, consent and snapshots retain their read-only meaning. No Home credential can enter or renew gameplay presence.

The game uses `companionGameAccess:beginGameplay` to request this scope. Baddie must use `previewGameplay` and `approveGameplay`, with the displayed scope, `consentVersion: 1`, `voice: false`, revision, Companion ID and Home ID. Approval is an explicit gameplay choice, never a renamed Home approval. The existing read-only preview/approve endpoints reject gameplay pairings. The native and mobile follow-up chats share the exact route `baddie://companion/play?token=<64 lowercase hex public pairing token>`. The existing `companion/connect` route stays read-only. A route is only a UI choice; approval still requires the server's matching gameplay preview. Device proof and account credentials never belong in either link.

The existing `exchange`, device proof, token hashing/HMAC, five-minute pairing window, fifteen-minute maximum grant, parent-session expiry bound and ten-grant account limit are reused. Gameplay credentials have their own HMAC scope prefix. The approving device-bound account session and moderation are checked without refreshing or rotating that session. Its credential never enters a client response or native request.

Gameplay pins the currently active saved Companion and Home. Changing the active Companion invalidates gameplay access, because the existing world owners resolve the active character. This differs deliberately from a Home read grant, which can retain a previously selected Companion. Missing library, removed selection, changed Home, revoked parent session or moderation loss prevents new gameplay requests. No unsaved default is created to satisfy pairing.

## Entry and presence

`companionGameplay` accepts only the gameplay credential, original device proof and device ID. The client also supplies an `entrySequence`, a positive integer no greater than one billion. The first sequence is one. Each new entry follows an explicit leave/cancel and increments it by one.

| Operation | Implemented behavior |
| --- | --- |
| `enter` | Reserves city entry and joins existing call presence in one mutation. Optional `crewThreadId` uses existing crew membership checks. A join failure rolls back its reservation. Voice is off and no RTC token is issued. |
| Repeated `enter` | A matching sequence/crew request returns the same still-valid call, participant and expression-session identity. It does not renew presence, rotate that identity or recreate a lost lease. |
| `heartbeat` | Rechecks grant, parent, selection and exact admitted lease, then invokes the existing presence renewal. It accepts no movement, reaction, voice or combat arguments. |
| `leave` | Releases only the recorded sequence and exact lease generation. A cancellation that arrives before its entry records the sequence as left, so a delayed entry cannot reopen it. Older cancellation requests do not affect newer entries. |
| `disconnect` | Verifies device proof, clears this grant's exact entry and revokes the grant. Cleanup remains available after expiry, revocation or feature disablement. |

Entry reuses `admitCompanionGameForIdentity` and `reserveGroupSpacePresence`. The original app endpoints now call those same implementations. They retain existing feature flags, public-city registry, crew/contact rules, capacity, entry/join rate limits, call conflict handling and scheduled expiry. Native entry requests a new presence and cannot reuse another live app/device presence, even when it belongs to the approving account session.

The grant records the resulting room/thread/call/participant/expression-session tuple. A replacement expression session, participant, room call or user session invalidates it. The successful response reports the grant and entry identities, approved Companion/Home, server time and bounded expiry. It contains neither an account credential nor a voice token.

Presence and guest expiry cannot extend beyond the gameplay grant or current approving-session deadline. Renewal uses the existing call heartbeat implementation; it does not authenticate by replaying the parent's token. Exact-lease cleanup uses the existing kitchen, transit, seat, guest and call-departure implementation. Revocation, pairing cancellation and expired-grant pruning invoke that cleanup. A stale grant cannot delete a replacement lease with another expression-session identity.

`gameplayConnections` lists this account's gameplay grants without credentials. The original `connections` endpoint lists only the two read scopes, so an older client cannot label gameplay access as read-only. The existing account-owned `revoke` operation handles either kind.

## Current limit and next integration

These endpoints establish entry and presence only. They do not authenticate existing account APIs, return a world snapshot, enter an instance, accept movement, ready a map, drive a vehicle, run missions, change inventory, award rewards, purchase anything or enable voice. Existing gameplay rules must own each later operation; a client-side `joined` flag cannot authorize it.

Baddie's separate consent/listing flow is implemented at `64bed8b7e`, with parent QR integration at `3cc0e74df`. Native session `8f6c3322a` remains separate from read-only `UBaddieCompanionSession`. Its source review covers in-memory credentials, old request rejection, bounded lifetimes, same-sequence retries, disposal cleanup and access-loss clearing. `entry_lost` requires ending that entry before another explicit entry; it does not silently take over or reapprove. Native connection-UI integration is now assigned, with configuration and authority still disabled. Connect the pawn/HUD interfaces only after the separate world/encounter contracts can supply accepted state.

The complete world and mission adapters remain unfinished. Imported native content, animation, render readiness and the apartment-to-outside walk-through are separate dependencies. This backend checkpoint does not activate any native pawn or change the mobile/web renderer.

## Evidence and delivery

The parent traced the existing `companion-games.ts` call owner, `companionGames`, `chatCallSpaces`, `companionGamePortalModel`, grant authentication and schema. The lockfile pins Convex 1.43.0. No package install, test, typecheck, lint, compilation, code generation, deployment or live backend call ran. Independent reviews covered the seven-file diff from `95715bc6a5440ef06bad9c52dd183dde8d2b3666` to `447fe8937a7d32e0e457165ff6926b6586f87909`.

Legacy-behavior review found no actionable issue. Security review found two P2 lifecycle defects. First, changing the selected Companion denied later native requests but left its existing participant alive until expiry, allowing the snapshot to show the newly selected character. Second, exact participant cleanup could expire a newer phone portal reservation owned by the same approving session.

Correction `c0a68c95fd3ece1e1b51507fda2c8a6e064b6329` retires invalidated native presence in the library-change transaction and tracks guest-reservation generations separately from participant expression generations. Independent re-review found no actionable defect and confirmed optional stored fields, no new indexes, unchanged public endpoint shapes and preserved legacy account behavior. A separate native-session review against that corrected backend found no new actionable defect. Transaction interleavings and runtime compatibility remain unverified.

The parent's proof-authenticated pairing cancellation correction also passed independent source review. Its bounded retention, pruning tradeoff and legacy-data limit are recorded in `standalone-companion-account-linking.md`. Native redirect-safe transport/configuration acceptance remains open. These are disabled source changes, not deployed behavior claims.

This checkpoint is on Mac. No Windows source transfer or backend deployment is claimed. Future metadata generation and runtime acceptance require their authorized pass. The source/contract review and eventual native handoff do not establish a playable game.

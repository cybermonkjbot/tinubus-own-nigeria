# Native gameplay session handoff

October 10, 2026. Source implemented and inspected against backend checkpoint `447fe8937a7d32e0e457165ff6926b6586f87909`. No live requests, compilation or gameplay acceptance.

## Owned files and boundary

- `apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionGameplaySession.h`
- `apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionGameplaySession.cpp`
- `docs/engineering/unreal-gameplay-session-handoff-2026-10-10.md`

`UBaddieCompanionGameplaySession` is separate from the read-only session. It defaults to disabled, reads no existing account-linking configuration and has no UI or configuration caller. Its subsystem initialization registers native lifecycle observers and a ticker but issues no request. No existing file, backend endpoint, project configuration or expedition character file changed.

The public interface is `Configure`, `BeginPairing`, `Enter`, `Leave`, `Disconnect`, `GetConnectionView` and `OnConnectionChanged`. Configuration requires a trusted HTTPS Convex deployment origin and an explicit enable flag. Pairing and entry require separate calls in the foreground. `Joined` reports call presence only. There is no public credential, lease-identity or accepted-world accessor, and no pawn, controller, world-clock or MissionAdmission binding.

The approval link is exactly `baddie://companion/play?token=<64 lower-case hex public pairing token>`. It is a routing hint, not authority. Public status contains only device name, display code, approval link, fixed error text, countdowns and stale state. Device proof, gameplay credential and admitted identities remain in private memory. No account token is accepted, obtained, persisted or placed in a request. No code logs request bodies, responses or credentials.

## Endpoint review

All seven backend entry/presence files still matched `447fe8937` at source review. The client follows these branches:

| Endpoint | Client behavior |
| --- | --- |
| `companionGameAccess:beginGameplay` | Sends only device ID/name. Checks `pending`, exact gameplay scope, device name, public token/proof/code formats, polling interval and five-minute bound. Rejects disabled/denied/unavailable/expired/selection-change/connection-limit responses. `slow_down` blocks a new begin until its delay. A failed begin is not retried automatically because its proof may be unknown. |
| `companionGameAccess:exchange` | Polls with original proof/device ID. Pending replies cannot extend pairing time. Connected replies require gameplay scope, `bcg1_` plus 64 lower-case hex, grant ID and a maximum fifteen-minute lifetime. A lost response retries the same proof. Approval stops at `Authorized`, not automatic entry. Terminal failures clear state and attempt cleanup; throttling delays the next poll without extending pairing time. |
| `companionGameplay:enter` | First sequence is one. Sends grant/proof/device ID, the occupied sequence and optional pinned crew ID. Transport uncertainty and throttling retry that same request identity. A joined reply must match grant/sequence, voice-off scope and valid saved/lease identities. Saved user/Companion/Home identities remain pinned across entries in the same grant. No world or mission state is inferred. |
| Repeated or lost entry | The backend retains the lease generation for a repeated sequence and does not renew it. The client does not change sequence or crew while uncertain. `entry_lost` starts a leave for the occupied sequence and requires a later explicit `Enter` after acknowledgement. It never silently takes over another presence. |
| `companionGameplay:heartbeat` | Sends only grant/proof/device ID and the occupied sequence. Checks ready/sequence/boolean result, timestamps and retry delay. Only a successful newer receipt can extend the bounded presence deadline. Declined or older receipts may shorten it, never extend it. Structured access loss or a Convex error ends the connection; transport errors retain only the existing bounded, stale presence status. |
| `companionGameplay:leave` | Clears local lease presentation before sending the exact occupied sequence. `{ok:true}` retires that sequence locally; only then can an explicit entry increment it. Lost acknowledgements retry the same leave. Cancellation can precede the original enter, using the backend's left-sequence record. Old callbacks cannot authorize a newer entry. |
| `companionGameplay:disconnect` | After local clearing, detached cleanup attempts the occupied sequence's leave, then grant revocation. It uses original proof/device ID and works without the local enable flag or an unexpired grant. Backend exact-lease cleanup remains authoritative. |
| `companionGameAccess:cancel` | Cleans up pairing when no gameplay credential is known. A late begin response can supply its missing proof solely for cancellation; a late connected exchange can supply its credential solely for revocation. Neither can restore a session or approval link. |

The implementation uses UE HTTP, JSON, weak UObject references and `FTSTicker`, following the existing read-session transport. It sends Convex mutation envelopes, requests no cookies, checks the exact response URL and rejects bodies above 32 KiB before parsing. This is a parser bound, not a streaming receive-memory cap. Requests have a fifteen-second timeout. Fixed diagnostics never display backend exception text.

Grant and pairing lifetimes use the smaller of monotonic and wall-clock remaining time, subtracting the whole observed request duration. Observed expiry is latched so a later wall-clock correction cannot revive it. Entry can only shorten grant time. Presence uses the canonical maximum initial setup window of 180 seconds, retains the existing 45-second renewal behavior through server receipts, and never exceeds the retained grant deadline. A reply arriving after local expiry cannot revive access. Native polling intervals are scheduling policy, not authority.

Foreground/background and Slate activation events clear local state and request cleanup. Returning to the foreground does not pair or join again. Request generation and pointer checks reject old callbacks; a duplicate callback for a still-current entry cannot cancel that entry. Retired requests retain their bounded completion callback so unknown proofs or credentials can still be cleaned up after subsystem disposal.

Cleanup retains only retired in-memory credentials, not the subsystem. It retries transport failures or valid server throttles at most twice per phase within a five-minute monotonic/wall deadline. Successful cleanup is not claimed from dispatch. HTTP requests and scheduled cleanup may retain their private copies until completion or the next resumed ticker opportunity. Process termination or an unavailable network can prevent delivery; server expiry remains necessary.

## Contract findings and enablement gaps

Independent integration/security review of native checkpoint `8f6c3322a679fb1d34d96d2294ee6300d1103f47` against corrected backend `c0a68c95f` found no new actionable defect. It covered pairing/selection, response acceptance, entry and replacement-lease generations, delayed cleanup, expiry, focus loss, teardown and private credentials. This does not establish runtime behavior or transport enablement.

The reviewed client implements the declared endpoint branches without changing the backend. Two backend behaviors need to remain visible in integration:

1. `inspectGroupSpacePresence`, presence renewal and feature/admission checks can throw Convex errors rather than return the endpoint's structured failure status. The client fails closed and requests cleanup on these envelopes. It cannot classify every such failure as recoverable `entry_lost` or preserve a server-provided retry hint that is not returned.
2. At the reviewed backend checkpoint, `companionGameAccess:cancel` returned `expired` after the pairing window, before revoking a previously exchanged grant. A permanently lost exchange reply could therefore leave an unused grant until expiry or account revocation. The parent's cancellation and proof-record retention correction passed separate source review. See `standalone-companion-account-linking.md` for the bounded pruning and legacy-data limits. This client already sends proof-only cancellation and needs no broader credential access. No presence is entered without a received credential.

Transport enablement also needs a platform review. The inspected installed Curl implementation enables redirects at `CurlHttp.cpp:183`; the portable request interface inspected here exposes no redirect-disable option. Exact response-URL checking is not a guarantee that a redirect never forwarded a request body. Keep configuration trusted and the client disabled until the deployment's no-redirect behavior and platform credential-safe transport policy are accepted. This patch does not change shared HTTP configuration or replace Unreal's transport.

## Native evidence and remaining work

Read-only inspection used installed UE 5.8.3 under `C:\Program Files\Epic Games\UE_5.8`. `SlateApplication.h:1611` provides `IsActive`; line 1691 provides the activation event. `CoreDelegates.h:352` and 361 provide reactivation and foreground callbacks. `IHttpRequest.h:151`, 215, 318 and 419 provide the cookie option, request option setter, timeout and game-thread completion policy. Existing module dependencies already include HTTP, JSON and Slate. No remote files changed or remote work was dispatched.

The separately presented Baddie gameplay consent and QR source have passed review. The existing controls chat now owns native connection/entry UI integration. Trusted configuration remains unavailable pending credential-safe transport acceptance; preserve the disabled backend gate. Future world, mission, movement, instance and rendering owners need their own accepted contracts; this session deliberately supplies none. The expedition pawn remains unbound.

Source review covered every endpoint branch above, exact argument sets, scope/identity checks, all teardown paths, late begin/exchange/enter completions, cancellation before entry, sequence reuse after acknowledgement, expired in-flight receipts, declined heartbeat renewal, focus loss and cleanup without an active grant. No tests, typechecks, lint, builds, compilation, code generation, engine launches, imports, deployments or live configuration changes ran. Windows delivery, native UI, lifecycle/runtime failure exercises, redirect/diagnostic privacy acceptance and rendered/device verification remain pending. No playable-game result is claimed.

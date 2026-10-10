# Native HTTP response bounds

October 10, 2026. Source-only continuation of the credential transport research. Account and gameplay access remain disabled. This change does not make credential transport activation-ready.

## Implemented boundary

Both native session owners bind Unreal's existing `SetResponseBodyReceiveStreamDelegateV2` before dispatch. A request that cannot install the delegate is not sent. The shared private `BaddieCompanionHttpResponse` helper owns a fixed-size byte buffer through thread-safe shared references. Its native critical section serializes receive access and completion reads; no UObject is touched on the receive thread.

The GameplaySession buffer remains 32,768 bytes. Home/apartment Session keeps its existing 278,528-byte envelope limit, equal to the 256 KiB snapshot limit plus 16 KiB. The helper rejects a negative chunk length, a nonempty null pointer or a chunk larger than the remaining capacity before copying. It latches rejection and sets the V2 processed length to zero, which makes Unreal's receive archive fail. An exact-limit body fits; any subsequent nonempty chunk fails. It does not trust Content-Length or grow the buffer from a server-provided length.

Successful completion parses the retained bounded body, not `IHttpResponse::GetContentAsString`, which is empty when streaming is enabled. Empty bodies, raw NUL and a failed UTF-8 round trip are rejected. Both owners compare `GetEffectiveURL()` to the exact configured mutation URL before accepting a success envelope. Existing operation, scope, generation, deadline, entry and saved-selection checks remain in place.

Foreground requests, retired gameplay completions and detached cleanup all use the same bounded factory. The request's stream delegate retains the buffer independently of the session. Gameplay cleanup also captures it for result parsing. Home cleanup ignores the result but still bounds reception. The buffer has no back-reference to the request or session.

Receive rejection is terminal for the foreground connection, rather than a retryable network error. Gameplay cleanup does not retry a rejected body; it retains the existing leave-to-disconnect progression. Ordinary network failures, temporary HTTP errors, throttling and generation handling retain their previous policy. A local delegate-binding failure skips dispatch and exposes a fixed unavailable state; detached cleanup remains best effort.

## Source evidence and limits

The installed UE 5.8.3 research in `unreal-credential-transport-research-2026-10-10.md` records the exact HTTP source hashes and effective-URL contract. A bounded SSH read in this continuation confirmed `IHttpRequest.h` lines 101-118 and 260-287, plus `IHttpRequest.cpp` lines 20-54. The V2 delegate is `void(void*, int64&)`; `FArchiveWithDelegateV2::Serialize` sets archive error when the processed length differs from the supplied length. The header states that receiving happens off the game thread and bypasses the normal response payload. An initial multiline stdin read returned no source output; the subsequent single-line read returned those declarations and implementation. Neither changed remote files.

Source inspection traced all request factories and completion sites, zero/exact/excess capacity arithmetic, signed length narrowing, callback ownership, failure clearing, cancellation and cleanup progression. No tests, compilation, executable HTTP checks or rendered inspection ran. Independent review and Windows handoff remain pending.

The byte budget covers retained application body bytes only. It does not bound TLS/curl buffers, aggregate headers, decoded text or parsed JSON memory. Buffer destruction clears its used bytes, but this is not a guarantee of secret erasure from all request/parser copies.

Redirect following remains unresolved. Effective-URL checking rejects a result after transmission; it cannot prevent credential forwarding or a redirect chain returning to the original URL. Engine diagnostic privacy, strict per-request TLS/cookie/header policy and loaded-binary acceptance also remain open. Read the transport research before those changes. No installed engine source, global HTTP settings, trusted origin, enable flag, backend endpoint or permission changed here.

## Owned files and next handoff

The source patch contains the new private `BaddieCompanionHttpResponse.h/.cpp`, both session `.cpp` consumers, this record and the two existing session/account handoff updates. Public headers and connection UI remain unchanged. Include the helper and both consumers together in any reviewed Windows transfer; a consumer-only transfer leaves an unresolved include/dependency.

Production renderer inputs and mobile/backend behavior were not changed. Renderer packaging and What's New are not triggered. Imported content, compilation, device/runtime acceptance and release remain separate work.

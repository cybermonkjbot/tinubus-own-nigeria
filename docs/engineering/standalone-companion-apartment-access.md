# Standalone apartment read access

October 10, 2026. Source-only consent, projection and Unreal session follow-up for the connected apartment. Access remains disabled. No deployment or live acceptance has run.

## Separate consent

`companion_apartment_read` covers the approved Companion and the complete saved room inventory shown in Baddie's approval sheet. It is separate from `companion_home_read`, which still covers exactly one pinned Home. Omitting `scope` from `companionGameAccess:begin` retains the original Home-only request. Existing pairings, grants, credential derivation and Home snapshot responses retain their meaning.

Apartment approval lists each saved room and identifies Home. It sends the displayed scope, revision, Companion ID, Home ID and room IDs. The server requires the pairing's requested scope and the exact complete room-ID set from the current saved library. A subset, duplicate, extra room or changed revision cannot be approved. Room names are display text, never identifiers. Both the decision and issued grant retain the approved room IDs in an optional schema field so existing Home-only rows remain valid.

Adding or removing a saved room requires fresh apartment consent. Changing Home or removing the approved Companion also invalidates the selection. Furniture, appearance and room-name edits within the approved selection refresh through the existing projection. Switching the active Companion does not replace the grant's approved Companion. The apartment projection retains the current active room separately from Home.

## Existing account and device owners

The existing device-bound Baddie session authenticates approval. No account ID supplied by the game establishes ownership. Exchange rechecks the approving session, moderation, scope and complete room inventory before issuing or recovering a grant. A recovered grant must match the pairing's scope and room IDs. Apartment credentials use a separate HMAC scope prefix; the Home prefix and token format are unchanged.

Both scopes retain the five-minute pairing window, maximum fifteen-minute grant, parent-session expiry bound, device proof, rate-limit buckets and ten-grant account limit. Neither scope authenticates existing account APIs or authorizes gameplay, edits, visits, purchases or world entry. Revocation and cancellation remain available while creation and reads are disabled.

## Apartment projection

The game uses `companionGameAccess:apartmentSnapshot` with its existing `gameToken`, `deviceSecret` and `deviceId`. The endpoint accepts only an apartment grant. The original `snapshot` endpoint accepts only a Home grant and keeps its one-room projection.

Before returning any room data, the apartment endpoint rechecks device proof, expiry, revocation, parent-session/account access, saved-library validity, pinned Home/Companion and the complete approved room-ID set. Missing consent metadata or a changed inventory returns no snapshot. The shared library validator bounds the inventory to twelve saved rooms. Reads never create default rooms, hallway furniture or a replacement living room.

A ready response contains the apartment scope, grant ID, server time, expiry, revision, approved `roomIds`, and a library with one approved Companion and every approved saved room. It retains saved IDs, names, room-local coordinates, furniture and Home/active-room identity. `projectCompanionLibrary` applies the same current wardrobe and furniture entitlement rules as the app. No other Companions, account metadata or entitlement lists are returned.

## Client source

The account API distinguishes the two preview/consent types. Apartment consent requires `roomIds`; Home consent does not accept them. The existing native/web approval sheet lists the requested rooms and submits only after explicit confirmation. Account changes, stale responses, expiry, cancellation and actionable errors keep their existing owners. Connection rows and revocation sheets distinguish apartment access from Home-only access. These changes reuse the existing scrollable confirmation sheet and do not change shared overlay styles.

Unreal's `BeginPairing` now takes an explicit `EConnectionScope`, defaulting to Home. The session sends that scope at begin, requires the matching begin/connected/ready scope, and chooses `snapshot` or `apartmentSnapshot` accordingly. Pending exchange currently omits scope; if it supplies one, it must match. Existing Home callers retain the one-character, one-room and active-equals-Home checks.

Apartment responses must contain one Companion and between one and twelve rooms. The separate `roomIds` array must be bounded, duplicate-free and match every decoded room by case-sensitive ID. The first accepted set is retained for the grant. Later responses may reorder rooms or update saved contents, but may not add or remove an ID, replace the Companion or Home, or lower the revision. The existing request generation, request identity, expiry and conservative lifetime checks remain in place. Clearing the connection also clears the approved IDs and all supplied rooms.

Only accepted apartment responses set the private completeness marker in `FSnapshotSelection`. The raw JSON reader creates a new unmarked selection, regardless of payload fields or room count. The Home presentation cache includes that marker in its exact-input identity and passes it to `Apartment.bCompleteHomeInventoryVerified`. It establishes complete read inventory, not authored native content, movement, visits or exterior admission.

The controller and Slate panel now connect this session path to explicit controls. `Connect Home only` and Enter from the device-name field retain Home scope. `Connect apartment` requests apartment scope. From a connected Home, that action opens the existing dedicated confirmation before clearing the old grant and requesting apartment approval. `Pair again` retains the current scope. Cancellation and invalidation clear the pending scope with the rest of the modal draft. The title, loading/ready text and confirmation copy distinguish Home and apartment reads. Both connection buttons use the same device-name, disabled, pending and manual-retry guards; offline inspection hides them.

These changes are integrated with reviewed native source `3f0318fb8`. Re-pair, disconnect, expiry, offline entry and teardown retain its native-content clearing. Its corrected camera/navigation copy methods remain unchanged. The combined client reached Windows `87295378a1ce93db180a879b87909dc7de3d050f`, pinned to Mac `581f0b66c4a887763a5922474547bad32750d0c1`. The parent independently matched all 81 delivered/reference files at 13:29:59 UTC on October 10, with a clean Unreal worktree. Backend/mobile source was not transferred. Neither the controls nor the native owner enables account access, native loading or gameplay.

## Review and remaining work

Source review covers scope isolation, replay checks, inventory pinning, optional schema compatibility, bounded reads, entitlement reuse, account/proof ownership and the unchanged Home projection. The installed Convex package is 1.43.0; this patch uses the existing validator and mutation APIs and adds no dependencies. No tests, typechecks, compilation, code generation or backend calls ran. The Convex implementation skill guided validator, schema and ownership handling; repository rules defer its suggested checks and deployment. The What's New guard passed against `df2570de8`, and the scoped whitespace diff check passed.

Before editing the approval copy, the available browser inventory contained no preview tabs and the running Simulator could not be inspected because its UI read timed out. No preview process was started. Narrow/wide, native/web, theme, text-scaling, scrolling, account-switch and error-state inspection remain incomplete.

The Unreal follow-up received source and whitespace-diff review only. Review covered both connect actions, scope-preserving re-pair, modal cancellation/expiry, automatic/manual retry separation, offline controls and preservation of the native lifecycle fixes. It reuses the existing UE JSON, HTTP, ticker and Slate owners and adds no dependencies. Its session-private helper names avoid collisions with the snapshot reader when Unreal combines source files.

A bounded Windows process read found no running Unreal or BaddieCompanion preview before the control edit. None was launched. Pointer/keyboard interaction, focus return, narrow/wide wrapping, text scaling and rendered states remain unverified. No tests, typechecks, lint, compilation or runtime requests ran. Source handoff is complete. Deployment, live consent/read acceptance and engine verification require separate authorization. Production renderer code, assets and bridge contracts are unchanged, so no renderer packaging is required. This Unreal-only follow-up does not change mobile behavior or require another What's New update.

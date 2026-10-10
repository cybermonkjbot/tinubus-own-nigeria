# Standalone companion in Unreal

October 10, 2026. Internal implementation plan and Windows handoff.

## Direction

We are starting a separately installed Unreal game connected to an existing Baddie account, using that person's saved Companion. Baddie remains our social app. The game does not create another account, companion library or economic ledger. Windows is the first development target; mobile delivery is a later platform pass. Browser streaming and GPU deployment are outside this first slice.

The first playable slice is one admitted Companion in the person's saved Home. Start with appearance, room selection, camera and local movement presentation. Add authoritative game entry and movement before porting city systems. Keep the existing mobile/web renderer working throughout.

## Existing owners

Paths below are relative to the repository root.

| Concern | Current implementation | Unreal integration rule |
| --- | --- | --- |
| Account session | `apps/backend/convex/lib/auth.ts`, `apps/backend/convex/lib/clientTrust.ts` | Verify the approving Baddie session server-side. The game receives a separate scoped grant, not the phone's account token. |
| Companion and rooms | `apps/backend/convex/companions.ts`, `packages/utils/src/companion.ts` | Reuse saved IDs, revision and entitlement-filtered appearance/furniture. A character ID is account-scoped, not a global account ID. |
| Game lifecycle | `apps/mobile/src/services/companionGameService.ts` | Follow its admitted-character selection and ownership rules. A joined game's character can differ from the library's currently active character. |
| World, care, missions and economy | Existing `companion*.ts` backend modules and their models | Keep permission checks, rewards, state transitions and write arbitration server-owned. Port presentation independently. |
| Embedded scene messaging | `packages/utils/src/companionSceneBridge.ts` | This is the current renderer protocol, not an external account-linking protocol. Leave it unchanged in this slice. |
| Movement offload | `docs/engineering/cloudflare-gaming-offload.md` | Its admitted social-interior transport does not authorize standalone city gameplay. Do not enable or broaden it for this work. |
| Models and art | `packages/utils/src/companionAssets.ts`, `packages/companion-scene/art/` | Reuse approved sources and stable asset IDs. Verify Unreal import, rig, materials, units and redistribution rights separately. |

`packages/character-runtime` currently depends on Three.js. Its rendering code does not run in Unreal. Shared identity and authored assets are reusable inputs, while the Unreal rig/material/animation adapter still needs implementation.

## Account-linking plan

These steps describe work still to implement, not deployed endpoints.

1. The game creates an expiring pairing request with a device-held proof secret. Show a public approval link/code and device name. Keep the proof secret off the approval URL.
2. Baddie's signed-in app or web flow resolves the request and presents a dedicated approval sheet. The backend derives the account from the verified session, records consent and binds the approval to that request and device proof.
3. The game exchanges the approved request over HTTPS for a short-lived, game-scoped session. A replay from another device fails. Retries after a lost response recover the same grant, without issuing broader or extra access. Recheck approval, expiry, revocation and account moderation at exchange.
4. A scoped snapshot operation derives the account and permitted Companion server-side, then projects the existing companion library and Home. Factor out the existing entitlement-filtered read when implementing this backend adapter; do not copy the companion store or call public functions using a borrowed phone token.
5. The Unreal session owner clears private state on sign-out, account change or revocation and discards late responses from older sessions. Only transport errors within the same valid grant may retain stale presentation. Client-supplied IDs never establish ownership. Later writes require an admitted game context, expected revision and retry identity.

Begin with read-only companion/Home access. Keep Inbox, social publication, purchases and other account operations out of the game grant. Return to the existing Baddie flow for actions outside that grant. Refresh and device revocation need explicit backend rules before a persistent sign-in is shipped. Store any persistent credential in platform secure storage, never Unreal config, save-game files or logs.

The TV pairing flow in `apps/backend/convex/tvAuth.ts` is a useful reference for separate public/private tokens, expiry, consent, rate limits and redemption retries. It issues `deviceClass: tv` sessions, which `requireNonTVSessionByToken` rejects for companions. Preserve that restriction. A game client is not a TV or a renamed full-access mobile client.

## First source slice

`apps/companion-unreal` now contains an Unreal project, a runtime snapshot reader and an editor-only offline inspection commandlet. The reader consumes the existing decoded snapshot shape and preserves the selected appearance and room documents. It does not introduce a production bridge contract or change the embedded renderer.

This gives the Windows implementation a concrete C++ starting point before engine installation and graphics work. It is not an account connection or a playable build. The reader is not a full validator for every retained JSON field; the future server projection and asset adapter must enforce their respective contracts.

No backend, mobile, shared scene data, production asset registry or renderer registration changes belong to this slice. Renderer packaging and What's New changes are therefore not triggered. Future changes to those production inputs must follow the root companion packaging and mobile release rules.

## Next implementation steps

1. Verify the installed Unreal version, compiler, Windows SDK and free space on Cribtest11. If Unreal is absent, choose the supported engine/toolchain combination before a separately authorized installation. Account login and licence acceptance stay with Joshua. Completion means recording real installed paths and versions, not assuming the launcher installed an engine.
2. Implement the scoped pairing and read-only backend snapshot adapter, plus Baddie's approval flow. Reuse existing auth and companion modules. Completion means an approved device can read only the granting account's permitted Companion/Home and an expired, revoked or different-device grant cannot. Run tests and compilation only when explicitly requested.
3. Connect the Unreal session owner to that adapter. Keep tokens out of renderer objects and Blueprint-exposed state. Completion means the admitted saved IDs reach the Unreal reader and session changes cannot reuse another account's data.
4. Import the approved Companion and one supported Home layout into Unreal. Preserve source IDs, scale and coordinate mapping; inspect rigging, outfit fit and room placement before claiming parity. Unsupported saved appearances/layouts need an explicit unavailable result, not a silently substituted character or room. Do not copy third-party art whose distribution clearance is unresolved.
5. Add authoritative game entry and movement through the existing server rules. In-game verification must cover re-entry, account switching, interrupted responses and simultaneous mobile/game activity before broader city work.

## Windows coordination

Cribtest11 replaces the earlier Windows Server VM for this work. Use the Mac launchers `/Users/joshua/Desktop/Cribtest11 Codex.command` or `/Users/joshua/Desktop/Cribtest11 Codex 6.1.command`. They target `20.109.124.208:8766`, verify the bridge certificate as `cribtest11.local`, and set the remote workspace to `C:\dev\cribtest-game`. Keep the VM model selection limited to `gpt-5.4` and `gpt-6.1-sol`.

This is a plain Windows 11 development VM. CPU-only source work is acceptable; WSL and virtualization are not prerequisites for this direction. Keep the old Cribtest VM out of active work. Its recorded checkout, compiler and SDK inventory do not describe Cribtest11.

Continue in `apps/companion-unreal` inside the remote workspace. The previously suggested `apps/bestie-game` name was a proposal, not the implemented project. Inspect the destination before transferring files; the fresh workspace must not be assumed to contain the Baddie repository or the earlier source slice.

### Cribtest11 inventory, October 10, 2026

The authenticated bridge returned this read-only inventory before source transfer:

| Item | Observed state |
| --- | --- |
| OS | CIM reports `Microsoft Windows 11 Pro`, 64-bit, on `Cribtest11`. Version `24H2`, build `26100.9457`. The registry's `ProductName` still says `Windows 10 Pro`; use the CIM caption for the edition. |
| Workspace | `C:\dev\cribtest-game` exists and has zero entries, including hidden files. It is not a Git repository. |
| Engine | No Unreal Engine or Epic Games Launcher found in the inspected standard directories, registry keys or uninstall entries. |
| C++ toolchain | No Visual Studio, Build Tools, MSVC or Windows SDK found in the inspected standard directories and registry keys. |
| Resources | 15.99 GiB RAM; 2 cores and 4 logical processors allocated; AMD EPYC 9V74 processor identification. |
| Storage | 98.9 GiB free on `C:` of 126.45 GiB at inspection. Recheck before any authorized installation. |

A scoped transfer contains `AGENTS.md`, `CONTEXT.md`, this handoff and `apps/companion-unreal`. It is source preparation, not a full repository checkout. Backend and shared-module references above remain in the Mac repository until separately transferred. Continue account-bridge work against those existing modules there; do not recreate them in the Windows source copy.

### Transfer and execution rules

The desktop helper starts an ephemeral remote conversation. Briefing that conversation does not automatically persist context for later agents. Keep this document and the Unreal source together when transferring the scoped patch. Verify exact file hashes on the Windows side and preserve unrelated changes.

Use the authenticated TLS bridge. Keep its existing certificate verification and token storage intact. Azure guest-agent/run-command and public plaintext WebSocket transport are not fallback paths. Remote agents inherit the same opt-in rules for tests, builds, compilation, project generation, installs and releases. Source transfer does not establish an account connection or a playable game.

## Documentation consulted

Epic's module and commandlet references informed the source structure. An actual engine association and successful compilation remain outstanding.

```text
https://dev.epicgames.com/documentation/en-us/unreal-engine/unreal-engine-modules
https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/UCommandlet
https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Json/FJsonObject
```

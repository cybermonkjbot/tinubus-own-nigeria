# Cribtest11 Python and Unreal MCP

October 10, 2026. Editor control setup, separate from gameplay acceptance.

## Reference and transport

Joshua supplied Epic's video, `A First Look at Unreal MCP in Unreal Engine 5.8`,
YouTube ID `FON_noub0C0`. We retrieved its description and followed the official
`unreal-mcp-in-unreal-editor` documentation linked there. The caption endpoint
returned no text; this record does not claim a full video review.

The installed UE 5.8.3 source confirms the native `ModelContextProtocol`,
`AllToolsets`, `ToolsetRegistry` and `PythonScriptPlugin` plugins. The project
enables MCP, all toolsets and Python for Editor targets only. No third-party MCP
server or alternate scene-control implementation is installed.

| Connection | Configuration |
| --- | --- |
| Windows editor | `C:\dev\cribtest-game\apps\companion-unreal\BaddieCompanion.uproject` |
| Engine | `C:\Program Files\Epic Games\UE_5.8\Engine`, version 5.8.3, CL 58210709 |
| Native MCP | `http://127.0.0.1:8000/mcp` on Windows |
| Mac MCP tunnel | `http://127.0.0.1:18000/mcp`, forwarded through authenticated SSH |
| Codex entry | `unreal-cribtest` on both hosts, using each host's loopback URL |
| Mac tunnel service | `~/Library/LaunchAgents/co.uk.baddie.cribtest-unreal-mcp.plist` |
| Native Python | Epic's `remote_execution.py`, Windows loopback discovery with TTL 0 |

No Azure ingress rule was added. The existing SSH key and host verification
protect transport to the VM. Native MCP itself has no authentication; do not
change its bind address to a public interface. No API keys or account tokens are
needed in either editor bridge. The separate Codex agent bridge is unchanged.

## Operation

Use `apps/companion-unreal/Scripts/Start-EditorBridge.ps1` from the active Windows
desktop session or its on-demand Windows scheduled task. Pass `-EngineRoot`
with the engine directory above. The script requires existing project binaries
and never starts a build. It enables loopback Python and native MCP for that
editor process, disables automatic pip installation, and records the PID and log
under the project's `Saved\Bridge` directory.

The normal launch requests Direct3D 11 software rendering, with a 15 FPS cap.
The installed `WindowsD3D.cpp` confirms `AllowSoftwareRendering`. `-Headless`
selects NullRHI explicitly; a headless connection cannot establish visual or
gameplay acceptance. The launcher refuses to start a second editor for the same
project. It does not stop or replace an existing editor.

For native MCP discovery from the Mac repository:

```sh
python3 apps/companion-unreal/Scripts/mcp_client.py
python3 apps/companion-unreal/Scripts/mcp_client.py --method tools/call \
  --params '{"name":"list_toolsets","arguments":{}}'
```

`mcp_client.py` is a serial HTTP client, not a replacement MCP server. Discover
`tools/list`, then the native `list_toolsets` and `describe_toolset` schemas before
calling an engine tool. `call_tool` takes the unqualified method name even though
`describe_toolset` returns prefixed names. For example, this read-only call worked:

```sh
python3 apps/companion-unreal/Scripts/mcp_client.py --method tools/call \
  --params '{"name":"call_tool","arguments":{"toolset_name":"editor_toolset.toolsets.scene.SceneTools","tool_name":"get_folders","arguments":{}}}'
```

Requests are not retried. Do not overlap tool calls or use the automation-test
tools without Joshua's explicit authorization. `get_actors_in_folder` requires an
existing folder; an empty string is not accepted as the scene root.

For unrestricted engine Python, invoke `Scripts/python_bridge.py` with Unreal's
embedded Windows Python, `--engine` and `--project-dir`. Send one JSON request on
stdin through SSH. `{}` returns engine version, project directory, PID and the
discovered node. A subsequent `{"expected_pid":1234,"code":"..."}` request
requires the inspected PID. The client rechecks the real project path and PID
inside the editor before executing the supplied code. Do not assume the
foreground editor is the intended project.

The Mac LaunchAgent keeps only the SSH forwarding process alive. It does not
start Unreal, run builds or issue engine tool calls. Windows Codex connects
directly to the Windows loopback endpoint. Newly registered tools may require
reconnecting MCP or opening a fresh Codex chat.

## Compilation and evidence

The direct user authorized project compilation and editor launch for this setup.
The two targets now pin `BuildSettingsVersion.V7` and the Unreal 5.8 include order.
The installed build rejected V5 because it changed settings shared with the
prebuilt editor. No forced build-environment override was used.

An initial detached SSH build worker ended before compiler output. The retry uses
an on-demand Windows scheduled task under `cribadmin`, not the Azure guest agent.
Operation records and build logs are under
`C:\dev\cribtest-game\.git\operations\unreal-python-20261010`; local receipts are
under `output/cribtest11-python-bridge-2026-10-10`.

Build attempt 5 completed successfully at `2026-10-10T14:35:28.364Z`, exit code 0.
Both the runtime and editor module DLLs linked. The compiled Windows checkpoint
is `206a331daf693eff0965cf2a4c539e54393b0c3b`. The subsequent launcher-only fix is
`df7476d46a0d787a377c25d750bb06fc4dbbf22b`, the Windows HEAD at verification.
That fix resolves the default project path inside the PowerShell script body.

The compatibility changes also correct an include path, put the matching header
first, adapt JSON key comparison to UE 5.8, avoid enum/member name collisions,
and use `TPimplPtr` for the session's incomplete private type. They do not change
connection scope, admission rules or UI behavior.

The on-demand task `Baddie-Unreal-Editor-desktop` started editor PID `11084` in
the active Windows desktop session at `2026-10-10T14:40:25.133Z`. It uses D3D11
with the Microsoft Basic Render Driver, not NullRHI. Its launch receipt and log
are `Saved\Bridge\launch-20261010-074025.json` and
`Saved\Bridge\editor-20261010-074025.log`. The filenames use Windows local time;
the receipt also records UTC. The task invokes the operation directory's
`editor-worker-v2.cjs`, which records launcher output and propagates its status.

Live connection evidence:

- Native Python returned PID `11084`, project
  `C:/dev/cribtest-game/apps/companion-unreal/` and engine
  `5.8.3-58210709+++UE5+Release-5.8`. Embedded Python is 3.11.8.
- Windows listener inspection matched `127.0.0.1:8000` to PID `11084`. The Mac
  LaunchAgent was loaded and the loopback SSH tunnel carried MCP requests.
- MCP initialization negotiated `2025-11-25`; `tools/list` and `list_toolsets`
  succeeded, with 52 native toolsets discovered. This installed server returns
  empty `serverInfo` name, title and version fields.
- Native `SceneTools.get_folders` returned `HLOD`, `HLOD/HLOD0_Instancing` and
  `Lighting`. `get_actors_in_folder` for `Lighting` returned six actor references
  in `/Temp/Untitled_1.Untitled_1`, including the directional light, sky atmosphere,
  skylight, fog and volumetric cloud. These calls did not modify the scene.

Local evidence includes `status-latest.json`, `binaries.json`,
`python-identity.json`, `mcp-scene-folders.json` and `mcp-scene-lighting.json` in
the operation output directory above. The two rejected exploratory calls are
retained separately: a prefixed tool name and an empty folder path. No tests,
lint, additional builds or UI automation were run for connection verification.

## Remaining warnings and scope

The editor reports VC++ redistributable 14.44.35211.0 as outdated, a missing
project `Content` directory, and a missing `GameFeatureData` asset-manager rule
from the GameFeatures dependency enabled by `AllToolsets`. These warnings did
not prevent editor startup or the verified calls. They remain unresolved; this
record does not establish that every discovered tool works.

This setup does not enable live account admission, import assets, author a map,
package the game, publish a release or establish in-game visual acceptance.
Unrelated furniture and squad work remains with its existing owner and is not
included in this setup's source transfers. The open world is an unsaved engine
template, not the Baddie map. Compilation acceptance applies to the Windows
checkpoint above, not queued furniture, rural, squad or audio-compass changes.

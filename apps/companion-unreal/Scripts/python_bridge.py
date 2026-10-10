"""Use Epic's native Python bridge on the Windows editor host.

Read one JSON request from stdin. An empty object inspects the editor. Running
code requires the PID returned by that inspection. Do not overlap requests.
"""

import argparse
import json
import os
from pathlib import Path
import sys
import time


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--engine", required=True)
    parser.add_argument("--project-dir", required=True)
    args = parser.parse_args()
    request = json.load(sys.stdin)
    code = request.get("code")
    expected_pid = request.get("expected_pid")
    if code is not None and (not isinstance(code, str) or type(expected_pid) is not int):
        raise ValueError("Executing code requires code text and an inspected expected_pid")

    plugin_python = Path(args.engine) / "Plugins/Experimental/PythonScriptPlugin/Content/Python"
    sys.path.insert(0, str(plugin_python))
    import remote_execution

    config = remote_execution.RemoteExecutionConfig()
    config.multicast_bind_address = "127.0.0.1"
    config.multicast_ttl = 0
    bridge = remote_execution.RemoteExecution(config)
    normalize = lambda value: os.path.normcase(os.path.realpath(value))
    project = normalize(args.project_dir)
    bridge.start()
    try:
        # Collect a complete discovery window instead of attaching to the first pong.
        time.sleep(3)
        nodes = [node for node in bridge.remote_nodes
                 if normalize(node.get("project_root", "")) == project]
        if len(nodes) != 1:
            raise RuntimeError(f"Expected one editor for {project}; found {len(nodes)}: {bridge.remote_nodes}")
        bridge.open_command_connection(nodes[0]["node_id"])
        identity_code = (
            "import json, os, unreal\n"
            f"assert os.path.normcase(os.path.realpath(unreal.Paths.project_dir())) == {project!r}, 'Wrong project'\n"
            "print(json.dumps({'pid': os.getpid(), 'project_dir': unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()), "
            "'engine_version': unreal.SystemLibrary.get_engine_version()}))\n"
        )
        result = bridge.run_command(identity_code, unattended=True,
                                    exec_mode=remote_execution.MODE_EXEC_FILE)
        if not result.get("success"):
            raise RuntimeError(f"Identity inspection failed: {result}")
        identity = None
        for entry in result.get("output", []):
            try:
                value = json.loads(entry.get("output", ""))
                if isinstance(value, dict) and "pid" in value:
                    identity = value
            except (ValueError, TypeError):
                continue
        if identity is None:
            raise RuntimeError(f"No identity in response: {result}")
        if code is None:
            print(json.dumps({"identity": identity, "node": nodes[0]}))
            return
        if identity["pid"] != expected_pid:
            raise RuntimeError(f"Editor changed: expected PID {expected_pid}, found {identity['pid']}")
        guarded_code = (
            "import os, unreal\n"
            f"assert os.getpid() == {expected_pid}, 'Editor PID changed'\n"
            f"assert os.path.normcase(os.path.realpath(unreal.Paths.project_dir())) == {project!r}, 'Project changed'\n"
            f"exec(compile({code!r}, '<baddie-remote-script>', 'exec'))\n"
        )
        result = bridge.run_command(guarded_code, unattended=True,
                                    exec_mode=remote_execution.MODE_EXEC_FILE)
        print(json.dumps({"identity": identity, "result": result}))
        if not result.get("success"):
            sys.exit(1)
    finally:
        bridge.stop()


if __name__ == "__main__":
    main()

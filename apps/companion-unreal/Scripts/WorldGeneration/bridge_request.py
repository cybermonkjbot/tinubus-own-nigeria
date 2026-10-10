"""Construct requests for the existing native Python client; never send them."""

from pathlib import Path
import re

from .canonical import REPO, SCRIPT_DIR, file_hash


def request(expected_pid,authorization_path,authorization_sha256,jobs):
    if type(expected_pid) is not int or expected_pid<=0 or not jobs:
        raise ValueError("An inspected editor PID and explicit job batch are required")
    if not re.fullmatch(r"[a-f0-9]{64}",authorization_sha256 or ""):
        raise ValueError("Expected independently approved authorization hash")
    path=Path(authorization_path).resolve()
    if file_hash(path)!=authorization_sha256:
        raise ValueError("Authorization file hash changed")
    code=("import sys\n"
          "_baddie_generator_bytecode_setting = sys.dont_write_bytecode\n"
          "sys.dont_write_bytecode = True\n"
          f"sys.path.insert(0, {str(SCRIPT_DIR.parent)!r})\n"
          "try:\n"
          "    from WorldGeneration import native_generator\n"
          f"    native_generator.execute({str(path)!r}, {authorization_sha256!r}, {expected_pid!r}, {list(jobs)!r})\n"
          "finally:\n"
          "    sys.path.pop(0)\n"
          "    sys.dont_write_bytecode = _baddie_generator_bytecode_setting\n"
          "    del _baddie_generator_bytecode_setting\n")
    return {"expected_pid":expected_pid,"code":code}


def client_arguments():
    return {"client":str(REPO/"apps/companion-unreal/Scripts/python_bridge.py"),
            "engine":"C:/Program Files/Epic Games/UE_5.8/Engine",
            "projectDir":str(REPO/"apps/companion-unreal"),
            "stdinProtocol":"Inspect with {}; execute with PID-bound request after authorization",
            "mcpClient":str(REPO/"apps/companion-unreal/Scripts/mcp_client.py"),
            "mcpEndpoint":"http://127.0.0.1:8000/mcp",
            "mcpUse":"Inspect available schemas first; do not overlap Python and MCP mutations"}

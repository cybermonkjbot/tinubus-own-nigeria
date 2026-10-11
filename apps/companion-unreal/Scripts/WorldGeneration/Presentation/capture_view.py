"""One serial native MCP PNG capture using the existing loopback client.

Caller first uses the PID-bound Python bridge to prepare an exact native view.
No Slate drive, image synthesis, tests, comparison tasks or UI automation.
"""
import argparse
import base64
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import sys

sys.dont_write_bytecode = True
REPO = Path('C:/dev/cribtest-game')
OUT = REPO / 'output/cribtest11-visible-world-2026-10-11'
sys.path.insert(0, str(REPO / 'apps/companion-unreal/Scripts'))
from mcp_client import UnrealMcp


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--camera', required=True)
    parser.add_argument('--label', required=True)
    parser.add_argument('--expected-pid', type=int, default=13096)
    args = parser.parse_args()
    handoff = json.loads((OUT / 'theatre-presentation-v3.json').read_text(encoding='utf-8'))
    if args.camera == 'theatre-stop-landmark':
        camera = json.loads((OUT / 'theatre-stop-presentation-v1.json').read_text(encoding='utf-8'))['camera']
    else:
        camera = next(c for c in handoff['cameras'] if c['id'] == args.camera)
    name = args.label + '.png'
    png_path = (OUT / name).resolve()
    record_path = png_path.with_suffix('.capture.json')
    assert png_path.parent == OUT.resolve() and not png_path.exists() and not record_path.exists()
    # Recheck the local listener and project immediately before the native capture.
    inspection = subprocess.run(['powershell', '-NoProfile', '-NonInteractive', '-Command',
        "$pids=@(Get-NetTCPConnection -LocalAddress 127.0.0.1 -LocalPort 8000 -State Listen -ErrorAction Stop | Select-Object -ExpandProperty OwningProcess -Unique); "
        "if($pids.Count -ne 1){throw 'Expected one native MCP listener'}; "
        "Get-CimInstance Win32_Process -Filter ('ProcessId='+$pids[0]) | Select-Object ProcessId,ExecutablePath,CommandLine | ConvertTo-Json -Compress"],
        capture_output=True, text=True, check=True)
    identity = json.loads(inspection.stdout)
    assert identity['ProcessId'] == args.expected_pid
    assert identity['ExecutablePath'].endswith('UnrealEditor.exe')
    assert 'C:\\dev\\cribtest-game\\apps\\companion-unreal\\BaddieCompanion.uproject' in identity['CommandLine']
    client = UnrealMcp('http://127.0.0.1:8000/mcp')
    try:
        client.initialize()
        result = client.request('tools/call', {'name': 'call_tool', 'arguments': {
            'toolset_name': 'EditorToolset.EditorAppToolset', 'tool_name': 'CaptureViewport',
            'arguments': {'captureTransform': None, 'annotations': None, 'bShowUI': False}}})
        assert not result.get('isError'), str(result)
        text = next(c['text'] for c in result['content'] if c['type'] == 'text')
        capture = json.loads(text)['returnValue']
        assert capture['image']['mimeType'] == 'image/png'
        raw = base64.b64decode(capture['image']['data'], validate=True)
        assert raw[:8] == b'\x89PNG\r\n\x1a\n'
        width, height = struct.unpack('>II', raw[16:24])
        capture.pop('image')
        for axis, value in zip(('x', 'y', 'z'), camera['location']):
            assert abs(capture['cameraLocation'][axis] - value) < 0.01
        for axis in ('pitch', 'yaw', 'roll'):
            assert abs(capture['cameraRotation'][axis] - camera['rotation'][axis]) < 0.001
        assert abs(capture['cameraFOV'] - camera['fov']) < 0.01
        with png_path.open('xb') as stream:
            stream.write(raw)
        record = {'recordedAt': datetime.now(timezone.utc).isoformat(), 'pid': args.expected_pid,
            'camera': args.camera, 'imagePath': str(png_path.relative_to(REPO)),
            'imageSha256': hashlib.sha256(raw).hexdigest(), 'width': width, 'height': height,
            'nativeCapture': capture, 'softwareRendering': True, 'nullRhi': False,
            'parentVisualAcceptance': 'pending', 'scriptSha256': hashlib.sha256(Path(__file__).read_bytes()).hexdigest()}
        with record_path.open('x', encoding='utf-8', newline='\n') as stream:
            json.dump(record, stream, indent=2)
            stream.write('\n')
        print(json.dumps(record))
    finally:
        client.close()


if __name__ == '__main__':
    main()

"""One receipt-bound native render PNG, after real detailed geometry and redraw.

No UI drive, synthesized image, headless proxy, build or validation suite.
Call prepare_view through the PID bridge before this helper. Existing frames
and receipts are exclusive-create; failed visual evidence remains immutable.
"""
import argparse
import base64
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess
import sys
import time

sys.dont_write_bytecode = True
ROOT = Path('C:/dev/cribtest-game')
SCRIPTS = ROOT / 'apps/companion-unreal/Scripts'
OUT = ROOT / 'output/cribtest11-visible-world-2026-10-11/realism'
ENGINE = 'C:/Program Files/Epic Games/UE_5.8/Engine'
CONFIG = json.loads(Path(__file__).with_name('slice-config.json').read_text(encoding='utf-8'))
sys.path.insert(0, str(SCRIPTS))
from mcp_client import UnrealMcp


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def probe(expected_pid):
    code = """import json,unreal
def art_capture_probe():
 levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
 key=levels.get_active_viewport_config_key()
 location,rotation=levels.get_level_viewport_camera_info(key)
 print(json.dumps({'world':unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world().get_path_name(),
  'frame':unreal.SystemLibrary.get_frame_count(),'location':[location.x,location.y,location.z],
  'rotation':{'pitch':rotation.pitch,'yaw':rotation.yaw,'roll':rotation.roll},'fov':levels.get_level_viewport_fov(key)}))
art_capture_probe()
"""
    request = json.dumps({'expected_pid': expected_pid, 'code': code})
    process = subprocess.run([sys.executable, '-B', str(SCRIPTS / 'python_bridge.py'),
        '--engine', ENGINE, '--project-dir', str(ROOT / 'apps/companion-unreal')],
        input=request, capture_output=True, text=True, check=True)
    result = json.loads(process.stdout)['result']
    assert result['success'], result
    return next(json.loads(entry['output']) for entry in result['output'] if entry['type'] == 'Info' and entry['output'].startswith('{'))


def same_pose(actual, camera):
    location = actual.get('location', [actual['cameraLocation'][a] for a in ('x', 'y', 'z')] if 'cameraLocation' in actual else None)
    rotation = actual.get('rotation', actual.get('cameraRotation'))
    fov = actual.get('fov', actual.get('cameraFOV'))
    assert all(abs(v - m * 100) < 0.05 for v, m in zip(location, camera['locationMetres']))
    for axis in ('pitch', 'yaw', 'roll'):
        delta = (rotation[axis] - camera['rotation'][axis] + 180) % 360 - 180
        assert abs(delta) < 0.001, (rotation, camera['rotation'])
    assert abs(fov - camera['fov']) < 0.01


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--expected-pid', required=True, type=int)
    parser.add_argument('--receipt', required=True)
    parser.add_argument('--camera', required=True)
    parser.add_argument('--label', required=True)
    args = parser.parse_args()
    assert re.fullmatch('[A-Za-z0-9_-]+', args.label)
    receipt = Path(args.receipt).resolve()
    assert receipt.parent == OUT.resolve()
    delivery = json.loads(receipt.read_text(encoding='utf-8'))
    assert delivery['meshes'] and delivery['placements'], 'No detailed source scene saved'
    assert delivery['mapPackage'].startswith(CONFIG['namespace'] + '/')
    for path, digest in delivery['files'].items():
        target = (ROOT / path).resolve()
        assert target.is_relative_to((ROOT / 'apps/companion-unreal/Content/BaddieGeneratedWorld').resolve())
        assert sha(target) == digest
    camera = next(c for c in delivery.get('cameras', CONFIG['cameras']) if c['id'] == args.camera)
    image_path = OUT / (args.label + '.png')
    evidence_path = OUT / (args.label + '.capture.json')
    assert not image_path.exists() and not evidence_path.exists()
    # Read-only native frame probes yield back to the editor between requests.
    first = probe(args.expected_pid)
    assert first['world'].split('.')[0] == delivery['mapPackage']
    same_pose(first, camera)
    deadline = time.monotonic() + 45
    while True:
        latest = probe(args.expected_pid)
        assert latest['world'] == first['world']
        same_pose(latest, camera)
        if latest['frame'] - first['frame'] >= 8:
            break
        assert time.monotonic() < deadline, 'Editor redraw did not advance; do not claim a current image'
        time.sleep(0.25)
    inspection = subprocess.run(['powershell', '-NoProfile', '-NonInteractive', '-Command',
        "$listeners=@(Get-NetTCPConnection -LocalAddress 127.0.0.1 -LocalPort 8000 -State Listen -ErrorAction Stop | Select-Object -ExpandProperty OwningProcess -Unique); "
        "if($listeners.Count -ne 1){throw 'Expected one native MCP listener'}; "
        "Get-CimInstance Win32_Process -Filter ('ProcessId='+$listeners[0]) | Select-Object ProcessId,ExecutablePath,CommandLine | ConvertTo-Json -Compress"],
        capture_output=True, text=True, check=True)
    identity = json.loads(inspection.stdout)
    assert identity['ProcessId'] == args.expected_pid and identity['ExecutablePath'].endswith('UnrealEditor.exe')
    assert 'C:\\dev\\cribtest-game\\apps\\companion-unreal\\BaddieCompanion.uproject' in identity['CommandLine']
    assert '-AllowSoftwareRendering' in identity['CommandLine'] and '-d3d11' in identity['CommandLine']
    assert '-NullRHI' not in identity['CommandLine']
    client = UnrealMcp('http://127.0.0.1:8000/mcp')
    try:
        client.initialize()
        result = client.request('tools/call', {'name': 'call_tool', 'arguments': {
            'toolset_name': 'EditorToolset.EditorAppToolset', 'tool_name': 'CaptureViewport',
            'arguments': {'captureTransform': None, 'annotations': None, 'bShowUI': False}}})
        assert not result.get('isError'), result
        text = next(c['text'] for c in result['content'] if c['type'] == 'text')
        capture = json.loads(text)['returnValue']
        assert capture['image']['mimeType'] == 'image/png'
        same_pose(capture, camera)
        raw = base64.b64decode(capture.pop('image')['data'], validate=True)
        assert raw[:8] == b'\x89PNG\r\n\x1a\n'
        width, height = struct.unpack('>II', raw[16:24])
        with image_path.open('xb') as stream:
            stream.write(raw)
        evidence = {'schemaVersion': 1, 'recordedAt': datetime.now(timezone.utc).isoformat(), 'pid': args.expected_pid,
            'receipt': receipt.relative_to(ROOT).as_posix(), 'receiptSha256': sha(receipt),
            'mapPackage': delivery['mapPackage'], 'camera': args.camera, 'cameraConfig': camera,
            'image': image_path.relative_to(ROOT).as_posix(), 'imageSha256': hashlib.sha256(raw).hexdigest(),
            'width': width, 'height': height, 'warmupNativeFrames': latest['frame'] - first['frame'],
            'nativeCapture': capture, 'softwareRendering': True, 'nullRHI': False,
            'parentVisualAcceptance': 'pending', 'scriptSha256': sha(__file__)}
        with evidence_path.open('x', encoding='utf-8', newline='\n') as stream:
            json.dump(evidence, stream, indent=2)
            stream.write('\n')
        print(json.dumps(evidence))
    finally:
        client.close()


if __name__ == '__main__':
    main()

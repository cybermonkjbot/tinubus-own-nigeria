"""Byte/pin checks for this migration only; no engine, build or test execution."""
import hashlib
import json
from pathlib import Path
import re
import subprocess
from datetime import datetime, timezone

original = Path(r"C:\dev\cribtest-game")
target = Path(r"C:\dev\tinubus-own-nigeria")
pin = "153d5f6c87dbe4ea3b8346ad2c5fb221c1db9bd6"

def git(root, *args):
    return subprocess.check_output(["git", "-C", str(root), *args])

def sha(filename):
    value = hashlib.sha256()
    with filename.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            value.update(chunk)
    return value.hexdigest()

def original_unchanged():
    if git(original, "rev-parse", "HEAD").decode().strip() != pin or git(original, "status", "--porcelain"):
        raise RuntimeError("Original Windows checkout changed; reconcile before publication")

original_unchanged()
files = [p.decode() for p in git(original, "ls-files", "-z").split(b"\0") if p]
if len(files) != 6437 or len(set(files)) != 6437:
    raise RuntimeError("Unexpected original path set")
source = "apps/companion-unreal/Source"
if git(target, "rev-parse", "HEAD:" + source) != git(target, "rev-parse", "refs/remotes/origin/migration/mac-source:" + source):
    raise RuntimeError("Native source is not exactly the selected Mac source")
config = "apps/companion-unreal/Config/DefaultEngine.ini"
if git(target, "rev-parse", "HEAD:" + config) != git(target, "rev-parse", "refs/remotes/origin/migration/mac-source:" + config):
    raise RuntimeError("Unexpected main engine configuration")
entries = []
different = []
unexpected = []
for p in files:
    old, new = original / p, target / p
    a, b = sha(old), sha(new)
    if a != b:
        different.append(p)
        approved_agent_prefix = p == "AGENTS.md" and new.read_bytes() == (target / ".git/game-agent-prefix.md").read_bytes() + old.read_bytes()
        if not p.startswith(source + "/") and p != config and not approved_agent_prefix:
            unexpected.append(p)
    entries.append({"path": p, "beforeSha256": a, "afterSha256": b,
        "beforeBytes": old.stat().st_size, "afterBytes": new.stat().st_size})
if unexpected:
    print(json.dumps({"unexpectedByteDifferences": unexpected}))
    raise RuntimeError("Unexpected migration byte differences; no acceptance receipt written")
if len(different) != 39:
    raise RuntimeError("Unexpected selected-source difference count")

# Check the known generated file-server value without placing it in arguments or logs.
old_config = (original / config).read_bytes()
match = re.search(rb"(?m)^SecurityToken=([^\r\n]+)", old_config)
if not match:
    raise RuntimeError("Expected original token field missing; reconcile redaction evidence")
secret = match.group(1)
objects = git(target, "rev-list", "--objects", "main").splitlines()
checked_blobs = 0
reader = subprocess.Popen(["git", "-C", str(target), "cat-file", "--batch"],
    stdin=subprocess.PIPE, stdout=subprocess.PIPE)
try:
    for row in objects:
        oid = row.split(b" ", 1)[0]
        reader.stdin.write(oid + b"\n")
        reader.stdin.flush()
        fields = reader.stdout.readline().split()
        if len(fields) != 3:
            raise RuntimeError("Invalid Git object readback")
        length = int(fields[2])
        if length > 128 * 1024 * 1024:
            raise RuntimeError("Unconverted oversized Git object")
        value = reader.stdout.read(length)
        if len(value) != length or reader.stdout.read(1) != b"\n":
            raise RuntimeError("Truncated Git object readback")
        if fields[1] == b"blob":
            checked_blobs += 1
            if secret in value:
                raise RuntimeError("Known token remains in reachable migration history")
    reader.stdin.close()
    if reader.wait() != 0:
        raise RuntimeError("Git object reader failed")
except BaseException:
    reader.terminate()
    raise
original_unchanged()
receipt = {"utc": datetime.now(timezone.utc).isoformat(), "originalHead": pin,
    "migrationHead": git(target, "rev-parse", "HEAD").decode().strip(),
    "originalFiles": len(files), "identicalFiles": len(files) - len(different),
    "intentionalDifferences": different, "allOutputBytesUnchanged": True,
    "sourceMatchesMacTree": True, "knownTokenAbsentFromReachableBlobs": checked_blobs,
    "originalCleanBeforeAndAfter": True, "files": entries}
receipt_path = target / "docs/migration/2026-10-15/byte-preservation.json"
receipt_path.parent.mkdir(parents=True, exist_ok=True)
receipt_path.write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8")
print(json.dumps({k: v for k, v in receipt.items() if k not in {"files", "intentionalDifferences"}}))

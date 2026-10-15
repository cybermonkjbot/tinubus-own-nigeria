"""Materialize local LFS objects and retain original CRLF bytes in the new clone."""
import hashlib
from pathlib import Path
import shutil
import subprocess
import json

old = Path(r"C:\dev\cribtest-game")
new = Path(r"C:\dev\tinubus-own-nigeria")

def git(root, *args):
    return subprocess.check_output(["git", "-C", str(root), *args])

def sha(p):
    h = hashlib.sha256()
    with p.open("rb") as f:
        for data in iter(lambda: f.read(1024 * 1024), b""):
            h.update(data)
    return h.digest()

if git(new, "rev-parse", "HEAD").decode().strip() != "203ca88d92317ad4cbb03011a4acbf567265b416":
    raise RuntimeError("Wrong migration checkpoint")
if git(new, "status", "--porcelain") or git(old, "status", "--porcelain"):
    raise RuntimeError("Unexpected local changes before byte restoration")
if git(old, "rev-parse", "HEAD").decode().strip() != "153d5f6c87dbe4ea3b8346ad2c5fb221c1db9bd6":
    raise RuntimeError("Original checkpoint advanced")
subprocess.run(["git", "-C", str(new), "lfs", "checkout"], check=True)
shutil.copyfile(new / ".git/game-gitattributes", new / ".gitattributes")
paths = [p.decode() for p in git(old, "ls-files", "-z").split(b"\0") if p]
if len(paths) != 6437:
    raise RuntimeError("Unexpected original path count")
restore = []
for p in paths:
    if p.startswith("apps/companion-unreal/Source/") or p == "apps/companion-unreal/Config/DefaultEngine.ini":
        continue
    a, b = old / p, new / p
    if sha(a) == sha(b):
        continue
    if a.stat().st_size > 128 * 1024 * 1024 or b.stat().st_size > 128 * 1024 * 1024:
        raise RuntimeError("Unexpected large-asset difference: " + p)
    if a.read_bytes().replace(b"\r\n", b"\n") != b.read_bytes():
        raise RuntimeError("Difference is not Git CRLF normalization: " + p)
    restore.append(p)
for p in restore:
    shutil.copyfile(old / p, new / p)
    if sha(old / p) != sha(new / p):
        raise RuntimeError("Recorded-byte copy mismatch: " + p)
print(json.dumps({"restoredCRLFPaths": restore, "count": len(restore),
    "binaryObjectsMaterializedFromLocalLFS": True, "originalFilesModified": False}))

"""Sanitize the copied main branch only; leave the original Windows repo untouched."""
import json
from pathlib import Path
import re
import subprocess

ROOT = Path(r"C:\dev\tinubus-own-nigeria")
ORIGINAL = Path(r"C:\dev\cribtest-game")
EXPECTED = "153d5f6c87dbe4ea3b8346ad2c5fb221c1db9bd6"

def git(*arguments, cwd=ROOT):
    return subprocess.check_output(["git", "-C", str(cwd), *arguments]).decode().strip()

if Path.cwd().resolve() != ROOT.resolve() or git("branch", "--show-current") != "main":
    raise RuntimeError("Unexpected migration checkout or branch")
if git("rev-parse", "HEAD", cwd=ORIGINAL) != EXPECTED:
    raise RuntimeError("Original Windows checkout advanced")
if git("status", "--porcelain"):
    raise RuntimeError("Migration checkout must be clean before rewriting")

before = git("rev-parse", "HEAD")
export = subprocess.Popen(["git", "fast-export", "--signed-tags=strip",
    "--signed-commits=strip", "--export-marks=.git/pre-redaction-marks", "main"],
    stdout=subprocess.PIPE)
importer = subprocess.Popen(["git", "fast-import", "--force", "--quiet",
    "--export-marks=.git/post-redaction-marks"], stdin=subprocess.PIPE)
kind = None
changed = []
try:
    while True:
        line = export.stdout.readline()
        if not line:
            break
        if line == b"blob\n":
            kind = "blob"
        elif line.startswith(b"commit ") or line.startswith(b"tag "):
            kind = "metadata"
        if not line.startswith(b"data "):
            importer.stdin.write(line)
            continue
        length = int(line[5:].strip())
        if length > 256 * 1024 * 1024:
            raise RuntimeError("Convert large assets to LFS before redaction")
        value = export.stdout.read(length)
        if len(value) != length:
            raise RuntimeError("Truncated export payload")
        if kind == "blob":
            value, count = re.subn(rb"(?m)^SecurityToken=[^\r\n]+", b"SecurityToken=", value)
            if count:
                changed.append(count)
        importer.stdin.write(b"data " + str(len(value)).encode() + b"\n")
        importer.stdin.write(value)
    importer.stdin.close()
    if export.wait() != 0 or importer.wait() != 0:
        raise RuntimeError("Git history transfer failed")
except BaseException:
    export.terminate()
    importer.terminate()
    raise

if not changed:
    raise RuntimeError("Expected token fields were not found; inspect before publishing")
# This new checkout is disposable migration state, never the live source checkout.
subprocess.run(["git", "restore", "--source=HEAD", "--staged", "--worktree",
    "--", "apps/companion-unreal/Config/DefaultEngine.ini"], check=True)
receipt = {"before": before, "after": git("rev-parse", "HEAD"),
    "redactedBlobCount": len(changed), "redactedFields": sum(changed),
    "originalHeadUnchanged": git("rev-parse", "HEAD", cwd=ORIGINAL) == EXPECTED,
    "originalWorkingFilesModified": False, "credentialRotated": False}
(ROOT / ".git/redaction-receipt.json").write_text(json.dumps(receipt, indent=2) + "\n")
print(json.dumps(receipt))

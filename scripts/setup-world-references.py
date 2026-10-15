"""Restore the exact pinned reference cache. Does not run Unreal or a generator."""
import hashlib
import json
from pathlib import Path, PurePosixPath
import shutil

root = Path(__file__).resolve().parents[1]
source = root / "references/world-generation-20261010"
destination = root / ".git/operations/world-generation-20261010"
lock = json.loads((root / "apps/companion-unreal/Scripts/WorldGeneration/source-lock.json").read_text())
if not (root / ".git").is_dir() or (root / ".git").is_symlink():
    raise RuntimeError("Use an independent clone with its own .git directory")

def sha(filename):
    digest = hashlib.sha256()
    with filename.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()

records = lock["files"]
if lock["sourceCommit"] != "e978763b74acfb0369b65b879af6a3a8c90b110b" or len(records) != 250:
    raise RuntimeError("Unexpected source pin or reference count")
expected = {
    "reference-manifest.json": lock["referenceManifestSha256"],
    "reference-receipt.json": lock["referenceReceiptSha256"],
}
for record in records:
    relative = record["path"]
    parts = PurePosixPath(relative)
    if parts.is_absolute() or ".." in parts.parts or "\\" in relative or ":" in relative:
        raise RuntimeError("Invalid pinned reference path")
    key = "readonly-source/" + relative
    if key in expected:
        raise RuntimeError("Duplicate pinned reference path")
    expected[key] = record["sha256"]
    if (source / key).stat().st_size != record["bytes"]:
        raise RuntimeError("Reference length mismatch: " + relative)

# Validate every input and existing destination before copying any files.
for relative, digest in expected.items():
    incoming, target = source / relative, destination / relative
    if not incoming.resolve().is_relative_to(source.resolve()) or sha(incoming) != digest:
        raise RuntimeError("Reference hash or path mismatch: " + relative)
    if not target.resolve().is_relative_to((root / ".git").resolve()):
        raise RuntimeError("Reference destination escapes the clone")
    if target.exists() and (not target.is_file() or sha(target) != digest):
        raise RuntimeError("Existing reference differs; nothing was replaced: " + relative)

copied = 0
for relative, digest in expected.items():
    target = destination / relative
    if not target.exists():
        target.parent.mkdir(parents=True, exist_ok=True)
        # Exclusive creation also refuses a file that appeared after preflight.
        with (source / relative).open("rb") as incoming, target.open("xb") as output:
            shutil.copyfileobj(incoming, output)
        copied += 1
    if sha(target) != digest:
        raise RuntimeError("Copied reference differs: " + relative)
print(f"Verified 250 pinned sources and two unchanged receipts; copied {copied} cache files.")
print("Historical manifest/receipt reconciliation and all native-operation holds remain unchanged.")

"""Write original mesh source assets, not an app build/export or an editor mutation."""
import hashlib
import json
from pathlib import Path

from tree_drain_geometry import catalog


ROOT = Path(__file__).resolve().parents[4]
OUT = ROOT / "output/unreal-street-detail-20261014/tree-drain-kit"


def immutable(path, data):
    if path.exists():
        if path.read_bytes() != data:
            raise ValueError("Refusing to replace immutable asset " + str(path))
    else:
        with path.open("xb") as stream:
            stream.write(data)


def obj(mesh, name, material_file):
    # Installed native OBJ import reflects Y. Reverse winding under that reflection.
    lines = ["# Native cm; precompensated installed OBJ Y reflection", "mtllib " + material_file, "o " + name]
    lines.extend("v %.9f %.9f %.9f" % (p[0], -p[1], p[2]) for p in mesh["verticesCm"])
    for uv in mesh["triangleUVsMetres"]:
        lines.extend("vt %.9f %.9f" % (p[0], 1 - p[1]) for p in uv)
    current = None
    for index, (face, material) in enumerate(zip(mesh["triangles"], mesh["faceMaterialIds"])):
        if current != material:
            lines.append("usemtl TD_" + material)
            current = material
        indices = [(vertex + 1, index * 3 + corner + 1) for corner, vertex in enumerate(face)]
        lines.append("f " + " ".join("%d/%d" % value for value in reversed(indices)))
    return ("\n".join(lines) + "\n").encode("ascii")


def author():
    packet = catalog()
    packet["geometrySourceSha256"] = hashlib.sha256(Path(__file__).with_name("tree_drain_geometry.py").read_bytes()).hexdigest()
    source = (json.dumps(packet, sort_keys=True, separators=(",", ":"), allow_nan=False) + "\n").encode("ascii")
    digest = hashlib.sha256(source).hexdigest()
    folder = OUT / digest[:16]
    folder.mkdir(parents=True, exist_ok=True)
    material_lines = []
    for name, recipe in packet["materials"].items():
        material_lines.extend(["newmtl TD_" + name, "Kd " + " ".join(str(x) for x in recipe["color"]),
                               "Pr " + str(recipe["roughness"]), "Pm " + str(recipe["metallic"])])
    immutable(folder / "tree-drain.mtl", ("\n".join(material_lines) + "\n").encode("ascii"))
    records = []
    for asset in packet["assets"]:
        lods = []
        for level, mesh in enumerate(asset["lods"]):
            filename = asset["id"] + "_LOD" + str(level) + ".obj"
            data = obj(mesh, asset["id"], "tree-drain.mtl")
            immutable(folder / filename, data)
            lods.append({"file": filename, "triangles": len(mesh["triangles"]),
                         "boundsCm": mesh["boundsCm"], "sha256": hashlib.sha256(data).hexdigest()})
        records.append({k: v for k, v in asset.items() if k != "lods"} | {"lods": lods})
    immutable(folder / "geometry.json", source)
    manifest = {k: v for k, v in packet.items() if k != "assets"} | {"assets": records, "geometrySha256": digest}
    immutable(folder / "manifest.json", (json.dumps(manifest, sort_keys=True, indent=2) + "\n").encode("ascii"))
    return {"folder": str(folder), "geometrySha256": digest, "treeModels": 6, "drainModules": 6,
            "objMeshFiles": sum(len(a["lods"]) for a in records), "nativeImported": False, "placedInStreet": False}


if __name__ == "__main__":
    print(json.dumps(author()))

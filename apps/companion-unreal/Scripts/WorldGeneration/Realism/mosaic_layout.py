"""Pure placement planning for an additive neighborhood; no engine side effects."""
import copy
import math


RUNTIME_KEYS = ("actor", "component", "instanceIndex", "actualTransform", "batched")


def rotate(point, yaw):
    angle = math.radians(yaw)
    x, y, z = point
    return [x * math.cos(angle) - y * math.sin(angle),
            x * math.sin(angle) + y * math.cos(angle), z]


def placement(template, identity, location, yaw, role):
    row = copy.deepcopy(template)
    for key in RUNTIME_KEYS:
        row.pop(key, None)
    row.update(sourceId=identity, sourceIdBasis="Authored mosaic derivative; not an OSM or runtime identity",
               runtimeId=None, authoringRole=role,
               transform={"locationCm": location,
                          "rotation": {"pitch": 0, "yaw": yaw, "roll": 0}, "scale": [1, 1, 1]})
    return row


def plan(base, config):
    """Retain complete imported buildings and interfaces at their original scale."""
    prefix = config["idPrefix"]
    rows = []
    templates = {}
    for old in base["placements"]:
        templates.setdefault(old["module"], old)
        if old.get("authoringRole") != "architecture":
            row = copy.deepcopy(old)
            for key in RUNTIME_KEYS:
                row.pop(key, None)
            row["sourceId"] = prefix + ":street:" + old["sourceId"]
            row["runtimeId"] = None
            rows.append(row)

    runs = {run["id"]: run for run in base["buildingRuns"]}
    groups = {name: [] for name in runs}
    for row in base["placements"]:
        if row.get("authoringRole") != "architecture":
            continue
        for name in runs:
            if ":" + name + ":" in row["sourceId"]:
                groups[name].append(row)
                break
        else:
            raise ValueError("Architecture has no source run: " + row["sourceId"])

    def add(module, identity, location, yaw, role):
        rows.append(placement(templates[module], prefix + ":" + identity, location, yaw, role))

    assignments = []
    for target in config["buildings"]:
        original = runs[target["templateRun"]]
        source_rows = groups[original["id"]]
        if not source_rows:
            raise ValueError("Empty building template")
        for old in source_rows:
            stock = old["module"].startswith("V3_")
            if target["use"] == "residential" and (stock or old["module"] == "OpenShopInterior"):
                continue
            if stock and target["use"] in ("print", "workshop") and old["module"] not in ("V3_PaperPackage", "V3_TimberCrate"):
                continue
            local = rotate([a - b for a, b in zip(old["transform"]["locationCm"], original["originCm"])], -original["yaw"])
            offset = rotate(local, target["yaw"])
            location = [a + b for a, b in zip(target["originCm"], offset)]
            yaw = old["transform"]["rotation"]["yaw"] - original["yaw"] + target["yaw"]
            identity = prefix + ":" + target["id"] + ":" + old["sourceId"]
            rows.append(placement(old, identity, location, yaw, "architecture"))
        if target["use"] == "residential":
            # Close former commercial entries with the complete source shutter,
            # not a stretched facade panel. Household access remains pending.
            for bay in range(3):
                if bay in original["closedBays"]:
                    continue
                offset = rotate([0, -390 * bay - 55, 0], target["yaw"])
                add("Shutter2800", target["id"] + ":closed-ground-bay:" + str(bay),
                    [a + b for a, b in zip(target["originCm"], offset)], target["yaw"], "architecture")
        assignments.append(dict(target, sourcePlacementCount=len(source_rows),
                                assetRole="Imported complete two-storey module; no surveyed facade claim",
                                householdInteriorImplemented=False))

    # Existing side-road ends at X=2661cm. Extend by whole 10m sections,
    # with source pavement/channel/cover/kerb sections along both sides.
    for i in range(2, config["sideRoadSections"]):
        add("RoadStraight10m", "side-road:" + str(i), [-339 + 1000 * i, -6261, 0], 90, "side_road")
    for i in range(20, config["sideRoadSections"] * 10):
        for module, role, north, south in (
            ("Pavement1000", "pedestrian", -6304, -7396),
            ("Drain1000", "drain", -6536, -7164),
            ("DrainCover1000", "drain_cover", -6536, -7164),
            ("Kerb1000", "kerb", -6589, -7111),
        ):
            add(module, "side-north:" + module + ":" + str(i), [100 * i, north, -5 if module != "Kerb1000" else -20], 90, "side_" + role)
            add(module, "side-south:" + module + ":" + str(i), [100 * i + 100, south, -5 if module != "Kerb1000" else -20], -90, "side_" + role)

    # Whole 2m by 1m source slabs support the yard without scaling their UVs.
    yard = config["yard"]
    for x in range(yard["minCm"][0] + 200, yard["maxCm"][0] + 1, 200):
        for y in range(yard["minCm"][1] + 100, yard["maxCm"][1] + 1, 100):
            add("Pavement1000", "yard:" + str(x) + ":" + str(y), [x, y, -5], 0, "courtyard_surface")

    for index, row in enumerate(rows):
        row["placementIndex"] = index
    if len({row["sourceId"] for row in rows}) != len(rows):
        raise ValueError("Duplicate mosaic placement identity")
    if any(row["transform"]["scale"] != [1, 1, 1] for row in rows):
        raise ValueError("Mosaic must retain human-scale imported geometry")
    if any(not math.isfinite(v) for row in rows for v in row["transform"]["locationCm"]):
        raise ValueError("Non-finite placement")
    return rows, assignments

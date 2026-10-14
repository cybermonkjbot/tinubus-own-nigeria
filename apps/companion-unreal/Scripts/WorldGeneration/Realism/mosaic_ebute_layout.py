"""Research-pinned Ebute Metta slice planner; importing this file is inert."""
import copy
import math

from WorldGeneration.Realism.mosaic_layout import rotate


RUNTIME_KEYS = ("actor", "component", "instanceIndex", "actualTransform", "batched")
EARTH_RADIUS_METRES = 6371008.8


def project(lon, lat, origin):
    lon0, lat0 = origin
    return [EARTH_RADIUS_METRES * math.radians(lon - lon0) * math.cos(math.radians(lat0)),
            EARTH_RADIUS_METRES * math.radians(lat - lat0)]


def _point(lon, lat, origin):
    east, north = project(lon, lat, origin)
    return [east * 100, -north * 100]


def _offset_line(points, distance_cm):
    result = []
    for index, point in enumerate(points):
        incoming = None if index == 0 else [point[i] - points[index - 1][i] for i in range(2)]
        outgoing = None if index == len(points) - 1 else [points[index + 1][i] - point[i] for i in range(2)]

        def unit(vector):
            length = math.hypot(*vector)
            if not length:
                raise ValueError("Mapped street contains a repeated consecutive coordinate")
            return [component / length for component in vector]

        normals = []
        for vector in (incoming, outgoing):
            if vector is not None:
                dx, dy = unit(vector)
                normals.append([-dy, dx])
        if len(normals) == 1:
            delta = [component * distance_cm for component in normals[0]]
        else:
            summed = [normals[0][i] + normals[1][i] for i in range(2)]
            length = math.hypot(*summed)
            if length < 1e-8:
                raise ValueError("Cannot offset a reversing street vertex")
            miter = [component / length for component in summed]
            denominator = sum(miter[i] * normals[1][i] for i in range(2))
            if abs(denominator) < 0.25:
                raise ValueError("Street bend requires a purpose-built junction surface")
            delta = [component * distance_cm / denominator for component in miter]
        result.append([point[0] + delta[0], point[1] + delta[1]])
    return [[point[0], 0, point[1]] for point in result]


def _line_metrics(points):
    distances, length = [0.0], 0.0
    for a, b in zip(points, points[1:]):
        length += math.dist(a, b) / 100
        distances.append(length)
    return distances, length


def _at_chainage(points, distances, chainage):
    if chainage < 0 or chainage > distances[-1]:
        raise ValueError("Frontage chainage is outside its mapped street segment")
    index = next((i for i in range(len(distances) - 1)
                  if distances[i] <= chainage <= distances[i + 1]), len(distances) - 2)
    start, end = points[index], points[index + 1]
    span = distances[index + 1] - distances[index]
    fraction = 0 if span == 0 else (chainage - distances[index]) / span
    location = [a + (b - a) * fraction for a, b in zip(start, end)]
    dx, dy = end[0] - start[0], end[1] - start[1]
    length = math.hypot(dx, dy)
    if length == 0:
        raise ValueError("Mapped street contains a repeated consecutive coordinate")
    tangent = [dx / length, dy / length]
    yaw = math.degrees(math.atan2(dx, -dy))
    return location, tangent, yaw


def plan(base, config):
    """Project retained OSM vertices at 1:1 planimetric scale and place complete modules."""
    if config["coordinateContract"]["compression"] != 1.0:
        raise ValueError("This slice preserves local planimetric scale")
    if config["designValues"]["carriagewayWidthMetres"] <= 0 or config["designValues"]["clearFootwayWidthMetres"] <= 0:
        raise ValueError("Road and clear footway design widths must be positive")
    origin_node = next(node for street in config["streets"] for node in street["nodes"]
                       if node["id"] == "1783114075")
    origin = [origin_node["lon"], origin_node["lat"]]
    nodes, edges, streets, frontages = {}, [], [], []
    for street in config["streets"]:
        if len(street["nodes"]) < 2:
            raise ValueError("Street requires at least two retained OSM vertices")
        path = []
        for source in street["nodes"]:
            identity = "osm:" + source["id"]
            location = _point(source["lon"], source["lat"], origin)
            prior = nodes.get(identity)
            if prior and (prior["locationCm"] != [location[0], 0, location[1]]
                          or prior["wgs84LonLat"] != [source["lon"], source["lat"]]):
                raise ValueError("Shared OSM node has conflicting source coordinates")
            nodes.setdefault(identity, {"id": identity, "locationCm": [location[0], 0, location[1]],
                                        "wgs84LonLat": [source["lon"], source["lat"]],
                                        "sourceNodeId": source["id"], "runtimeId": None})
            path.append(location)
        distances, length = _line_metrics(path)
        for i, (a, b) in enumerate(zip(street["nodes"], street["nodes"][1:])):
            edges.append({"id": "edge:" + street["id"] + ":" + str(i),
                          "from": "osm:" + a["id"], "to": "osm:" + b["id"],
                          "modes": ["walk"], "lengthMetres": distances[i + 1] - distances[i],
                          "available": False,
                          "blockedReason": "Mapped residential way does not verify public access, footway or native collision",
                          "engineTraversalVerified": False, "publicAccessVerified": False,
                          "vehicleTraversalAuthored": False})
        streets.append({"id": street["id"], "name": street["name"],
                        "osmWayId": street["osmWayId"], "osmVersion": street["osmVersion"],
                        "sourceUrl": street["sourceUrl"], "sourceTags": street["sourceTags"],
                        "sourceNodeIds": [n["id"] for n in street["nodes"]],
                        "centerlineCm": [[p[0], 0, p[1]] for p in path],
                        "mappedLengthMetres": length, "authoringCompression": 1.0,
                        "verticalDatum": None, "roadWidthMeasured": False,
                        "footwayPresenceVerified": False,
                        "crossSectionProposal": {
                            "carriagewayWidthMetres": config["designValues"]["carriagewayWidthMetres"],
                            "clearFootwayWidthMetres": config["designValues"]["clearFootwayWidthMetres"],
                            "carriagewayEdgesCm": {
                                "right": _offset_line(path, config["designValues"]["carriagewayWidthMetres"] * 50),
                                "left": _offset_line(path, -config["designValues"]["carriagewayWidthMetres"] * 50)},
                            "footwayOuterEdgesCm": {
                                "right": _offset_line(path, (config["designValues"]["carriagewayWidthMetres"] / 2
                                                               + config["designValues"]["clearFootwayWidthMetres"]) * 100),
                                "left": _offset_line(path, -(config["designValues"]["carriagewayWidthMetres"] / 2
                                                                + config["designValues"]["clearFootwayWidthMetres"]) * 100)},
                            "miteredAtMappedVertices": True,
                            "junctionStitchMeshesCreated": False,
                            "collisionAndAccessibilityReviewed": False}})
        for frontage in street["frontages"]:
            location, tangent, yaw = _at_chainage(path, distances, frontage["chainageMetres"])
            frontages.append({**frontage, "streetId": street["id"],
                              "locationCm": [location[0], 0, location[1]],
                              "tangentCm": tangent, "yaw": yaw,
                              "useIsFictional": True, "publicEntryVerified": False})

    runs = {r["id"]: r for r in base["buildingRuns"]}
    architecture = {}
    module_templates = {}
    for row in base["placements"]:
        module_templates.setdefault(row.get("module"), row)
        if row.get("authoringRole") != "architecture":
            continue
        for run_id in runs:
            if ":" + run_id + ":" in row["sourceId"]:
                architecture.setdefault(run_id, []).append(row)
                break
        else:
            raise ValueError("Unassigned source frontage module: " + row["sourceId"])

    structure = next(mesh for mesh in base["meshes"] if mesh["module"] == "FrontageStructure")
    structure_centre_y = (structure["boundsCm"][0][1] + structure["boundsCm"][1][1]) / 2
    placements, buildings = [], []
    for frontage in frontages:
        run = runs.get(frontage["run"])
        if not run or frontage["run"] not in architecture:
            raise ValueError("Missing complete imported source run: " + frontage["run"])
        cardinal = {"east": [1, 0], "west": [-1, 0],
                    "north": [0, -1], "south": [0, 1]}[frontage["side"]]
        dx, dy = frontage["tangentCm"]
        right = [-dy, dx]
        side_sign = 1 if cardinal[0] * right[0] + cardinal[1] * right[1] >= 0 else -1
        expects_right = side_sign == 1
        if frontage["run"].startswith("right-") != expects_right:
            raise ValueError("Frontage template side disagrees with its mapped street orientation")
        target_yaw = frontage["yaw"] + (0 if expects_right else 180)
        normal = [right[0] * side_sign, right[1] * side_sign]
        centre = frontage["locationCm"]
        # Keep the source cross-section between each road centerline and facade.
        wall_distance = config["designValues"]["frontageSetbackFromMappedCentrelineMetres"] * 100
        target_origin_xy = [centre[0] + normal[0] * wall_distance,
                            centre[2] + normal[1] * wall_distance]
        target_origin_xy = [a - b for a, b in zip(target_origin_xy,
                                                  rotate([0, structure_centre_y, 0], target_yaw)[:2])]
        source_origin = run["originCm"]
        delta_yaw = target_yaw - run["yaw"]
        for old in architecture[frontage["run"]]:
            module = old.get("module", "")
            if frontage["fictionalUse"] == "household" and (module.startswith("V3_")
                                                               or module == "OpenShopInterior"):
                continue
            if module.startswith("V3_") and frontage["fictionalUse"] != "food-provisions":
                continue
            row = copy.deepcopy(old)
            for key in RUNTIME_KEYS:
                row.pop(key, None)
            local = [a - b for a, b in zip(old["transform"]["locationCm"], source_origin)]
            offset = rotate(local, delta_yaw)
            row["sourceId"] = (config["idPrefix"] + ":" + frontage["id"] + ":" + old["sourceId"])
            row["runtimeId"] = None
            row["authoringRole"] = "fictional_frontage_template"
            row["streetReference"] = frontage["streetId"]
            row["sourceStreetNodeIds"] = streets[[s["id"] for s in streets].index(frontage["streetId"])]["sourceNodeIds"]
            row["fictionalUse"] = frontage["fictionalUse"]
            row["transform"] = {"locationCm": [target_origin_xy[0] + offset[0],
                                                  offset[1], target_origin_xy[1] + offset[2]],
                                "rotation": {"pitch": 0, "yaw": old["transform"]["rotation"]["yaw"] + delta_yaw,
                                             "roll": 0}, "scale": [1, 1, 1]}
            placements.append(row)
        if frontage["fictionalUse"] == "household":
            shutter = module_templates.get("Shutter2800")
            if shutter is None:
                raise ValueError("Household fronts require the complete source shutter mesh")
            for bay in range(3):
                if bay in run["closedBays"]:
                    continue
                row = copy.deepcopy(shutter)
                for key in RUNTIME_KEYS:
                    row.pop(key, None)
                offset = rotate([0, -390 * bay - 55, 0], target_yaw)
                row["sourceId"] = config["idPrefix"] + ":" + frontage["id"] + ":closed-bay:" + str(bay)
                row["runtimeId"] = None
                row["authoringRole"] = "fictional_household_shutter"
                row["streetReference"] = frontage["streetId"]
                row["transform"] = {"locationCm": [target_origin_xy[0] + offset[0], offset[1],
                                                      target_origin_xy[1] + offset[2]],
                                    "rotation": {"pitch": 0, "yaw": target_yaw, "roll": 0},
                                    "scale": [1, 1, 1]}
                placements.append(row)
        buildings.append({"id": frontage["id"], "streetId": frontage["streetId"],
                          "sourceRun": frontage["run"], "fictionalUse": frontage["fictionalUse"],
                          "locationCm": centre, "runtimeId": None,
                          "householdInteriorImplemented": False, "publicEntryVerified": False})
    ids = [row["sourceId"] for row in placements]
    if len(ids) != len(set(ids)) or any(row["transform"]["scale"] != [1, 1, 1] for row in placements):
        raise ValueError("Placement identities must be unique and source geometry must retain scale")
    if any(not math.isfinite(value) for row in placements for value in row["transform"]["locationCm"]):
        raise ValueError("Non-finite frontage transform")
    return {"coordinateSpace": "local-native-centimetres", "authoringOnly": True,
            "runtimeAdmission": False, "verticalDatum": None, "streets": streets,
            "nodes": list(nodes.values()), "edges": edges, "buildings": buildings,
            "placements": placements,
            "designValues": copy.deepcopy(config["designValues"]),
            "gates": {"publicAccess": False, "movement": False, "vehicleTraffic": False,
                      "householdIdentity": False, "productionRegistration": False}}

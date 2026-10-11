"""Editor-only route planning; authored connectivity never admits gameplay."""
import heapq
import math

from WorldGeneration.Realism.mosaic_layout import rotate


def minute(value):
    if type(value) is not int or value < 0:
        raise ValueError("Provide a nonnegative integer game minute")
    return value


def build_navigation(config):
    nodes, edges, destinations = {}, [], []

    def node(identity, location, kind):
        if identity in nodes or any(not math.isfinite(v) for v in location):
            raise ValueError("Invalid navigation node")
        nodes[identity] = {"id": identity, "locationCm": location, "kind": kind}
        return identity

    def edge(identity, start, end, modes, available=True, reason=None, bidirectional=True):
        distance = math.dist(nodes[start]["locationCm"], nodes[end]["locationCm"]) / 100
        edges.append({"id": identity, "from": start, "to": end, "modes": modes,
                      "lengthMetres": distance, "available": available,
                      "blockedReason": reason, "bidirectional": bidirectional,
                      "engineTraversalVerified": False})

    for side, x in (("east", -140), ("west", -1040)):
        previous = None
        for index, y in enumerate((0, -1200, -2400, -3600, -4800, -6380, -7200, -8400, -9600, -10800, -12000)):
            current = node(side + ":" + str(index), [x, y, -5], "footway")
            if previous:
                edge(side + ":walk:" + str(index), previous, current, ["walk"])
            previous = current

    side_nodes = []
    for side, y in (("north", -6400), ("south", -7300)):
        previous = None
        for index, x in enumerate(range(0, config["sideRoadSections"] * 1000, 1000)):
            current = node(side + ":" + str(index), [x, y, -5], "footway")
            side_nodes.append(current)
            if previous:
                edge(side + ":walk:" + str(index), previous, current, ["walk"])
            previous = current
    edge("east-to-north", "east:5", "north:0", ["walk"])
    edge("east-to-south", "east:6", "south:0", ["walk"])
    edge("main-road-crossing", "east:5", "west:5", ["walk"], False,
         "Protected crossing and vehicle conflict handling are not implemented")
    side_crossing = min(3, config["sideRoadSections"] - 1)
    edge("side-road-crossing", "north:" + str(side_crossing), "south:" + str(side_crossing), ["walk"], False,
         "Protected crossing and vehicle conflict handling are not implemented")
    yard = config["yard"]
    yard_entry = node("yard:entry", [(yard["minCm"][0] + yard["maxCm"][0]) / 2, yard["minCm"][1], -5], "yard")
    yard_centre = node("yard:centre", [(yard["minCm"][0] + yard["maxCm"][0]) / 2,
                                     (yard["minCm"][1] + yard["maxCm"][1]) / 2, -5], "yard")
    edge("yard-local", yard_entry, yard_centre, ["walk"])
    nearest_yard = min((key for key in side_nodes if key.startswith("north:")),
                       key=lambda key: (math.dist(nodes[key]["locationCm"], nodes[yard_entry]["locationCm"]), key))
    edge("yard-interface", nearest_yard, yard_entry, ["walk"], False,
         "Covered drainage interface and step-free yard access require native review")

    for building in config["buildings"]:
        origin = building["originCm"]
        offset = rotate([-140, -585, -5], building["yaw"])
        location = [a + b for a, b in zip(origin, offset)]
        identity = "entry:" + building["id"]
        available = building["use"] != "residential"
        node(identity, location, "destination-entry")
        if building["id"].startswith("main-east"):
            candidates = [key for key in nodes if key.startswith("east:")]
        elif building["id"].startswith("main-west"):
            candidates = [key for key in nodes if key.startswith("west:")]
        elif building["id"].startswith("side-north"):
            candidates = [key for key in side_nodes if key.startswith("north:")]
        elif building["id"].startswith("side-south"):
            candidates = [key for key in side_nodes if key.startswith("south:")]
        else:
            candidates = [yard_entry]
        nearest = min(candidates, key=lambda key: (math.dist(location, nodes[key]["locationCm"]), key))
        edge("approach:" + building["id"], nearest, identity, ["walk"], available,
             None if available else "Usable residential door and household access are pending")
        destinations.append({"id": "destination:" + building["id"], "buildingId": building["id"],
                             "use": building["use"], "entryNodeId": identity,
                             "locationCm": location, "available": available,
                             "capacity": 3 if available else 0, "installedGenerator": False,
                             "interiorVerified": False, "engineInteractionImplemented": False})

    for lane, x, ys in (("southbound", -710, (-500, -5500, -11500)),
                        ("northbound", -470, (-11500, -5500, -500))):
        previous = None
        for index, y in enumerate(ys):
            current = node(lane + ":" + str(index), [x, y, -20], "vehicle-lane")
            if previous:
                edge(lane + ":drive:" + str(index), previous, current, ["car", "danfo"],
                     False, "Vehicle swept paths, collision and traffic authority are pending", False)
            previous = current
    return {"schemaVersion": 1, "coordinateSpace": "local-native-centimetres",
            "authoringOnly": True, "runtimeAdmission": False,
            "nodes": list(nodes.values()), "edges": edges, "destinations": destinations}


def route(graph, start, end, mode="walk", blocked_edges=()):
    """Shortest authored route; never use unavailable or caller-blocked edges."""
    nodes = {node["id"]: node for node in graph["nodes"]}
    if start not in nodes or end not in nodes:
        raise ValueError("Unknown route endpoint")
    blocked = set(blocked_edges)
    adjacency = {identity: [] for identity in nodes}
    for edge in graph["edges"]:
        if not edge["available"] or edge["id"] in blocked or mode not in edge["modes"]:
            continue
        adjacency[edge["from"]].append((edge["to"], edge["lengthMetres"], edge["id"]))
        if edge["bidirectional"]:
            adjacency[edge["to"]].append((edge["from"], edge["lengthMetres"], edge["id"]))
    distances, parents, pending = {start: 0}, {}, [(0, start)]
    while pending:
        distance, current = heapq.heappop(pending)
        if distance != distances[current]:
            continue
        if current == end:
            path, edge_ids = [end], []
            while path[-1] != start:
                previous, edge_id = parents[path[-1]]
                path.append(previous)
                edge_ids.append(edge_id)
            return {"status": "authored-route", "nodeIds": path[::-1], "edgeIds": edge_ids[::-1],
                    "lengthMetres": distance, "engineTraversalVerified": False}
        for neighbor, length, edge_id in sorted(adjacency[current]):
            candidate = distance + length
            if candidate < distances.get(neighbor, math.inf):
                distances[neighbor], parents[neighbor] = candidate, (current, edge_id)
                heapq.heappush(pending, (candidate, neighbor))
    return {"status": "unavailable", "nodeIds": [], "edgeIds": [], "lengthMetres": None,
            "engineTraversalVerified": False}


def build_events(destinations):
    candidates = sorted((d for d in destinations if d["available"] and d["use"] == "print"), key=lambda d: d["id"])
    if not candidates:
        return []
    return [{"id": "mosaic:binding-collection", "authoringOnly": True,
             "destinationId": candidates[0]["id"], "startMinute": 960, "endMinute": 975,
             "kind": "group-coursework-collection", "blockedEdgeIds": [],
             "additionalQueueRequests": 2, "capacity": candidates[0]["capacity"],
             "navigationDisruption": "Longer optional service wait; public circulation unchanged",
             "clearsWhen": "Authored collection window ends; no forced player assistance",
             "performanceAndInteractionImplemented": False}]


def sample_events(events, game_minute):
    day_minute = minute(game_minute) % 1440
    return [dict(event, dayIndex=game_minute // 1440, active=True)
            for event in events if event["startMinute"] <= day_minute < event["endMinute"]]

"""Clock-stepped authoring movement and logical queues, never runtime actors."""
from copy import deepcopy
import math

from WorldGeneration.Realism.mosaic_navigation import minute, route, sample_events
from WorldGeneration.Realism.mosaic_routines import sample_routines


def build_policy(destinations):
    return {
        "schemaVersion": 1, "authoringOnly": True, "runtimeAdmission": False,
        "gameSecondsPerMinute": 60, "walkingMetresPerGameSecond": 1.2,
        "clockBasis": "Fictional uncompressed game clock; not measured walking speed",
        "maximumStepMinutes": 1,
        "queuePositionBasis": "Logical FIFO rank only; no physical waiting slots authored",
        "queues": [{"destinationId": d["id"], "serviceCapacity": d["capacity"],
                    "waitingCapacity": d["capacity"], "serviceMinutes": 3}
                   for d in sorted(destinations, key=lambda row: row["id"])
                   if d["available"] and type(d["capacity"]) is int and d["capacity"] > 0],
    }


def initial_state(graph, routines, game_minute=0, start_node_ids=None):
    """Explicit caller positions only. Missing positions cannot enter a route."""
    clock = minute(game_minute)
    nodes = {n["id"] for n in graph["nodes"]}
    samples = sample_routines(routines, clock)
    starts = dict(start_node_ids or {})
    if set(starts) - {s["id"] for s in samples} or any(n not in nodes for n in starts.values()):
        raise ValueError("Initial positions require known routine and navigation node ids")
    return {"schemaVersion": 1, "authoringOnly": True, "runtimeAdmission": False,
            "gameMinute": clock, "actors": [
                {"id": s["id"], "nodeId": starts.get(s["id"]), "edge": None,
                 "locationCm": None, "status": "position_supplied" if s["id"] in starts else "entry_required",
                 "activityKey": None, "destinationId": None, "request": None}
                for s in samples], "queues": []}


def _positive(value, name):
    if type(value) not in (int, float) or not math.isfinite(value) or value <= 0:
        raise ValueError(name + " must be finite and positive")
    return value


def _move(actor, graph, target, budget, blocked):
    nodes = {n["id"]: n for n in graph["nodes"]}
    edges = {e["id"]: e for e in graph["edges"]}
    while budget > 0:
        cursor = actor["edge"]
        if cursor is None:
            if actor["nodeId"] == target:
                actor["status"] = "arrived"
                return
            path = route(graph, actor["nodeId"], target, blocked_edges=blocked)
            if path["status"] == "unavailable":
                actor["status"] = "route_unavailable"
                return
            cursor = {"id": path["edgeIds"][0], "from": path["nodeIds"][0],
                      "to": path["nodeIds"][1], "distanceCm": 0}
            actor["edge"] = cursor
        edge = edges[cursor["id"]]
        if not edge["available"] or edge["id"] in blocked or "walk" not in edge["modes"]:
            actor["status"] = "edge_blocked"
            return
        start, end = nodes[cursor["from"]]["locationCm"], nodes[cursor["to"]]["locationCm"]
        length = math.dist(start, end)
        travel = min(budget, max(0, length - cursor["distanceCm"]))
        cursor["distanceCm"] += travel
        budget -= travel
        fraction = cursor["distanceCm"] / length if length else 1
        actor["locationCm"] = [a + (b - a) * fraction for a, b in zip(start, end)]
        actor["status"] = "travelling"
        if cursor["distanceCm"] >= length:
            actor["nodeId"], actor["edge"] = cursor["to"], None
            if actor["nodeId"] == target:
                actor["status"] = "arrived"
                return


def step_activity(graph, routines, events, policy, previous, game_minute,
                  blocked_edge_ids=(), cancelled_actor_ids=()):
    """Advance exactly one game minute; rewind/seek requires a fresh checkpoint.

    Intent changes finish the current available edge before replanning. Blocked
    edges retain their exact cursor. Requests expire with the activity, and a
    completed service does not submit again until the activity changes.
    """
    clock = minute(game_minute)
    if previous.get("authoringOnly") is not True or previous.get("runtimeAdmission") is not False:
        raise ValueError("Require an authoring-only checkpoint")
    if clock != minute(previous["gameMinute"]) + 1:
        raise ValueError("Advance one minute from the checkpoint; do not skip or rewind")
    if policy.get("authoringOnly") is not True or policy.get("runtimeAdmission") is not False:
        raise ValueError("Require an authoring-only policy")
    if graph.get("authoringOnly") is not True or graph.get("runtimeAdmission") is not False:
        raise ValueError("Require an authoring-only navigation graph")
    budget = (100 * _positive(policy["walkingMetresPerGameSecond"], "walking speed")
              * _positive(policy["gameSecondsPerMinute"], "game clock scale"))
    samples = {s["id"]: s for s in sample_routines(routines, clock)}
    cancelled = set(cancelled_actor_ids)
    if cancelled - set(samples):
        raise ValueError("Unknown cancelled routine")
    nodes = {n["id"]: n for n in graph["nodes"]}
    destinations = {d["id"]: d for d in graph["destinations"]}
    actors = deepcopy(previous["actors"])
    if len(actors) != len(samples) or {a["id"] for a in actors} != set(samples):
        raise ValueError("Checkpoint routine identities changed")
    queue_ids = [q["destinationId"] for q in policy["queues"]]
    if len(set(queue_ids)) != len(queue_ids) or any(d not in destinations for d in queue_ids):
        raise ValueError("Queue rules require unique known destinations")
    blocked = set(blocked_edge_ids)
    if blocked - {e["id"] for e in graph["edges"]}:
        raise ValueError("Unknown blocked edge")
    active_events = sample_events(events, clock)
    blocked.update(edge for event in active_events for edge in event["blockedEdgeIds"])
    if blocked - {e["id"] for e in graph["edges"]}:
        raise ValueError("Event contains an unknown blocked edge")
    for actor in sorted(actors, key=lambda a: a["id"]):
        sample = samples[actor["id"]]
        key = [sample["activityStartGameMinute"], sample["destinationId"], sample["requestedAction"]]
        changed = actor["activityKey"] != key
        if changed:
            actor["request"] = None
        actor["activityKey"], actor["destinationId"] = key, sample["destinationId"]
        if actor["id"] in cancelled:
            actor["request"] = {"arrivalMinute": clock, "status": "cancelled", "serviceStartMinute": None}
        if actor["nodeId"] is not None and actor["locationCm"] is None:
            actor["locationCm"] = list(nodes[actor["nodeId"]]["locationCm"])
        if sample["action"] == "authoring_despawn_marker":
            actor.update(nodeId=None, edge=None, locationCm=None, request=None, status="offstage")
            continue
        target = destinations.get(sample["destinationId"])
        if target is None or not target["available"] or sample["available"] is not True:
            actor["request"] = None
            actor["status"] = "holding" if actor["nodeId"] is not None else "entry_required"
            continue
        if actor["nodeId"] is None:
            actor["status"] = "entry_required"
            continue
        if actor["request"] and actor["request"]["status"] == "cancelled":
            actor["status"] = "cancelled"
            continue
        # New intentions do not earn movement before their start boundary.
        if sample["elapsedMinute"] > 0:
            _move(actor, graph, target["entryNodeId"], budget, blocked)
        elif actor["edge"] is None and actor["nodeId"] == target["entryNodeId"]:
            actor["status"] = "arrived"
        else:
            actor["status"] = "travel_pending"
        if actor["status"] == "arrived" and actor["request"] is None:
            actor["request"] = {"arrivalMinute": clock, "status": "pending", "serviceStartMinute": None}

    queues = []
    for rule in policy["queues"]:
        destination = rule["destinationId"]
        for field in ("serviceCapacity", "waitingCapacity", "serviceMinutes"):
            if type(rule[field]) is not int or rule[field] < (1 if field != "waitingCapacity" else 0):
                raise ValueError("Invalid queue " + field)
        members = sorted((a for a in actors if a["destinationId"] == destination and a["request"]),
                         key=lambda a: (a["request"]["arrivalMinute"], a["id"]))
        for actor in members:
            request = actor["request"]
            if request["status"] == "service" and clock >= request["serviceStartMinute"] + rule["serviceMinutes"]:
                request["status"] = "completed"
        serving = [a for a in members if a["request"]["status"] == "service"]
        pending = [a for a in members if a["request"]["status"] in ("pending", "waiting", "capacity_rejected")]
        free = max(0, rule["serviceCapacity"] - len(serving))
        for actor in pending[:free]:
            actor["request"].pop("queueRank", None)
            actor["request"].update(status="service", serviceStartMinute=clock)
            serving.append(actor)
        waiting = pending[free:free + rule["waitingCapacity"]]
        for rank, actor in enumerate(waiting):
            actor["request"].update(status="waiting", queueRank=rank)
        for actor in pending[free + rule["waitingCapacity"]:]:
            actor["request"].pop("queueRank", None)
            actor["request"]["status"] = "capacity_rejected"
        for actor in members:
            actor["status"] = actor["request"]["status"]
        demand = sum(e["additionalQueueRequests"] for e in active_events if e["destinationId"] == destination)
        # Unpositioned event demand is reported, never converted into invented arrivals.
        queues.append({"destinationId": destination, "servingIds": [a["id"] for a in serving],
                       "waitingIds": [a["id"] for a in waiting],
                       "eventDemandUnpositioned": demand, "physicalQueueVerified": False})
    return {"schemaVersion": 1, "authoringOnly": True, "runtimeAdmission": False,
            "gameMinute": clock, "actors": actors, "queues": queues,
            "engineTraversalVerified": False, "runtimeActorsMoved": False}

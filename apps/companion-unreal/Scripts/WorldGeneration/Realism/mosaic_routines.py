"""Pure fictional authoring schedules; no resident, utility or runtime authority.

config uses idPrefix and optional routines.actorCount (default 6, maximum 64).
Intervals are half-open integer day minutes [0, 1440). Offstage actions are
authoring spawn/despawn markers, never runtime travel or teleport instructions.
No audio assets, recorded voices or reviewed performances are attached.
"""
from copy import deepcopy


DAY = 1440
PENDING = ("runtime_admission_not_granted", "route_connectivity_unverified")
USES = {
    "print": ("print",), "workshop": ("workshop",),
    "food": ("food", "food-provisions"),
    "home": ("home", "residential"), "idle": ("meeting", "rest"),
}


def _integer(value, name, maximum=None):
    if isinstance(value, bool) or not isinstance(value, int) or value < 0:
        raise ValueError(name + " must be a nonnegative integer")
    if maximum is not None and value > maximum:
        raise ValueError(name + " exceeds " + str(maximum))
    return value


def _windows(start, end, shift):
    shifted = (start + shift) % DAY
    duration = end - start
    first = min(duration, DAY - shifted)
    result = [(shifted, shifted + first, 0)]
    if first < duration:
        result.append((0, duration - first, first))
    return result


def _fits(reserved, windows, capacity):
    events = [(start, 1) for start, end in reserved]
    events.extend((end, -1) for start, end in reserved)
    for start, end, offset in windows:
        events.extend(((start, 1), (end, -1)))
    load = 0
    # Ends precede starts at a shared boundary: intervals are half-open.
    for minute, delta in sorted(events):
        load += delta
        if load > capacity:
            return False
    return True


def build_routines(config, destinations):
    """Return fresh bounded schedules, independent of input order and wall time.

    Only available=True entries with an entry node and positive integer capacity
    can be assigned. Capacity is reserved across overlapping daily intervals.
    Missing/full uses remain unavailable intervals; no substitute home is made.
    """
    prefix = config.get("idPrefix", "mosaic-authoring")
    if not isinstance(prefix, str) or not prefix:
        raise ValueError("idPrefix must be a nonempty string")
    count = _integer(config.get("routines", {}).get("actorCount", 6),
                     "routines.actorCount", 64)
    rows = {}
    for source in destinations:
        identity = source.get("id")
        if not isinstance(identity, str) or not identity or identity in rows:
            raise ValueError("Destinations require unique nonempty string ids")
        row = deepcopy(source)
        row["usable"] = (row.get("available") is True
                         and row.get("entryNodeId") not in (None, "")
                         and not isinstance(row.get("entryNodeId"), bool)
                         and type(row.get("capacity")) is int
                         and row["capacity"] > 0)
        rows[identity] = row
    pools = {use: [rows[key] for key in sorted(rows)
                   if rows[key]["usable"] and rows[key].get("use") in names]
             for use, names in USES.items()}
    reservations = {identity: [] for identity in rows}
    routines = []
    for index in range(count):
        shift = (0, 15, 30, 45)[index % 4]
        intervals = []

        def add(start, end, use, action, optional=False):
            windows = _windows(start, end, shift)
            candidates = pools[use] if use else []
            if candidates:
                pivot = index % len(candidates)
                candidates = candidates[pivot:] + candidates[:pivot]
            chosen = next((row for row in candidates
                           if _fits(reservations[row["id"]], windows,
                                    row["capacity"])), None)
            blocked = []
            if chosen:
                reservations[chosen["id"]].extend((a, b) for a, b, _ in windows)
            elif use and not optional:
                blocked = ["no_available_entry_or_capacity:" + use]
            for a, b, offset in windows:
                intervals.append({
                    "startMinute": a, "endMinute": b,
                    "destinationId": chosen["id"] if chosen else None,
                    "entryNodeId": chosen["entryNodeId"] if chosen else None,
                    "action": action if chosen or not use or optional else "unavailable",
                    "requestedAction": action, "requiredUse": use,
                    "available": True if chosen else (False if blocked else None),
                    "elapsedOffsetMinute": offset, "durationMinute": end - start,
                    "blockedRequirements": blocked[:],
                })
            return chosen

        # Homes must be supplied, usable and capacity-compatible, never inferred
        # from config.buildings or the presence of a residential facade.
        home = add(0, 479, "home", "rest_at_home", optional=True)
        if not home:
            for interval in intervals:
                interval["action"] = "offstage"
                interval["requestedAction"] = "offstage"
                interval["requiredUse"] = None
        add(479, 480, None, "authoring_activity_start_marker" if home
            else "authoring_spawn_marker")
        add(480, 540, "print", "paper_activity")
        add(540, 750, "workshop", "repair_activity")
        add(750, 810, "food", "meal_collection")
        if pools["idle"]:
            add(810, 960, "idle", "local_meeting_or_rest")
        else:
            add(810, 960, "workshop", "repair_activity")
        add(960, 1020, "print", "paper_collection")
        if pools["food"]:
            add(1020, 1080, "food", "meal_collection")
        else:
            add(1020, 1080, None, "offstage_waiting")
        if pools["idle"]:
            add(1080, 1140, "idle", "local_meeting_or_rest")
        else:
            add(1080, 1140, None, "offstage_waiting")
        add(1140, 1141, None, "authoring_despawn_marker")
        add(1141, DAY, None, "offstage")
        # A supplied home is not assumed to support an unverified return route.
        intervals.sort(key=lambda interval: interval["startMinute"])
        routines.append({
            "id": prefix + ":routine:" + str(index), "authoringOnly": True,
            "runtimeId": None, "runtimeAdmitted": False,
            "basis": "Fictional activity sequence; not demographics or resident identity",
            "dayMinuteRange": {"startMinute": 0, "endMinute": DAY},
            "intervals": intervals, "blockedRequirements": list(PENDING),
            "audioAvailable": False, "reviewedPerformanceAvailable": False,
        })
    return routines


def sample_routines(routines, game_minute):
    """Sample repeating authoring intervals using an explicit integer game clock.

    Preserve unavailable activities and offstage markers. Midnight splits retain
    activity elapsed time; negative activityStartGameMinute denotes the previous
    cycle at day zero, not negative input time. This does not admit or move actors.
    """
    minute = _integer(game_minute, "game_minute")
    day_index, day_minute = divmod(minute, DAY)
    samples = []
    identities = set()
    for routine in routines:
        identity = routine["id"]
        if identity in identities or routine.get("authoringOnly") is not True:
            raise ValueError("Require unique authoring-only routine ids")
        identities.add(identity)
        expected = 0
        active = None
        for interval in routine["intervals"]:
            start = _integer(interval["startMinute"], "startMinute", DAY - 1)
            end = _integer(interval["endMinute"], "endMinute", DAY)
            if start != expected or end <= start:
                raise ValueError("Intervals must be sorted, complete and nonoverlapping")
            expected = end
            if start <= day_minute < end:
                active = interval
        if expected != DAY or active is None:
            raise ValueError("Intervals must cover the complete day")
        offset = _integer(active.get("elapsedOffsetMinute", 0), "elapsedOffsetMinute")
        duration = _integer(active.get("durationMinute", active["endMinute"]
                                      - active["startMinute"]), "durationMinute")
        elapsed = offset + day_minute - active["startMinute"]
        if duration < offset + active["endMinute"] - active["startMinute"]:
            raise ValueError("Invalid wrapped activity duration")
        sample = deepcopy(active)
        if sample.get("available") is False:
            sample["action"] = "unavailable"
            if not sample.get("blockedRequirements"):
                sample["blockedRequirements"] = ["destination_unavailable"]
        sample.update({
            "id": identity, "authoringOnly": True, "runtimeAdmitted": False,
            "gameMinute": minute, "dayIndex": day_index, "dayMinute": day_minute,
            "elapsedMinute": elapsed, "remainingMinute": duration - elapsed,
            "activityStartGameMinute": minute - elapsed,
            "activityEndGameMinute": minute - elapsed + duration,
            "intervalStartGameMinute": minute - day_minute + active["startMinute"],
            "intervalEndGameMinute": minute - day_minute + active["endMinute"],
            "blockedRequirements": deepcopy(routine.get("blockedRequirements", []))
                                   + deepcopy(sample.get("blockedRequirements", [])),
            "audioAvailable": False, "reviewedPerformanceAvailable": False,
        })
        samples.append(sample)
    return samples

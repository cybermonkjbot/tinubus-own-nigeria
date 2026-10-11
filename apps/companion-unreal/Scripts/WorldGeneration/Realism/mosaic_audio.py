"""Pure editor-only audio intent; fictional routines, no recordings or engine I/O.

Optional config: audioAssets maps cue keys to paths; audioPerformances maps
cue keys to reviewed booleans; audioSourceStates maps destination IDs to
activity-state dictionaries. installedGenerator=True belongs to a building.
Paths are caller-supplied references, not file-existence or rights verification.
"""
import copy
import math


# Daily minute windows and attenuation are fictional local design values.
_USES = {
    "print": ("printer", "printing", ((480, 1080),), 350, "on"),
    "residential": ("indoor-conversation", "conversation", ((420, 540), (1080, 1320)), 250, None),
    "workshop": ("workshop", "manualToolWork", ((480, 1080),), 450, None),
    "food-provisions": ("food", "foodPreparation", ((360, 1260),), 300, None),
}
_POWER = (None, "on", "outage", "unavailable")


def _location(value):
    if not isinstance(value, (list, tuple)) or len(value) != 3:
        raise ValueError("locationCm must contain three native centimetre values")
    if any(type(v) not in (int, float) or (type(v) is float and not math.isfinite(v))
           for v in value):
        raise ValueError("locationCm must be finite numbers, not booleans")
    return list(value)


def build_emitters(config, destinations):
    """Return localized intent for available destinations without mutating inputs.

    No asset/performance is presumed. Source states are explicit caller values;
    absent activity remains unknown rather than scheduled simulated occupancy.
    Locations already use imported native centimetres; no axis conversion.
    """
    buildings = {row["id"]: row for row in config["buildings"]}
    if len(buildings) != len(config["buildings"]):
        raise ValueError("Duplicate configured building identity")
    assets = config.get("audioAssets", {})
    performances = config.get("audioPerformances", {})
    states = config.get("audioSourceStates", {})
    rows, seen = [], set()
    for destination in destinations:
        if destination.get("available") is not True:
            continue
        identity = destination["id"]
        if not isinstance(identity, str) or not identity or identity in seen:
            raise ValueError("Available destination IDs must be unique nonempty strings")
        seen.add(identity)
        building = buildings.get(destination["buildingId"])
        if building is None or building["use"] != destination["use"]:
            raise ValueError("Destination must retain its configured building use")
        spec = _USES.get(destination["use"])
        if spec is None:
            continue
        location = _location(destination["locationCm"])
        capacity = destination["capacity"]
        if type(capacity) is not int or capacity < 0:
            raise ValueError("Destination capacity must be a nonnegative integer")
        source_states = states.get(identity, {})
        if not isinstance(source_states, dict):
            raise ValueError("audioSourceStates entries must be dictionaries")
        specs = [spec]
        if building.get("installedGenerator") is True:
            specs.append(("generator", "generatorRunning", spec[2], 500, "outage"))
        for cue, activity, schedule, attenuation, power in specs:
            asset = assets.get(cue)
            asset = asset.strip() if isinstance(asset, str) and asset.strip() else None
            speech = cue == "indoor-conversation"
            reviewed = speech and performances.get(cue) is True
            rows.append({
                "id": config["idPrefix"] + ":audio:" + identity + ":" + cue,
                "destinationId": identity, "buildingId": destination["buildingId"],
                "entryNodeId": destination["entryNodeId"], "use": destination["use"],
                "available": True, "capacity": capacity, "cue": cue,
                "locationCm": list(location), "attenuationCm": attenuation,
                "coordinateBasis": "already-converted-local-native-centimetres",
                "scheduleMinutes": [list(window) for window in schedule],
                "sourceStateRequirements": {activity: True},
                "sourceStates": copy.deepcopy(source_states),
                "requiredPowerStatus": power,
                "installedGenerator": building.get("installedGenerator") is True,
                "assetPath": asset, "audioAssetStatus": "supplied-unverified" if asset else "pending",
                "performanceRequired": speech,
                "performanceStatus": "caller-reviewed" if reviewed else "pending" if speech else "not-applicable",
                "recordingVerified": False, "rightsVerified": False,
                "enginePlaybackIntegrated": False,
                "dialogue": None, "music": None, "runtimeId": None,
                "authoringOnly": True, "routineBasis": "fictional-not-demographic",
            })
    return rows


def sample_emitters(emitters, game_minute, power_status=None):
    """Return requested/active intent, using caller game minutes and power only.

    Windows repeat daily, start-inclusive/end-exclusive. Active needs explicit
    activity and required power; unknown activity/power never grants activation.
    Candidate readiness only means a supplied path and caller-reviewed speech.
    Playable stays False: recording, rights and engine integration are not verified.
    No asset resolution, audio playback, utility mutation or device clock access.
    """
    if type(game_minute) is not int or game_minute < 0:
        raise ValueError("game_minute must be a finite nonnegative integer, not bool")
    if power_status not in _POWER:
        raise ValueError("power_status must be on, outage, unavailable or None")
    minute = game_minute % 1440
    result = []
    for emitter in emitters:
        row = copy.deepcopy(emitter)
        scheduled = any(start <= minute < end for start, end in row["scheduleMinutes"])
        requested = row.get("available") is True and row["capacity"] > 0 and scheduled
        values = [row.get("sourceStates", {}).get(key)
                  for key in row["sourceStateRequirements"]]
        if any(value is False for value in values):
            source_status = "inactive"
        else:
            source_status = "active" if values and all(v is True for v in values) else "unavailable"
        required = row["requiredPowerStatus"]
        power_met = required is None or power_status == required
        generator_met = row["cue"] != "generator" or row.get("installedGenerator") is True
        active = requested and source_status == "active" and power_met and generator_met
        asset = row.get("assetPath")
        supplied = isinstance(asset, str) and bool(asset.strip())
        reviewed = not row["performanceRequired"] or row["performanceStatus"] == "caller-reviewed"
        row.update(gameMinute=game_minute, minuteOfDay=minute, scheduled=scheduled,
                   requested=requested, active=active, sourceStateStatus=source_status,
                   powerStatus=power_status, powerRequirementMet=power_met,
                   assetPathProvided=supplied, candidateAssetReady=supplied and reviewed,
                   recordingVerified=False, rightsVerified=False,
                   enginePlaybackIntegrated=False, playable=False)
        result.append(row)
    return result

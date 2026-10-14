"""Dimensioned fictional fit-outs for the mapped Ebute pocket; no engine writes."""
import copy

from WorldGeneration.Realism.mosaic_layout import rotate


# Dimensions are authored centimetres, not measurements of Lagos premises.
MESHES = {
    "service-counter": ([60, 140, 92], "closed-box", "painted-timber"),
    "workbench": ([180, 80, 85], "closed-box", "worked-timber"),
    "chair-frame": ([60, 65, 90], "convex-hull", "worked-timber"),
    "foam-offcuts": ([70, 50, 45], "none", "foam-fabric"),
    "fabric-roll-rack": ([110, 45, 180], "closed-box", "painted-metal"),
    "hand-tool-board": ([90, 12, 100], "none", "worked-timber"),
    "sewing-table": ([120, 65, 78], "closed-box", "painted-metal"),
    "fitting-screen": ([160, 10, 200], "closed-box", "foam-fabric"),
    "garment-rail": ([120, 55, 170], "convex-hull", "painted-metal"),
    "lidded-stock-bin": ([60, 45, 55], "closed-box", "washed-plastic"),
    "provisions-shelf": ([140, 35, 180], "closed-box", "painted-timber"),
    "wash-basin": ([60, 50, 85], "convex-hull", "washed-plastic"),
    "stacking-stool": ([40, 40, 45], "convex-hull", "washed-plastic"),
    "covered-container": ([45, 45, 60], "closed-box", "washed-plastic"),
    "folding-table": ([120, 70, 74], "closed-box", "painted-timber"),
    "drying-rail": ([150, 50, 170], "convex-hull", "painted-metal"),
    "repair-clamps": ([65, 35, 20], "none", "painted-metal"),
    "wrapped-collection": ([90, 65, 80], "closed-box", "foam-fabric"),
    "job-ticket-board": ([60, 8, 65], "none", "painted-timber"),
    "door-curtain": ([120, 8, 210], "none", "foam-fabric"),
    "shutter-door": ([130, 15, 240], "closed-box", "painted-metal"),
    "drain-cover": ([90, 45, 8], "closed-box", "mineral-render"),
    "canopy-module": ([300, 180, 20], "closed-box", "painted-metal"),
    "shielded-wall-light": ([24, 18, 35], "none", "painted-metal"),
    "small-fan": ([45, 30, 55], "convex-hull", "washed-plastic"),
}

MATERIALS = {
    "mineral-render": {
        "baseRoughness": [0.72, 0.92], "metallic": 0,
        "layers": ["substrate", "older paint", "bounded repair patch", "sheltered finish", "wet contact"],
        "masks": ["repair polygon", "roof shelter", "verified water-contact region", "touch region"],
        "wetRoughnessFloor": 0.48, "wetDarkeningLimit": 0.18,
        "rules": ["No universal flood tide-line", "No invented building age from wear",
                  "Keep repair boundaries independent of procedural noise"],
    },
    "painted-metal": {
        "baseRoughness": [0.45, 0.78], "metallic": 0,
        "layers": ["opaque paint", "exposed metal", "oxide only on exposed metal", "rain streak"],
        "masks": ["paint loss", "fastener contact", "edge handling", "roof shelter"],
        "exposedMetallic": 1, "oxideMetallic": 0,
        "rules": ["Paint and rust are dielectric; metallic applies only to exposed substrate",
                  "Corrugation normal must agree with roof geometry and runoff direction"],
    },
    "painted-timber": {
        "baseRoughness": [0.5, 0.82], "metallic": 0,
        "layers": ["directional wood grain", "paint", "hand-polished edge", "localized repair"],
        "masks": ["grain UV", "paint loss", "touch region"],
        "rules": ["End grain differs from longitudinal grain", "No rust on timber"],
    },
    "worked-timber": {
        "baseRoughness": [0.55, 0.88], "metallic": 0,
        "layers": ["wood grain", "clamp pressure", "bounded adhesive residue", "fresh sanded patch"],
        "masks": ["tool contact", "joint region", "repair stage"],
        "rules": ["Fresh repair changes a specific joint, not the whole chair"],
    },
    "foam-fabric": {
        "baseRoughness": [0.78, 0.98], "metallic": 0,
        "layers": ["weave or foam pore", "fold", "compression", "separate stitched seam"],
        "masks": ["seam UV", "fold region", "compression region"],
        "rules": ["Do not assign named cultural textile patterns without reviewed references"],
    },
    "washed-plastic": {
        "baseRoughness": [0.3, 0.68], "metallic": 0,
        "layers": ["moulded body", "handling scuff", "bounded water film"],
        "masks": ["touch region", "wet contact"],
        "rules": ["Food stock and wash equipment stay visually separate"],
    },
}

FINISH_RECIPES = {
    "maintained-sheltered": {"shelteredPaintCoverageTarget": 0.9, "repairPatchCoverageTarget": 0.04,
                             "touchPolishStrengthTarget": 0.15, "exteriorWetMask": "exposed surfaces only"},
    "bounded-repair": {"shelteredPaintCoverageTarget": 0.75, "repairPatchCoverageTarget": 0.12,
                       "touchPolishStrengthTarget": 0.12, "repairMask": "one artist-bounded region"},
    "tool-contact": {"shelteredPaintCoverageTarget": 0.8, "repairPatchCoverageTarget": 0.06,
                     "touchPolishStrengthTarget": 0.35, "residueMask": "active bench/joint only"},
    "washed-service": {"shelteredPaintCoverageTarget": 0.85, "repairPatchCoverageTarget": 0.04,
                       "touchPolishStrengthTarget": 0.2, "waterFilmMask": "wash equipment only"},
    "sheltered-exposed-split": {"shelteredPaintCoverageTarget": 0.85, "repairPatchCoverageTarget": 0.05,
                               "touchPolishStrengthTarget": 0.12, "shelterMask": "geometry-derived canopy edge"},
}

AUDIO_STATES = {
    "household": [], "cushion-repair": ["cut", "fit"], "furniture-repair": ["prepare"],
    "alterations": ["sew"], "food-provisions": ["shelve", "pack"],
    "shared-household-workroom": ["work"], "household-workroom": ["work"],
    "small-workroom": ["hand-repair"], "shared-yard-workroom": ["sort"],
}

WORK_TIMING = {
    "receive-item": [8, 20], "receive": [8, 20], "tag": [5, 12], "measure": [15, 40],
    "cut": [30, 80], "stitch-cover": [45, 120], "fit": [20, 55], "inspect": [10, 30],
    "collect": [8, 25], "assess-joint": [15, 40], "disassemble": [30, 90],
    "prepare": [30, 100], "glue-and-clamp": [30, 75], "cure": [300, 900],
    "record-size-with-consent": [15, 35], "mark": [15, 40], "sew": [40, 100],
    "private-fitting": [60, 150], "receive-sealed-stock": [20, 60], "check": [10, 30],
    "shelve": [20, 60], "take-order": [8, 20], "pack": [10, 25], "handover": [5, 15],
    "clean": [25, 70], "set-out": [20, 60], "work": [60, 180], "pack-away": [20, 60],
    "restore-shared-space": [15, 45], "hand-repair": [45, 120], "assess": [15, 40],
    "sort": [30, 90], "dry": [180, 600], "bring-in": [20, 60], "clear-shared-yard": [15, 45],
}

# Bounds are min/max in frontage-local X depth, Y along frontage, Z up.
# Wider/deeper rooms below replace the shallow imported shell; they do not extend it invisibly.
PROFILES = {
    "household": {
        "rooms": [("threshold", [20, -340, 0], [180, -20, 260], "resident-invitation"),
                  ("living", [200, -560, 0], [560, -20, 280], "private"),
                  ("service", [200, -1100, 0], [500, -580, 280], "private")],
        "props": [("door-curtain", "threshold", [90, -180, 105]),
                  ("stacking-stool", "living", [400, -220, 22.5]),
                  ("covered-container", "service", [360, -840, 30]),
                  ("small-fan", "living", [450, -450, 27.5])],
        "workflow": ["leave-home", "return-to-threshold", "unlock", "enter", "close"],
        "sound": ("household-room-tone", "living", "resident-present", 700),
        "queueCapacity": 0,
    },
    "cushion-repair": {
        "rooms": [("collection", [20, -340, 0], [200, -20, 280], "counter"),
                  ("cutting", [240, -550, 0], [700, -20, 300], "staff"),
                  ("storage", [240, -1100, 0], [650, -580, 300], "staff")],
        "props": [("service-counter", "collection", [170, -180, 46]),
                  ("workbench", "cutting", [460, -200, 42.5]),
                  ("foam-offcuts", "cutting", [600, -450, 22.5]),
                  ("fabric-roll-rack", "storage", [440, -850, 90]),
                  ("wrapped-collection", "storage", [560, -660, 40])],
        "workflow": ["receive-item", "tag", "measure", "cut", "stitch-cover", "fit", "inspect", "collect"],
        "sound": ("fabric-cutting-and-hand-staple", "cutting", "cut-or-fit", 1200),
        "queueCapacity": 2,
    },
    "furniture-repair": {
        "rooms": [("collection", [20, -340, 0], [200, -20, 280], "counter"),
                  ("bench", [240, -560, 0], [760, -20, 300], "staff"),
                  ("cure-store", [240, -1100, 0], [760, -580, 300], "staff")],
        "props": [("service-counter", "collection", [170, -180, 46]),
                  ("workbench", "bench", [460, -210, 42.5]),
                  ("chair-frame", "bench", [630, -430, 45]),
                  ("hand-tool-board", "bench", [300, -554, 170]),
                  ("repair-clamps", "bench", [460, -210, 95]),
                  ("wrapped-collection", "cure-store", [530, -840, 40])],
        "workflow": ["assess-joint", "disassemble", "prepare", "glue-and-clamp", "cure", "inspect", "collect"],
        "sound": ("bounded-hand-tool-taps", "bench", "prepare", 1300),
        "queueCapacity": 2,
    },
    "alterations": {
        "rooms": [("collection", [20, -340, 0], [200, -20, 280], "counter"),
                  ("sewing", [240, -560, 0], [650, -20, 300], "staff"),
                  ("fitting", [240, -1100, 0], [650, -580, 300], "customer-invitation")],
        "props": [("service-counter", "collection", [170, -180, 46]),
                  ("sewing-table", "sewing", [450, -200, 39]),
                  ("garment-rail", "sewing", [470, -430, 85]),
                  ("fitting-screen", "fitting", [390, -840, 100])],
        "workflow": ["receive", "record-size-with-consent", "mark", "sew", "inspect", "private-fitting", "collect"],
        "sound": ("sewing-machine-pulses", "sewing", "sew", 1000),
        "queueCapacity": 2,
    },
    "food-provisions": {
        "rooms": [("counter", [20, -340, 0], [200, -20, 280], "counter"),
                  ("stock", [240, -560, 0], [650, -20, 300], "staff"),
                  ("wash", [240, -1100, 0], [650, -580, 300], "staff")],
        "props": [("service-counter", "counter", [170, -180, 46]),
                  ("provisions-shelf", "stock", [470, -210, 90]),
                  ("lidded-stock-bin", "stock", [360, -440, 27.5]),
                  ("wash-basin", "wash", [400, -840, 42.5])],
        "workflow": ["receive-sealed-stock", "check", "shelve", "take-order", "pack", "handover", "clean"],
        "sound": ("stock-handling-and-conversation", "stock", "shelve-or-pack", 900),
        "queueCapacity": 2,
    },
    "shared-household-workroom": {
        "rooms": [("threshold", [20, -340, 0], [200, -20, 280], "resident-invitation"),
                  ("workroom", [240, -560, 0], [650, -20, 300], "staff"),
                  ("domestic", [240, -1100, 0], [650, -580, 300], "private")],
        "props": [("door-curtain", "threshold", [120, -180, 105]),
                  ("folding-table", "workroom", [460, -220, 37]),
                  ("lidded-stock-bin", "workroom", [410, -450, 27.5]),
                  ("covered-container", "domestic", [460, -840, 30])],
        "workflow": ["reserve-workroom", "set-out", "work", "pack-away", "restore-shared-space"],
        "sound": ("packing-and-fabric-handling", "workroom", "work", 800),
        "queueCapacity": 0,
    },
    "household-workroom": {
        "rooms": [("collection", [20, -340, 0], [200, -20, 280], "counter"),
                  ("workroom", [240, -560, 0], [650, -20, 300], "staff"),
                  ("domestic", [240, -1100, 0], [650, -580, 300], "private")],
        "props": [("service-counter", "collection", [170, -180, 46]),
                  ("folding-table", "workroom", [450, -220, 37]),
                  ("garment-rail", "workroom", [460, -450, 85]),
                  ("door-curtain", "domestic", [400, -840, 105])],
        "workflow": ["open-counter", "receive", "work", "collect", "close-counter", "domestic-time"],
        "sound": ("packing-and-fabric-handling", "workroom", "work", 800),
        "queueCapacity": 1,
    },
    "small-workroom": {
        "rooms": [("collection", [20, -340, 0], [200, -20, 280], "counter"),
                  ("bench", [240, -560, 0], [650, -20, 300], "staff"),
                  ("store", [240, -1100, 0], [650, -580, 300], "staff")],
        "props": [("service-counter", "collection", [170, -180, 46]),
                  ("workbench", "bench", [450, -210, 42.5]),
                  ("hand-tool-board", "bench", [300, -554, 170]),
                  ("lidded-stock-bin", "store", [460, -840, 27.5])],
        "workflow": ["receive", "assess", "hand-repair", "inspect", "collect"],
        "sound": ("bounded-hand-tool-taps", "bench", "hand-repair", 950),
        "queueCapacity": 1,
    },
    "shared-yard-workroom": {
        "rooms": [("collection", [20, -340, 0], [200, -20, 280], "counter"),
                  ("workroom", [240, -560, 0], [650, -20, 300], "staff"),
                  ("yard", [240, -1100, 0], [800, -580, 300], "resident-invitation")],
        "props": [("service-counter", "collection", [170, -180, 46]),
                  ("folding-table", "workroom", [450, -220, 37]),
                  ("drying-rail", "yard", [470, -840, 85]),
                  ("covered-container", "yard", [690, -680, 30])],
        "workflow": ["reserve-yard-slot", "set-out", "sort", "dry", "bring-in", "clear-shared-yard"],
        "sound": ("packing-and-fabric-handling", "workroom", "sort", 900),
        "queueCapacity": 1,
    },
}

# Each identity is an invented occupant/use, never an address or trading-name claim.
PREMISES = {
    "freeman-01": ("Curtained return-home threshold", "maintained-sheltered", "Curtain closes after entry; no street view into the living room.", "covered-container", "service"),
    "freeman-02": ("Single cushion cover replacement", "tool-contact", "One cushion is unwrapped; matching completed pieces remain tagged in storage.", "job-ticket-board", "storage"),
    "freeman-03": ("Fold-away shared packing room", "bounded-repair", "Table folds away when the booked work slot ends; the domestic door stays separate.", "stacking-stool", "workroom"),
    "freeman-04": ("Recently repaired entrance panel", "bounded-repair", "One render patch has a clean edge; adjacent protected paint is retained.", "stacking-stool", "service"),
    "freeman-05": ("Collection counter beside a home", "maintained-sheltered", "Wrapped items move to the counter only for collection, not onto the pavement.", "wrapped-collection", "workroom"),
    "freeman-06": ("Evening reading room", "maintained-sheltered", "Domestic light remains behind a curtain; no automatic outage or distress state.", "folding-table", "service"),
    "freeman-07": ("Hand-tool repair bench", "tool-contact", "The currently used tool leaves an empty board socket; the rest stay arranged.", "repair-clamps", "bench"),
    "market-01": ("Small sealed-provisions counter", "washed-service", "A stock check separates incoming cartons from sale-ready sealed goods.", "job-ticket-board", "stock"),
    "market-02": ("Quiet household frontage", "bounded-repair", "Shutters are ordinary household boundaries, not a shop closure story.", "covered-container", "living"),
    "market-03": ("Booked shared drying yard", "sheltered-exposed-split", "Items are brought under cover before rain; the yard is not a public shortcut.", "stacking-stool", "yard"),
    "griffith-01": ("Clamped chair joint", "tool-contact", "Clamps persist through curing; no finished chair appears before inspection.", "job-ticket-board", "cure-store"),
    "griffith-02": ("Resident threshold with resting place", "maintained-sheltered", "A stool is inside the property; the public route stays clear.", "stacking-stool", "service"),
    "griffith-03": ("Alterations and private fitting", "maintained-sheltered", "Collection continues outside a closed fitting vestibule without revealing the customer.", "job-ticket-board", "sewing"),
    "griffith-04": ("Domestic storage behind a screen", "bounded-repair", "Covered containers belong behind the private door, not as generic street clutter.", "covered-container", "living"),
    "willoughby-01": ("Repair awaiting collection", "tool-contact", "A wrapped repaired frame occupies storage while the next joint is assessed.", "chair-frame", "cure-store"),
    "willoughby-02": ("Sheltered domestic entrance", "sheltered-exposed-split", "Rain darkens only the exposed face; the protected door remains distinct.", "stacking-stool", "service"),
    "kadara-01": ("Low-activity household edge", "maintained-sheltered", "Passing residents do not turn into a vendor crowd at every doorway.", "covered-container", "living"),
    "kadara-02": ("Shared room restored after work", "washed-service", "Packing bins close and table clears before the household uses the room again.", "stacking-stool", "workroom"),
}


def _world(frame, point):
    offset = rotate(point, frame["yaw"])
    return [a + b for a, b in zip(frame["originCm"], offset)]


def _volume(frame, low, high):
    center = [(a + b) / 2 for a, b in zip(low, high)]
    return {"localBoundsCm": [low, high], "centerCm": _world(frame, center),
            "sizeCm": [b - a for a, b in zip(low, high)], "yaw": frame["yaw"]}


def plan(layout):
    buildings = layout["buildings"]
    if {row["id"] for row in buildings} != set(PREMISES):
        raise ValueError("Detailed premises must cover the exact mapped frontage set")
    premises, props, emitters, portals = [], [], [], []
    for building in buildings:
        identity, frame = building["id"], building["authoringFrame"]
        title, finish, story, signature_mesh, signature_room = PREMISES[identity]
        profile = PROFILES[building["fictionalUse"]]
        rooms = [{"id": identity + ":" + name, "name": name, "access": access,
                  **_volume(frame, low, high), "geometryCreated": False,
                  "replacementShellRequired": True, "navigationAdmitted": False}
                 for name, low, high, access in profile["rooms"]]
        room_by_name = {room["name"]: room for room in rooms}
        fitout = list(profile["props"])
        room = room_by_name[signature_room]
        low, high = room["localBoundsCm"]
        # A distinct socket near the back edge leaves the standard central work socket intact.
        signature_size = MESHES[signature_mesh][0]
        signature_height = {"job-ticket-board": 170, "repair-clamps": 95}.get(signature_mesh, signature_size[2] / 2)
        signature_point = [high[0] - signature_size[0] / 2 - 15,
                           low[1] + signature_size[1] / 2 + 15, signature_height]
        if signature_mesh == "job-ticket-board":
            signature_point[1] = low[1] + signature_size[1] / 2
        elif signature_mesh == "repair-clamps":
            bench = next(point for mesh, _, point in fitout if mesh == "workbench")
            signature_point = [bench[0], bench[1], 95]
        fitout.append((signature_mesh, signature_room, signature_point))
        local_prop_ids = []
        for index, (mesh, room_name, local) in enumerate(fitout):
            size, collision, material = MESHES[mesh]
            room = room_by_name[room_name]
            low, high = room["localBoundsCm"]
            if any(local[i] - size[i] / 2 < low[i] or local[i] + size[i] / 2 > high[i]
                   for i in range(3)):
                raise ValueError("Prop socket exceeds proposed room: " + identity + ":" + mesh)
            prop_id = identity + ":fitout:" + str(index)
            local_prop_ids.append(prop_id)
            props.append({"id": prop_id, "premiseId": identity, "roomId": room["id"],
                          "meshRecipeId": mesh, "localCenterCm": local,
                          "transform": {"locationCm": _world(frame, local), "yaw": frame["yaw"], "scale": [1, 1, 1]},
                          "sizeCm": size, "collisionRecipe": collision, "materialRecipeId": material,
                          "runtimeAssetPath": None, "meshCreated": False,
                          "enabled": False, "placementClearanceReviewed": False})
        threshold_name = profile["rooms"][0][0]
        for room in rooms:
            low, high = room["localBoundsCm"]
            doorway = [low[0], (low[1] + high[1]) / 2, 120]
            portals.append({"id": room["id"] + ":door", "premiseId": identity,
                            "roomId": room["id"], "locationCm": _world(frame, doorway),
                            "yaw": frame["yaw"], "clearWidthCm": 120, "clearHeightCm": 240,
                            "access": room["access"], "connectsTo": "proposed-property-circulation",
                            "publicShortcut": False, "openingMeshCreated": False,
                            "state": "closed", "egressMustRemainPossible": True})
        sound, room_name, activity_description, radius = profile["sound"]
        audio_states = AUDIO_STATES[building["fictionalUse"]]
        if not set(audio_states).issubset(profile["workflow"]):
            raise ValueError("Audio gate references an unauthored work state")
        sound_room = room_by_name[room_name]
        low, high = sound_room["localBoundsCm"]
        emitter_point = [(low[0] + high[0]) / 2, (low[1] + high[1]) / 2, 120]
        emitters.append({"id": identity + ":activity-audio", "premiseId": identity,
                         "roomId": sound_room["id"], "locationCm": _world(frame, emitter_point),
                         "sourceRecipe": sound, "activityDescription": activity_description,
                         "activityStateIds": audio_states, "occupiedRoomRequired": True,
                         "maxDistanceCm": radius, "nearFieldCm": 100,
                         "attenuation": "smooth-distance-falloff; no audible hard radius boundary",
                         "occlusion": {"traceWalls": True, "routeThroughDoorPortals": True,
                                       "closedDoorGainDbTarget": -14, "closedDoorLowpassHzTarget": 1200},
                         "valuesAreMixTargetsNotMeasurements": True,
                         "assetPath": None, "recordingsPresent": False, "rightsCleared": False,
                         "playable": False, "speechBus": "independent-of-tools"})
        capacity = profile["queueCapacity"]
        premises.append({"id": identity, "streetId": building["streetId"], "title": title,
                         "useIsFictional": True, "finishRecipe": finish, "microStory": story,
                         "authoringFrame": copy.deepcopy(frame), "rooms": rooms,
                         "propIds": local_prop_ids, "thresholdRoomId": identity + ":" + threshold_name,
                         "replacementGeometry": {"required": True, "created": False,
                                                  "maxDepthCm": max(r[2][0] for r in profile["rooms"]),
                                                  "importedShellDepthCmApprox": 322,
                                                  "parcelOverlapReviewed": False,
                                                  "circulationAndExitGeometryAuthored": False},
                         "activity": {"orderedStates": profile["workflow"], "runtimeImplemented": False,
                                      "stateSpecifications": [{"id": state,
                                          "durationSecondsTarget": WORK_TIMING.get(state),
                                          "advance": "completed work" if state in WORK_TIMING else "actor intent or reservation granted",
                                          "interrupt": "preserve item stage; release tool and visitor reservations"}
                                          for state in profile["workflow"]],
                                      "timingsAreGamePacingNotTradeMeasurements": True,
                                      "schedule": {"opening": "individual premise schedule; not a Lagos opening-hours claim",
                                                   "closed": "finish safe work stage, close counter, keep exit available",
                                                   "rain": "pause exposed work and move only to a free admitted recess"},
                                      "workSocketReservation": "one worker per active tool; release on cancellation",
                                      "itemIdentityPersists": True, "skipUnfinishedProductSpawns": True,
                                      "genderOrEthnicityDeterminesJob": False},
                         "service": {"capacity": capacity, "interactionRadiusCm": 180,
                                     "waitingRecess": (_volume(frame, [20, -1100, 0], [200, -560, 280])
                                                       if capacity else None),
                                     "waitingSocketsLocalCm": [[110, -695 - i * 270, 0] for i in range(capacity)],
                                     "waitingSocketsWorldCm": [_world(frame, [110, -695 - i * 270, 0]) for i in range(capacity)],
                                     "waitingRecessGeometryCreated": False,
                                     "calledCustomerSocketLocalCm": ([80, -180, 0] if capacity else None),
                                     "calledCustomerSocketWorldCm": (_world(frame, [80, -180, 0]) if capacity else None),
                                     "callRule": "one customer leaves waiting recess only when counter approach is clear",
                                     "queueRequiresCollisionFreeSockets": True, "spillToPublicFootway": False,
                                     "overflow": "defer service or leave; do not form another queue",
                                     "allStaffAndCustomersMayDecline": True, "navigationAdmitted": False}})
    mesh_recipes = [{"id": name, "sizeCm": size, "pivot": "center; placement Z accounts for half-height",
                     "collisionRecipe": collision, "materialRecipeId": material,
                     "lodBudgetTriangles": [2400, 900, 220], "lodDistancesMetresTarget": [0, 12, 35],
                     "budgetIsTargetNotGeneratedMeshCount": True, "naniteRequired": False,
                     "smallParts": "normal detail beyond 12m; retain threshold silhouette",
                     "assetPath": None, "created": False}
                    for name, (size, collision, material) in MESHES.items()]
    return {"schemaVersion": 1, "status": "offline-dimensioned-fictional-authoring",
            "sourceFacts": "Street anchors only; rooms, occupants, objects and numeric tuning are authored",
            "coordinateSpace": "UE centimetres X south, Y east, Z up; local room axes rotate once",
            "premises": premises, "propPlacements": props, "doorPortals": portals,
            "meshRecipes": mesh_recipes, "materialRecipes": copy.deepcopy(MATERIALS),
            "finishRecipes": copy.deepcopy(FINISH_RECIPES),
            "audioEmitters": emitters,
            "spawnRules": {
                "runtimeImplemented": False, "admissionRequires": ["replacement shell and exits",
                    "non-overlapping parcel", "collision clearance", "access review", "reviewed performances"],
                "workerCap": "one active work socket per workroom; no workers at household fronts",
                "customerCap": "sum of admitted service capacities, not street density",
                "residentActivity": "leave/return schedules chosen per household; no demographic template",
                "traffic": "No vehicle or roadside hawker spawning from these room recipes",
                "barks": {"reviewStatus": "unrecorded fictional candidate lines; local performance review pending",
                          "greeting": "How far?", "request": "Abeg, give me small space.",
                          "acknowledgement": "No wahala.", "cooldownSeconds": 45,
                          "rules": "No compulsory Pidgin, accent imitation or ethnicity-to-language assignment"}},
            "weatherAndLighting": {
                "runtimeImplemented": False, "tuningIsAuthoredNotClimatology": True,
                "dry": {"surfaceWetness": 0, "dustMask": "exposure and untouched horizontal surfaces only"},
                "rain": {"wetnessRiseSecondsTarget": 60, "wetnessDrySecondsTarget": 900,
                         "roofSound": "separate metal canopy and sheltered room layers",
                         "relocation": "only to admitted free property recess sockets",
                         "puddles": "bounded authoring masks only; no inferred water plane or outfall",
                         "deepFloodEnabled": False, "publicRouteMustRemainClear": True},
                "haze": {"volumetricExtinctionMultiplierTarget": 1.6,
                         "sunSaturationMultiplierTarget": 0.85, "globalOrangeFilter": False,
                         "rules": "Keep sky and shaded walls distinct; no smoke source implied"},
                "night": {"publicRouteMinimumVisibility": "readable thresholds without household light dependency",
                          "privateWindows": "curtain-occluded, individually scheduled", "globalOutage": False},
                "utility": {"generatorMeshOrSoundAdded": False,
                            "reason": "No verified ventilated parcel, exhaust direction or source recording"}},
            "events": [{
                "id": "ebute:griffith-03-private-fitting", "premiseId": "griffith-03",
                "runtimeImplemented": False, "initialState": "disabled-pending-admission",
                "guards": ["invitation vestibule and street counter admitted", "all visitors can exit",
                           "edge:griffith-e5-e6:1 has verified public continuity in both departure directions",
                           "counter waiting socket free", "no competing route obstruction"],
                "states": [
                    {"id": "invitation-open", "next": "clear-vestibule", "trigger": "consented fitting begins"},
                    {"id": "clear-vestibule", "next": "private-fitting", "trigger": "visitors exited",
                     "timeoutAction": "cancel closure; never trap or teleport a visitor"},
                    {"id": "private-fitting", "next": "collection-only", "trigger": "screen closed",
                     "effects": ["disable invitation entry only", "retain exit", "occlude fitting view"]},
                    {"id": "collection-only", "next": "owner-decision", "trigger": "fitting completes",
                     "playerOptions": ["use counter", "leave toward E4", "leave toward E6", "cancel interaction"]},
                    {"id": "owner-decision", "nextByChoice": {"open": "invitation-open", "closed": "invitation-closed"},
                     "trigger": "owner chooses", "reopenRequired": False},
                    {"id": "invitation-closed", "next": "invitation-open", "trigger": "owner later invites",
                     "publicServiceUnaffected": True}],
                "publicCirculationBlocked": False, "forcedWaitOrPurchase": False,
                "fittingCustomerIdentityVisible": False, "cooldownSecondsTarget": 600,
                "cancelFromAnyState": "release reservations, preserve egress; keep private boundary owner-controlled"}],
            "gates": {"geometry": False, "collision": False, "npc": False, "audio": False,
                      "culturalReview": False, "runtimeRegistration": False}}

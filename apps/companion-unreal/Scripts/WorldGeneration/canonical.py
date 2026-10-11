"""Read pinned references; never evaluate the incomplete TypeScript checkout."""

import hashlib
import json
from pathlib import Path
import re
from urllib.parse import quote

SOURCE_COMMIT = "e978763b74acfb0369b65b879af6a3a8c90b110b"
OWNER = "cribtest11-world-generation-20261010"
SCRIPT_DIR = Path(__file__).resolve().parent
REPO = SCRIPT_DIR.parents[3]
REFERENCE_ROOT = REPO / ".git/operations/world-generation-20261010/readonly-source"
OUTPUT = REPO / "output/cribtest11-world-generation"
NAMESPACE = "/Game/BaddieGeneratedWorld"


def digest(value):
    return hashlib.sha256(json.dumps(value, sort_keys=True, separators=(",", ":"),
                                    allow_nan=False).encode("utf-8")).hexdigest()


def file_hash(path):
    with Path(path).open("rb") as stream:
        value = hashlib.sha256()
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            value.update(block)
    return value.hexdigest()


class References:
    def __init__(self):
        self.lock = json.loads((SCRIPT_DIR / "source-lock.json").read_text(encoding="utf-8"))
        if self.lock["sourceCommit"] != SOURCE_COMMIT:
            raise ValueError("Different source pin; incoming work needs coordinator review")
        self.entries = {item["path"]: item for item in self.lock["files"]}
        self.cache = {}

    def verify(self, manifest_review=None):
        operations = REPO / ".git/operations/world-generation-20261010"
        for filename, key in (("reference-manifest.json", "referenceManifestSha256"),
                              ("reference-receipt.json", "referenceReceiptSha256")):
            if file_hash(operations / filename) != self.lock[key]:
                raise ValueError(f"Changed reference evidence {filename}")
        receipt=json.loads((operations/"reference-receipt.json").read_text(encoding="utf-8-sig"))
        dispatch_hash=receipt["manifestSha256"]
        observed_hash=self.lock["referenceManifestSha256"]
        if dispatch_hash!=observed_hash:
            if (not manifest_review or not manifest_review.get("humanReviewEvidence")
                    or manifest_review.get("observedSha256")!=observed_hash
                    or manifest_review.get("dispatchSha256")!=dispatch_hash):
                raise ValueError("Reference manifest/dispatch receipt discrepancy requires direct human evidence reconciliation before engine work")
        for relative, record in self.entries.items():
            path = REFERENCE_ROOT / relative
            if path.stat().st_size != record["bytes"] or file_hash(path) != record["sha256"]:
                raise ValueError(f"Pinned source changed: {relative}")
        engine=Path(self.lock["engineRoot"])
        for record in self.lock["engineEvidence"]:
            if file_hash(engine/record["path"])!=record["sha256"]:
                raise ValueError("Installed engine API source changed: "+record["path"])

    def text(self, relative):
        record = self.entries[relative]
        path = REFERENCE_ROOT / relative
        if file_hash(path) != record["sha256"]:
            raise ValueError(f"Source hash mismatch {relative}")
        return path.read_text(encoding="utf-8-sig")

    def json(self, relative):
        if relative not in self.cache:
            self.cache[relative] = json.loads(self.text(relative))
        return self.cache[relative]

    def identity(self, relative, pointer=""):
        return {"path": relative, "sha256": self.entries[relative]["sha256"],
                "sourceCommit": SOURCE_COMMIT, "pointer": pointer}


def city_plan(refs):
    return refs.json("packages/assets/city/lagos-city-v3.json")


def geo_point(plan,x,z):
    import math
    return {"latitude":plan["origin"]["latitude"]-z/111320,
            "longitude":plan["origin"]["longitude"]+x/(111320*math.cos(math.radians(plan["origin"]["latitude"])))}


def bank(points,x):
    for a,b in zip(points,points[1:]):
        if x<=b[0]:
            t=max(0,min(1,(x-a[0])/(b[0]-a[0])))
            return a[1]+(b[1]-a[1])*t
    return points[-1][1]


def destinations(refs):
    # Deliberately bounded literal extraction, not a JS evaluator. Hash guards
    # freeze the grammar; an unmatched call or changed count is an error.
    plan = city_plan(refs)
    world_source = "packages/utils/src/companionWorld.ts"
    world = refs.text(world_source).split("export const companionWorldPlaces:", 1)[1].split("...companionPlaceExpansionDestinations", 1)[0]
    pattern = (r'\{ id: "([^"]+)", label: "([^"]+)", kind: "([^"]+)", '
               r'anchor: \{ latitude: ([\d.]+), longitude: ([\d.]+) \}, '
               r'placement: "proposed", seed: "([^"]+)" \}')
    result = [{"id":m[0],"label":m[1],"kind":m[2],
               "anchor":{"latitude":float(m[3]),"longitude":float(m[4])},
               "seed":m[5],"source":refs.identity(world_source, "companionWorldPlaces/" + m[0])}
              for m in re.findall(pattern, world)]
    if len(result) != 11:
        raise ValueError("Base destination extraction is incomplete")
    expansion_source = "packages/utils/src/companionPlacesExpansion.ts"
    expansion = refs.text(expansion_source).split("export const companionPlaceExpansionDestinations:", 1)[1].split("]);", 1)[0]
    parcels = {p["id"]:p for p in plan["parcels"]}
    result.append({"id":"race-circuit","label":"Lagos Circuit","kind":"race-circuit", "seed":"lagos-circuit-v1",
                   "anchor":geo_point(plan,1620,-1366),
                   "city":{"version":plan["version"],"parcelId":"lagos-circuit-site-v1","entrance":[1620,0,-1366]},
                   "source":refs.identity(expansion_source,"companionPlaceExpansionDestinations/race-circuit"),
                   "racingSource":refs.identity("packages/utils/src/companionRacing.ts", "racingRules")})
    import math
    matches = re.findall(r'destination\("([^"]+)", "([^"]+)", "([^"]+)", "([^"]+)"(?:, (-?[\d.]+))?\)', expansion)
    if len(matches) != 16:
        raise ValueError("Expansion destination extraction is incomplete")
    for identifier,label,kind,parcel_id,offset_text in matches:
        parcel = parcels[parcel_id]
        if parcel["kind"] != "shop" or parcel["units"] != 0:
            raise ValueError("A public venue cannot replace Home units")
        offset = float(offset_text or 0)
        entrance = list(parcel["entrance"])
        entrance[0] += math.cos(parcel["yaw"]) * offset
        entrance[2] -= math.sin(parcel["yaw"]) * offset
        result.append({"id":identifier,"label":label,"kind":kind,"seed":identifier + "-v1",
                       "anchor":geo_point(plan,entrance[0],entrance[2]),
                       "city":{"version":plan["version"],"parcelId":parcel_id,"entrance":entrance},
                       "source":refs.identity(expansion_source,"companionPlaceExpansionDestinations/" + identifier)})
    water_source = "packages/utils/src/companionWaterfront.ts"
    water = refs.text(water_source).split("export const companionWaterfrontPlaces:",1)[1].split("];",1)[0]
    water_matches = re.findall(r'place\("([^"]+)", "([^"]+)", "([^"]+)", (-?[\d.]+), (.+?), "(lagoon|creek|ocean)", (true|false)(.*?)\),',water)
    if len(water_matches) != 12:
        raise ValueError("Waterfront destination extraction is incomplete")
    for identifier,label,kind,xtext,ztext,zone,boat,tail in water_matches:
        x = float(xtext)
        if ztext.startswith("south("):
            z = bank(plan["lagoon"]["southBank"],x)-14
        elif ztext.startswith("north("):
            z = bank(plan["lagoon"]["northBank"],x)+16
        else:
            z = float(ztext)
        item = {"id":identifier,"label":label,"kind":kind,"seed":identifier+":lagos-waterfront-v1",
                "anchor":geo_point(plan,x,z),
                "mapPoint":[x,z],"waterZone":zone,"boatOnly":boat=="true",
                "level":"abutment" if '"abutment"' in tail else "pier" if '"pier"' in tail else "ground",
                "source":refs.identity(water_source,"companionWaterfrontPlaces/"+identifier)}
        if "south(" in tail or "north(" in tail or "[" in tail:
            item["cityEntrance"] = [x,0,z]
        result.append(item)
    if len({d["id"] for d in result}) != 40:
        raise ValueError("Canonical world registration differs")
    return result


def registered_asset_version(refs,path,keys):
    source=refs.text(path)
    versions=[]
    for key in sorted(set(keys)):
        matches=re.findall(r'\b'+re.escape(key)+r'\s*:\s*\{[^}]*?sha256:\s*"([a-f0-9]{64})"',source)
        if len(matches)!=1:
            raise ValueError("Unproven canonical asset identity "+key)
        versions.append(key+":"+matches[0])
    return "|".join(versions)


def inspect_place_contract(value,refs):
    """Bind the handoff to the registered seed and this pin's producer branch."""
    place=next((p for p in destinations(refs) if "places/"+p["id"]==value["id"]),None)
    contract=value.get("contract")
    required=("placeId","layoutId","seed","assetVersion","generatorVersion","navigation","entrances","spawns")
    if not place or not isinstance(contract,dict) or not all(key in contract for key in required):
        raise ValueError("Exact registered place layout/navigation/entrance contract required")
    kind=place["kind"]
    if (contract["placeId"]!=place["id"] or contract["seed"]!=place["seed"]
            or contract.get("kind",kind)!=kind or contract["generatorVersion"]!="lagos-places-v1"):
        raise ValueError("Place identity/seed/generator differs from canonical registration")
    if contract.get("housingOption") is not None:
        raise ValueError("Dynamic housing input is unproven; retain the resolved-layout blocker")
    sources={place["source"]["path"]}
    utils="packages/utils/src/"
    assets=utils+"companionAssets.ts"
    prefix=place["id"]+":lagos-places-v1:"
    seed=quote(place["seed"],safe="~()*!.'-")
    if kind in ("civic-interior","pool-courtyard"):
        civic=kind=="civic-interior"
        design_path="packages/assets/venues/"+("civic-interiors-v1.json" if civic else "pool-courtyard-v1.json")
        design=refs.json(design_path)
        if civic and not any(floor["id"]==place["id"] for floor in design["floors"]):
            raise ValueError("Unregistered authored Civic floor")
        sources.update((design_path,utils+("companionPlacesCivicInteriors.ts" if civic else "companionPlacesPoolCourtyard.ts")))
        asset_version=design["id"]
        layout_id=prefix+asset_version+":"+seed
    elif kind=="race-circuit":
        producer=utils+"companionRacing.ts"
        rules=re.search(r'export const racingRules = Object.freeze\(\{\s*placeId: "([^"]+)", version: "([^"]+)"',refs.text(producer))
        if not rules or rules[1]!=place["id"]:
            raise ValueError("Unproven canonical racing identity")
        sources.add(producer)
        asset_version=rules[2]
        layout_id=asset_version+":"+place["seed"]
    elif "waterZone" in place:
        producer=utils+"companionWaterfrontLayouts.ts"
        boats=utils+"companionBoatAssets.ts"
        version=re.search(r'export const COMPANION_WATERFRONT_VERSION = "([^"]+)";',refs.text(utils+"companionWaterfront.ts"))
        if not version:
            raise ValueError("Unproven canonical waterfront version")
        # These fixed selections project the pinned producer's branches, not
        # caller-supplied props. Unrecognized/dynamic branches stay blocked.
        if kind=="surf-island":
            boat_keys=["bestieBoatPassengerSpeedboat"]
        elif kind=="stilt-creek":
            boat_keys=["bestieBoatDugoutCanoe","bestieBoatPlankedCanoe","bestieBoatFishingSkiff"]
        elif kind=="timber-atoll":
            boat_keys=["bestieBoatSandDredger","bestieBoatTimberBarge","bestieBoatDugoutCanoe","bestieBoatPassengerSpeedboat"]
        elif kind=="under-bridge":
            boat_keys=["bestieBoatDugoutCanoe"] if place["level"]=="pier" else []
        elif kind=="water-terminal":
            boat_keys=["bestieBoatPassengerSpeedboat","bestieBoatSandDredger"]
            if place["id"]=="ikoyi-marina":
                boat_keys += ["bestieBoatCabinYacht","bestieBoatFlybridgeYacht","bestieBoatOpenSpeedboat",
                              "bestieBoatInflatableTender","bestieBoatJetSki"]
        else:
            raise ValueError("Unproven dynamic waterfront assets; retain the resolved-layout blocker")
        sources.update((producer,boats,assets))
        asset_version=version[1]+"|"+registered_asset_version(refs,assets,["bestieTownSquareNpcs"])
        if boat_keys:
            asset_version+="|"+registered_asset_version(refs,boats,boat_keys)
        layout_id=prefix+version[1]+":"+seed
    else:
        expansion=("market","supermarket","bus-terminal","fuel-station","mechanic","salon",
                   "gym","cinema","library","arts-centre","campus","office")
        base=("hospital","mama-put","content-house","skyline","fine-dining","nightclub","station","soccer-pitch","arena","lounge")
        if kind not in base+expansion:
            raise ValueError("Unproven dynamic place inputs; retain the resolved-layout blocker")
        sources.add(utils+"companionPlacesLayouts.ts")
        keys=[] if kind=="lounge" else ["bestieTownSquareNpcs"]
        if kind in ("hospital","content-house","skyline","fine-dining"):
            keys.append("bestiePatStudio")
            sources.add(utils+"companionRoomLayouts.ts")
        if kind=="mama-put":
            keys.append("bestieKitchen")
        if kind in expansion:
            sources.add(utils+"companionPlacesExpansionLayouts.ts")
        if kind=="lounge":
            sources.add("packages/assets/venues/dance-again-v1.json")
        if keys:
            sources.add(assets)
        asset_version=registered_asset_version(refs,assets,keys) if keys else "geometry-v1"
        layout_id=prefix+kind+":default:"+seed
    navigation_revision=layout_id if kind=="race-circuit" else layout_id+":"+asset_version
    if (contract["layoutId"]!=layout_id or contract["assetVersion"]!=asset_version
            or not isinstance(contract["navigation"],dict)
            or contract["navigation"].get("revision")!=navigation_revision):
        raise ValueError("Place layout/asset/navigation identity differs from pinned producer")
    supplied=set()
    for source in value.get("sources",[]):
        path=source.get("path")
        if (path not in refs.entries or source.get("sha256")!=refs.entries[path]["sha256"]
                or source.get("sourceCommit",SOURCE_COMMIT)!=SOURCE_COMMIT):
            raise ValueError("Place source differs from pinned reference")
        supplied.add(path)
    if not sources.issubset(supplied):
        raise ValueError("Place handoff lacks canonical registration/producer sources: "+", ".join(sorted(sources-supplied)))
    for path in sources:
        refs.text(path)


def travel_contract(refs):
    """Retain proposed corridors and boat/level exclusions, never grant travel."""
    import math
    plan=city_plan(refs)
    places=destinations(refs)
    water={p["id"]:p for p in places if "waterZone" in p}
    def length(points):
        total=0
        for a,b in zip(points,points[1:]):
            lat=math.radians(b["latitude"]-a["latitude"])
            lng=math.radians(b["longitude"]-a["longitude"])
            h=math.sin(lat/2)**2+math.cos(math.radians(a["latitude"]))*math.cos(math.radians(b["latitude"]))*math.sin(lng/2)**2
            total+=6371000*2*math.atan2(math.sqrt(h),math.sqrt(max(0,1-h)))
        return total
    routes=[]
    for a in places:
        for b in places:
            if a["id"]==b["id"] or a.get("level")=="abutment" or b.get("level")=="abutment":
                continue
            wa,wb=water.get(a["id"]),water.get(b["id"])
            boat=None
            if wa and wb and not (wa["kind"]=="under-bridge" and wa["level"]!="pier" or wb["kind"]=="under-bridge" and wb["level"]!="pier"):
                creek=wa["waterZone"]=="creek" or wb["waterZone"]=="creek"
                if not creek or "okobaba-timber" in (wa["id"],wb["id"]):
                    boat="canoe" if creek else "speedboat"
                    x=(wa["mapPoint"][0]+wb["mapPoint"][0])/2
                    lane=(bank(plan["lagoon"]["southBank"],x)+bank(plan["lagoon"]["northBank"],x))/2
                    points=[a["anchor"],geo_point(plan,wa["mapPoint"][0],lane),geo_point(plan,wb["mapPoint"][0],lane),b["anchor"]]
            if boat is None:
                if a.get("boatOnly") or b.get("boatOnly"):
                    continue
                raw=[a["anchor"],{"latitude":a["anchor"]["latitude"],"longitude":3.4101},
                     {"latitude":b["anchor"]["latitude"],"longitude":3.4101},b["anchor"]]
                points=[p for i,p in enumerate(raw) if i==0 or p!=raw[i-1]]
            route={"id":a["id"]+":"+b["id"],"from":a["id"],"to":b["id"],"version":"lagos-corridors-v1",
                   "placement":"proposed","points":points,"lengthMetres":length(points),"boat":boat}
            if boat:
                route["durationMs"]=max(8000,math.ceil(route["lengthMetres"]/(2.2 if boat=="canoe" else 12)*1000))
            routes.append(route)
    return {"worldId":"lagos-v1","destinations":places,"routes":routes,
            "levelTransitions":[{"from":"obalende-underbridge","to":"obalende-abutment","service":"waterfront:level:obalende-abutment"},
                                {"from":"obalende-abutment","to":"obalende-underbridge","service":"waterfront:level:obalende-underbridge"}],
            "sources":[refs.identity("packages/utils/src/companionWorld.ts"),refs.identity("packages/utils/src/companionWaterfront.ts"),
                       refs.identity("packages/utils/src/companionJourney.ts")],"admission":"backend-authoritative-closed"}


def require_handoff(path, expected_sha256, refs):
    """Caller supplies an independently approved hash, never trust self-declared hashes."""
    path = Path(path).resolve()
    if not re.fullmatch(r"[a-f0-9]{64}", expected_sha256 or "") or file_hash(path) != expected_sha256:
        raise ValueError("Missing or mismatched approved handoff hash")
    value = json.loads(path.read_text(encoding="utf-8-sig"))
    if value.get("sourceCommit") != SOURCE_COMMIT or value.get("schemaVersion") != 1:
        raise ValueError("Incompatible incoming handoff")
    for record in value.get("sources", []):
        if record.get("sha256") != refs.entries[record["path"]]["sha256"]:
            raise ValueError("Incoming reference does not match pin")
    if not value.get("sources"):
        raise ValueError("Unprovenanced handoff")
    return value

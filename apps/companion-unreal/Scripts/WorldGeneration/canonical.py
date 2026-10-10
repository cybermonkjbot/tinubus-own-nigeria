"""Read pinned references; never evaluate the incomplete TypeScript checkout."""

import hashlib
import json
from pathlib import Path
import re

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
    def bank(points, x):
        for a,b in zip(points,points[1:]):
            if x <= b[0]:
                t = max(0,min(1,(x-a[0])/(b[0]-a[0])))
                return a[1] + (b[1]-a[1])*t
        return points[-1][1]
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


def travel_contract(refs):
    """Retain proposed corridors and boat/level exclusions, never grant travel."""
    import math
    plan=city_plan(refs)
    places=destinations(refs)
    water={p["id"]:p for p in places if "waterZone" in p}
    def bank(points,x):
        for a,b in zip(points,points[1:]):
            if x<=b[0]:
                return a[1]+(b[1]-a[1])*max(0,min(1,(x-a[0])/(b[0]-a[0])))
        return points[-1][1]
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

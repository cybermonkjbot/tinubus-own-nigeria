"""Source-derived build jobs; blocked areas never receive invented geometry."""

from collections import defaultdict
from copy import deepcopy
import math
from urllib.parse import quote

from .canonical import city_plan, destinations, digest, travel_contract
from .geometry import (beam_mesh, home_point, home_shell_point, part_vertices,
                      place_point, ribbon, triangles)

CITY = "packages/assets/city/lagos-city-v3.json"


def job(identifier, sources, parts=None, meshes=None, blockers=None, category="city"):
    return {"id":identifier,"category":category,"sources":sources,
            "parts":parts or [],"meshes":meshes or [],"blockers":blockers or [],
            "authority":{"access":False,"movementAdmission":False,"missionReadiness":False,"utilityPower":False}}


def explicit(identifier, vertices, faces, material, collision="none"):
    return {"id":identifier,"vertices":vertices,"faces":faces,"material":material,"collision":collision}


def box_part(identifier, position, size, colour, yaw=0):
    return {"id":identifier,"shape":"box","position":position,"size":size,
            "rotation":[0,yaw,0],"material":{"colour":colour,"roughness":.86,"metalness":0},"collision":"none"}


def authored_parts(parts, materials, family, position=(0,0,0), yaw=0, floor_y=0):
    result=[]
    for index,part in enumerate(parts):
        material=materials[part["material"]]
        if isinstance(material,str):
            material={"colour":material,"roughness":.86,"metalness":0}
        material={**material, "colour":part.get("colour",material.get("colour","#ffffff")),"emission":0}
        # Bake full local XYZ Euler/nonuniform scale, then parent yaw. This avoids
        # nonuniform-parent quaternion loss and retains the curved/open meshes.
        vertices,faces=part_vertices(part,family)
        vertices=[place_point([p[0],p[1]-floor_y,p[2]],position,yaw) for p in vertices]
        result.append(explicit(part.get("name","part")+":"+str(index),vertices,faces,material))
    return result


def venue_contract(design, place_id, seed, grid_size, floor_y=0):
    local=lambda p:[p[0],p[1]-floor_y,p[2]]
    identifier=f"{place_id}:lagos-places-v1:{design['id']}:{quote(seed,safe='')}"
    floor=next((p for p in design.get("floors",[]) if p["id"]==place_id),design)
    footprints=[{"id":p["id"],"minX":p["x"]-p["width"]/2,"maxX":p["x"]+p["width"]/2,
                 "minZ":p["z"]-p["depth"]/2,"maxZ":p["z"]+p["depth"]/2,"height":p["height"]}
                for p in floor["footprints"]]
    bounds=floor.get("bounds") or {"minX":-design["size"][0]/2,"maxX":design["size"][0]/2,
                                  "minZ":-design["size"][2]/2,"maxZ":design["size"][2]/2}
    return {"placeId":place_id,"seed":seed,"layoutId":identifier,"assetVersion":design["id"],
            "generatorVersion":"lagos-places-v1","assemblyFloorY":floor_y,
            "navigation":{"revision":identifier+":"+design["id"],"floorY":0,"radius":.24,
                          "gridSize":grid_size,"bounds":bounds,"obstacles":footprints},
            "entrances":[{"id":"world-entry","point":local(floor["entrance"]["point"]),
                          "outside":local(floor["entrance"]["outside"]),"width":floor["entrance"]["width"],
                          "role":"world","access":"place-membership"}],
            "spawns":[{"point":local(p),"yaw":math.pi} for p in floor["spawns"]],
            "rooms":floor.get("rooms",[]),"services":[],"npcs":[],"gates":"runtime-authoritative-closed"}


def home_contract(base,turns,extension):
    value=deepcopy(base)
    value["id"]=f"{base['id']}:home-v1:{turns}:{extension:g}"
    def at(record):
        x,_,z=home_point([record["x"],0,record["z"]],turns)
        return {**record,"x":x,"z":z}
    def box(record):
        result=at(record)
        if turns%2:
            result["width"],result["depth"]=record["depth"],record["width"]
        return result
    bounds={**base["bounds"],"maxZ":base["bounds"]["maxZ"]+extension}
    a=home_point([bounds["minX"],0,bounds["minZ"]],turns)
    b=home_point([bounds["maxX"],0,bounds["maxZ"]],turns)
    value["bounds"]={"minX":min(a[0],b[0]),"maxX":max(a[0],b[0]),
                     "minZ":min(a[2],b[2]),"maxZ":max(a[2],b[2])}
    value["obstacles"]=[box(p) for p in base["obstacles"]]
    if "floorPatches" in base:
        value["floorPatches"]=[box(p) for p in base["floorPatches"]]
    standing=deepcopy(base["standingCandidates"])
    if extension:
        standing += [{"x":x,"z":2+extension/2,"yaw":math.pi} for x in (-1.6,-.8,0,.8,1.6)]
    value["standingCandidates"]=[{**at(p),"yaw":p["yaw"]+turns*math.pi/2} for p in standing]
    value["surfaces"]=[]
    for surface in base["surfaces"]:
        expanded={**surface,"z":surface["z"]+extension/2,"depth":surface["depth"]+extension} if surface["id"]=="floor" else surface
        transformed={**box(expanded),"anchors":[at(a) for a in surface["anchors"]]}
        if surface.get("clearance"):
            transformed["clearance"]=box(surface["clearance"])
        value["surfaces"].append(transformed)
    value["walls"]=[]
    for wall in base.get("walls",[]):
        transformed=at({**wall,"z":wall["z"]+extension/2} if wall["id"]=="wall-left" and extension else wall)
        transformed.update(width=wall["width"]+(extension if wall["id"]=="wall-left" else 0),yaw=wall["yaw"]+turns*math.pi/2)
        value["walls"].append(transformed)
    return value


def prepare_home_handoff(identifier,incoming,refs):
    segments=identifier.split("/")
    authored_base=len(segments)==3 and segments[2]=="authored"
    if not authored_base and (len(segments)!=4 or not segments[2].startswith("turn-") or not segments[3].startswith("extension-")):
        # Legacy, terrace, housing and apartment hallway are independent shells.
        contract=incoming.get("contract",{})
        required=("layoutId","navigation","entrances","shellOnly","savedItems","roomSelection","layoutRevision")
        if (not all(key in contract for key in required) or contract["shellOnly"] is not True
                or contract["savedItems"]!="runtime-authoritative-not-baked"
                or contract["roomSelection"]!="runtime-authoritative"
                or contract["layoutRevision"]!="runtime-authoritative"
                or not contract["entrances"] or not contract["navigation"]):
            raise ValueError("Independent Home shell needs exact layout/navigation/opening and runtime ownership contracts")
        if any(mesh.get("role")!="shell" for mesh in incoming.get("meshes",[])):
            raise ValueError("Independent Home shells cannot bake private saved furniture")
        return incoming
    layout=segments[1]
    turns,extension=(0,0) if authored_base else (int(segments[2][5:]),float(segments[3][10:]))
    if turns not in range(4) or extension not in (0,.6,1.2) or extension and layout not in ("modern-kitchen-v01","bathroom-v01"):
        raise ValueError("Invalid Home variant")
    shell=incoming.get("homeShell")
    if not shell or shell.get("layoutId")!=layout or shell.get("shellOnly") is not True or shell.get("coordinates")!="runtime-y-up-metres":
        raise ValueError("Home needs approved shell-only runtime-metre geometry; never a furnished account snapshot")
    base=shell["baseLayout"]
    if (base.get("id")!=layout or not shell.get("assetSha256")
            or not all(key in base for key in ("bounds","obstacles","surfaces","standingCandidates"))
            or not isinstance(shell.get("sourceFloorYMetres"),(int,float))
            or not math.isfinite(shell["sourceFloorYMetres"])):
        raise ValueError("Home layout/asset identity missing")
    registry={"pat-studio-v01":"bestiePatStudio","pink-bedroom-v01":"bestiePinkBedroom",
              "pink-bedroom-v02":"bestiePinkBedroomV2","modern-kitchen-v01":"bestieKitchen","bathroom-v01":"bestieBathroom"}
    assets=refs.text("packages/utils/src/companionAssets.ts")
    import re
    record=re.search(r"\b"+registry[layout]+r"\s*:\s*\{([^}]+)\}",assets)
    expected=re.search(r'sha256:\s*"([a-f0-9]+)"',record.group(1)).group(1)
    if shell["assetSha256"]!=expected:
        raise ValueError("Home shell does not derive from exact registered room art")
    if layout=="pink-bedroom-v01" and shell.get("unpublishedAssetApproval") is not True:
        raise ValueError("Unpublished v01 room art requires direct source delivery/approval")
    transformed=deepcopy(incoming)
    transformed["meshes"]=[]
    for mesh in incoming.get("meshes",[]):
        if mesh.get("role")!="shell":
            raise ValueError("Do not bake saved-account furniture or appearance into reusable shells")
        transformed["meshes"].append({**mesh,"vertices":[home_shell_point(p,turns,extension) for p in mesh["vertices"]]})
    if not transformed["meshes"] or incoming.get("template") or incoming.get("parts"):
        raise ValueError("Authored Home variants need explicit shell vertices, not untransformed template geometry")
    authored=layout in ("pat-studio-v01","pink-bedroom-v01","pink-bedroom-v02")
    door={"authored":authored}
    if authored:
        floor_y=shell["sourceFloorYMetres"]
        point=[-1.74,floor_y,-2.6] if layout=="pink-bedroom-v02" else [-1.6891,floor_y,.6931]
        approach=[-1.74,floor_y,-2.25] if layout=="pink-bedroom-v02" else [-1.34,floor_y,.6931]
        door.update(doorway=home_point(point,turns),preferredApproach=home_point(approach,turns))
    else:
        contract=incoming.get("contract",{})
        if (contract.get("nativeOpeningsVariant")!=identifier or not contract.get("nativeOpenings")
                or contract.get("openingCoordinates")!="runtime-y-up-metres-exact-selected-variant"):
            raise ValueError("Unmeasured Home needs real native openings for the exact selected shell variant")
        door["nativeOpenings"]=deepcopy(contract["nativeOpenings"])
        door["sourceMeasured"]=False
    transformed["contract"]={**deepcopy(incoming.get("contract",{})),
                             "roomLayout":deepcopy(base) if authored_base else home_contract(base,turns,extension),"door":door,
                             "savedRoomIds":"runtime-authoritative","savedItems":"runtime-authoritative-not-baked",
                             "appearance":"runtime-authoritative","roomSelection":"runtime-authoritative",
                             "navigationRevision":"runtime-authoritative; preserve resolved roomNavigationFor revision",
                             "homeDesign":None if authored_base else {"version":1,"turns":turns,"extension":extension}}
    return transformed


def build_jobs(refs, handoffs=None):
    handoffs=handoffs or {}
    plan=city_plan(refs)
    architecture_path="packages/assets/city/lagos-architecture-delivery-v1.json"
    architecture=refs.json(architecture_path)
    reserved={p["parcelId"] for p in architecture["buildings"]} | {"civic-street-2",architecture["exterior"]["parcelId"]}
    palette={
        "town-centre":["#d9cdb7","#e8dfce","#b5b6ad","#c5aa94","#cbc8ba","#eee5d8"],
        "palm-courts":["#d4c28f","#dfd2ae","#a6b69b","#cfad97","#ddd6bf","#b8bdac"],
        "garden-quarter":["#e5e1d1","#bbc5ac","#cfc0ac","#e0d1b6","#c2cbbb","#d5bfa9"],
        "south-market":["#b98762","#c6ab80","#d4c198","#ad9d85","#c5b29b","#cba480"],
        "marina":["#c3c4b5","#cfd6cf","#aab9b7","#cebaa6","#e2ddd0","#a9b9bf"],
        "skyline":["#b8cacf","#d5d9d0","#9aaeb2","#dedbd0","#b4c0b8","#b9b9ad"],
        "lagoon-gardens":["#efe9db","#d9d2be","#e9e2d1","#cfcec0","#e0d8c8","#c6d0c5"]}
    districts=defaultdict(list)
    for parcel in plan["parcels"]:
        if parcel["id"] in reserved:
            continue
        w,d,h=parcel["width"],parcel["depth"],parcel["floors"]*3.3
        tint=palette[parcel["district"]][parcel["variant"]]
        parts=districts[parcel["district"]]
        def add(x,y,z,mw,mh,md,colour,shape="box"):
            p=place_point([x,y,z],[parcel["x"],0,parcel["z"]],parcel["yaw"])
            part=box_part(parcel["id"]+":"+str(len(parts)),p,[mw,mh,md],colour,parcel["yaw"])
            part["shape"],part["parcelId"]=shape,parcel["id"]
            parts.append(part)
        def mass(x,z,mw,md,mh,base=0):
            add(x,base+mh/2,z,mw,mh,md,tint)
            add(x,base+mh+.16,z,mw+.4,.32,md+.4,"#b5ac97" if parcel["district"]=="lagoon-gardens" else "#828c84")
        style=parcel["style"]
        if style=="stepped-tower":
            mass(0,0,w,d,9.9)
            mass((1 if parcel["variant"]%2 else -1)*w*.08,-d*.06,w*.68,d*.72,h-9.9,9.9)
            mass(w*.05,-d*.09,w*.4,d*.43,6.6,h)
        elif style=="courtyard":
            mass(-w*.35,0,w*.3,d,h); mass(w*.35,0,w*.3,d,h)
            mass(0,-d*.34,w*.4,d*.32,max(6.6,h-3.3))
        elif style in ("terrace-villa","courtyard-villa"):
            mass(-w*.18,0,w*.64,d,h);mass(w*.32,-d*.2,w*.36,d*.6,h-3.3)
            add(w*.32,h-3.1,-d*.2,w*.36,.18,d*.6,"#d7c7a7")
        elif style in ("urban-apartment","waterfront"):
            mass(-w*.08,0,w*.84,d,h);mass(w*.4,-d*.12,w*.2,d*.76,max(6.6,h-3.3))
        else:
            mass(0,0,w,d,h)
            if style in ("townhouse","garden-home","warehouse","workshop"):
                add(0,h+1.65,0,w+1,3,d+1,"#697b7c" if style=="warehouse" else "#8b5e45" if parcel["variant"]%2 else "#777669","city-gable")
    jobs=[job("city/districts/"+identifier,[refs.identity(CITY),refs.identity("packages/companion-scene/src/interactions/city/cityScene.ts")],parts)
          for identifier,parts in sorted(districts.items())]
    for building in architecture["buildings"]:
        parcel=next(p for p in plan["parcels"] if p["id"]==building["parcelId"])
        if (building["position"] != [parcel["x"],0,parcel["z"]] or building["yaw"] != parcel["yaw"]
                or building["entrance"] != parcel["entrance"] or building["units"] != parcel["units"]):
            raise ValueError("Architecture must retain canonical parcel contract")
        jobs.append(job("city/signatures/"+building["parcelId"],[refs.identity(architecture_path)],meshes=authored_parts(
            building["parts"],architecture["materials"],"signature",building["position"],building["yaw"])))
    exterior=architecture["exterior"]
    parcel=next(p for p in plan["parcels"] if p["id"]==exterior["parcelId"])
    jobs.append(job("city/dance-again-exterior",[refs.identity(architecture_path)],meshes=authored_parts(
        exterior["parts"],exterior["materials"],"signature",[parcel["x"],0,parcel["z"]],parcel["yaw"])))
    civic_path="packages/assets/venues/civic-terrace-v1.json"
    civic=refs.json(civic_path)
    parcel=next(p for p in plan["parcels"] if p["id"]==civic["parcelId"])
    jobs.append(job("city/civic-terrace-exterior",[refs.identity(civic_path),refs.identity(CITY)],meshes=authored_parts(
        civic["parts"],civic["materials"],"signature",[parcel["x"],0,parcel["z"]],parcel["yaw"])))
    # Exact road geometry. Current divided-road/surface/paint and park path
    # derivatives are separate handoff jobs, not silently treated as implemented.
    meshes=[]
    for road in plan["roads"]:
        if road["kind"]=="retained":
            continue
        ribbons=(("paving",road["width"]+road["pavement"]*2,-.02,"#c6b898"),
                 ("surface",road["width"],-.012,"#525751"))
        if road["kind"] in ("path","sand-path","boardwalk"):
            ribbons=(("surface",road["width"],-.008,"#ac9673" if road["kind"]=="boardwalk" else "#c4af86" if road["kind"]=="sand-path" else "#c6b898"),)
        for suffix,width,y,colour in ribbons:
            vertices,faces=ribbon(road["points"],width,y)
            meshes.append(explicit(road["id"]+":"+suffix,vertices,faces,{"colour":colour,"roughness":.9,"metalness":0}))
    jobs.append(job("city/road-ribbons",[refs.identity(CITY)],meshes=meshes))
    landmark_meshes=[]
    for index,p in enumerate(plan["landmarkPieces"]):
        identity=p["id"]+":"+str(index)
        part=box_part(identity,[p["x"],p["y"],p["z"]],[p["w"],p["h"],p["d"]],p["colour"],p.get("yaw",0))
        v,f=part_vertices(part)
        landmark_meshes.append(explicit(identity,v,f,part["material"]))
    for index,b in enumerate(plan["landmarkBeams"]):
        v,f=beam_mesh(b["from"],b["to"],b["width"])
        landmark_meshes.append(explicit(b["id"]+":"+str(index),v,f,{"colour":b["colour"],"roughness":.86,"metalness":0}))
    jobs.append(job("city/landmarks",[refs.identity(CITY)],meshes=landmark_meshes))
    theatre_path="packages/assets/city/lagos-national-theatre-geometry-v1.json"
    theatre=refs.json(theatre_path)
    def theatre_material(key):
        return {"colour":theatre["materials"][key],"roughness":.22 if key=="glass" else .86,
                "metalness":.3 if key=="glass" else .2 if key=="trim" else 0,"twoSided":True}
    theatre_meshes=[explicit(p["name"]+":part:"+str(index),p["vertices"],triangles(p["faces"]),theatre_material(p["material"])) for index,p in enumerate(theatre["parts"])]
    for index,p in enumerate(theatre["boxes"]):
        part=box_part(p["name"]+":box:"+str(index),[p["x"],p["y"],p["z"]],[p["w"],p["h"],p["d"]],theatre["materials"][p["material"]],p.get("yaw",0))
        v,f=part_vertices(part)
        theatre_meshes.append(explicit(part["id"],v,f,theatre_material(p["material"])))
    for index,b in enumerate(theatre["beams"]):
        v,f=beam_mesh(b["a"],b["b"],b["width"],b["depth"])
        theatre_meshes.append(explicit(b["name"]+":beam:"+str(index),v,f,theatre_material(b["material"])))
    jobs.append(job("city/national-theatre",[refs.identity(theatre_path)],meshes=theatre_meshes))
    interior_path="packages/assets/venues/civic-interiors-v1.json"
    interiors=refs.json(interior_path)
    for floor in interiors["floors"]:
        value=job("places/"+floor["id"],[refs.identity(interior_path),refs.identity("packages/utils/src/companionPlacesCivicInteriors.ts")],meshes=authored_parts(
            floor["parts"],interiors["materials"],"civic",floor_y=floor["floorY"]),category="interior")
        value["contract"]=venue_contract(interiors,floor["id"],floor["id"]+"-v1",.25,floor["floorY"])
        jobs.append(value)
    pool_path="packages/assets/venues/pool-courtyard-v1.json"
    pool=refs.json(pool_path)
    meshes=authored_parts([p for p in pool["parts"] if p["shape"]!="water"],pool["materials"],"pool")
    water={"shape":"water","position":[pool["pool"]["x"],pool["pool"]["waterY"],pool["pool"]["z"]],
           "size":[pool["pool"]["width"],1,pool["pool"]["depth"]],"rotation":[0,0,0],"material":"water","name":"pool-water"}
    meshes += authored_parts([water],pool["materials"],"pool")
    value=job("places/pool-courtyard",[refs.identity(pool_path),refs.identity("packages/utils/src/companionPlacesPoolCourtyard.ts")],meshes=meshes,category="interior")
    value["contract"]=venue_contract(pool,"pool-courtyard","pool-courtyard-v1",.3)
    value["contract"]["pool"]=pool["pool"]
    jobs.append(value)
    supplied=set(j["id"] for j in jobs)
    for destination in destinations(refs):
        identifier="places/"+destination["id"]
        if destination["id"] in ("town-square","own-home") or identifier in supplied:
            continue
        jobs.append(job(identifier,[destination["source"]],blockers=["Resolved pinned layout and native scenery handoff required"],category="waterfront" if "waterZone" in destination else "place"))
    for layout in ("pat-studio-v01","pink-bedroom-v01","pink-bedroom-v02","modern-kitchen-v01","bathroom-v01"):
        jobs.append(job(f"home/{layout}/authored",[refs.identity("packages/utils/src/companionRoomLayouts.ts")],
            blockers=["Reusable shell-only geometry and exact base layout required; missing homeDesign retains original revision"],category="Home"))
        for turns in range(4):
            for extension in ((0,.6,1.2) if layout in ("modern-kitchen-v01","bathroom-v01") else (0,)):
                identifier=f"home/{layout}/turn-{turns}/extension-{extension:g}"
                jobs.append(job(identifier,[refs.identity("packages/utils/src/companionRoomLayouts.ts"),refs.identity("packages/utils/src/companionHomeDesign.ts")],
                    blockers=["Reusable shell-only geometry and resolved layout handoff required; no saved-account furniture"],category="Home"))
    for identifier in ("legacy-studio","terrace","private-hallway","shared-living-room"):
        jobs.append(job("home/"+identifier,[refs.identity("docs/engineering/standalone-companion-apartment.md")],blockers=["Approved reusable native shell and doorway handoff required"],category="Home"))
    for option in ("room","compound","apartment","large-home"):
        jobs.append(job("home/housing/"+option,[refs.identity("packages/utils/src/companionPlacesLayouts.ts", "housing")],
            blockers=["Exact resolved canonical housing layout required; saved-room visit grants remain runtime-owned"],category="Home"))
    for identifier in ("expressway","damaged-incident","bush-paths-and-gullies","river","fortified-camp"):
        jobs.append(job("rural/"+identifier,[refs.identity("docs/engineering/standalone-companion-rural-expedition.md")],blockers=["Approved metric rural terrain, layout and initialized native Landscape/template required"],category="rural"))
    for identifier in ("terrain-and-beaches","parks-and-amenities","theatre-grounds","refinery","race-circuit-city","retained-town-square","road-treatments-and-street-density","frontages-and-billboards","transit-visuals","native-navigation","outdoor-lighting","streamed-world-composition"):
        jobs.append(job("city/"+identifier,[refs.identity(CITY)],blockers=["Pinned derived scenery/asset contract and native recipe handoff required"],category="city"))
    # Handoffs supply ordinary geometry/native template recipes. Never JS, scene
    # authority, fabricated service grants, or alternative runtime implementations.
    for value in jobs:
        incoming=handoffs.get(value["id"])
        if incoming:
            if not value["blockers"]:
                raise ValueError("Incoming handoff conflicts with an implemented canonical job")
            if incoming.get("authority") != value["authority"]:
                raise ValueError("Scene handoff cannot grant runtime authority")
            if value["category"]=="Home":
                incoming=prepare_home_handoff(value["id"],incoming,refs)
            if not incoming.get("sources") or not (incoming.get("meshes") or incoming.get("template")):
                raise ValueError("Empty or unprovenanced native handoff")
            if incoming["id"]!=value["id"] or incoming.get("category",value["category"])!=value["category"]:
                raise ValueError("Incoming canonical job identity changed")
            value.update({key:incoming[key] for key in ("meshes","parts","template","contract","sources") if key in incoming})
            value["blockers"]=[]
        if value["id"].startswith("city/"):
            value.setdefault("contract",{})["navigationSource"]=refs.identity("packages/assets/city/lagos-street-density-navigation-v1.json","navigation")
            value["contract"]["navigationRevision"]="lagos-city-v3:street-density-v1"
            value["contract"]["coordinateMapping"]="UE(X,Y,Z)=100*source(Z,X,Y); yawDegrees=sourceYawRadians*180/pi"
            value["contract"]["fictionalMetricPlan"]=True
        value["recipeSha256"]=digest(value)
    # Travel data is retained as a contract, not actors or local authority.
    composition=next(value for value in jobs if value["id"]=="city/streamed-world-composition")
    composition.setdefault("contract",{})["travel"]=travel_contract(refs)
    composition["recipeSha256"]=digest({key:value for key,value in composition.items() if key!="recipeSha256"})
    return jobs

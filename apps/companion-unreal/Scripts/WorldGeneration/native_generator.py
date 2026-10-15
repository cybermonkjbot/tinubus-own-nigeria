"""Run only inside the authorized, PID-inspected UE 5.8.3 editor.

Creates native static meshes/materials, persistent HISM components and .umap
documents. Does not launch the editor, compile gameplay, build lighting/nav,
set startup maps, open admission gates, or manufacture incomplete-area maps.
"""

from collections import defaultdict
from contextlib import contextmanager
from copy import deepcopy
import hashlib
import json
import math
import os
from pathlib import Path
import re
import time

from .canonical import (NAMESPACE, OUTPUT, OWNER, REPO, SCRIPT_DIR, SOURCE_COMMIT,
                       References, digest, file_hash, inspect_place_contract, require_handoff)
from .geometry import obj_text, shape_mesh, ue_point
from .recipes import build_jobs, inspect_content


def owned_output(relative):
    path=(OUTPUT / relative).resolve()
    if not path.is_relative_to(OUTPUT.resolve()):
        raise ValueError("Output path escaped worker ownership")
    return path


def atomic_json(path, value):
    path=Path(path).resolve()
    if not path.is_relative_to(OUTPUT.resolve()):
        raise ValueError("Journal must stay in worker output")
    path.parent.mkdir(parents=True,exist_ok=True)
    temp=path.with_suffix(path.suffix+f".{os.getpid()}.tmp")
    with temp.open("x",encoding="utf-8",newline="\n") as stream:
        json.dump(value,stream,indent=2,allow_nan=False)
        stream.write("\n")
        stream.flush();os.fsync(stream.fileno())
    os.replace(temp,path)


def save_immutable_completion(path,value):
    path=Path(path).resolve()
    if not path.is_relative_to(OUTPUT.resolve()):
        raise ValueError("Completion evidence must stay in worker output")
    path.parent.mkdir(parents=True,exist_ok=True)
    # Never replace completion evidence. A crash during this exclusive write
    # leaves a partial receipt that the next attempt holds for recovery review.
    with path.open("x",encoding="utf-8",newline="\n") as stream:
        json.dump(value,stream,indent=2,allow_nan=False)
        stream.write("\n")
        stream.flush();os.fsync(stream.fileno())


def read_completion(path,expected_sha256=None):
    try:
        raw=path.read_bytes()
        if expected_sha256 is not None and hashlib.sha256(raw).hexdigest()!=expected_sha256:
            raise ValueError("Completion bytes changed")
        value=json.loads(raw.decode("utf-8"))
        if not isinstance(value,dict):
            raise ValueError("Completion is not an object")
        return value
    except (OSError,UnicodeError,ValueError) as error:
        raise RuntimeError("Missing or invalid completion evidence; explicit recovery review required") from error


def batch_identity(value):
    return digest({key:value[key] for key in ("owner","sourceCommit","authorizationSha256","generatorIdentity","jobIdentities")})


def world_completion_path(identity):
    if not isinstance(identity,str) or not re.fullmatch(r"[a-f0-9]{64}",identity):
        raise ValueError("Malformed world completion identity")
    return owned_output("execution/completed-worlds/"+identity+".json")


def completed_world(identity):
    # Locate the original immutable batch independently of the mutable journal.
    reference=read_completion(world_completion_path(identity))
    try:
        if (set(reference)!={"schemaVersion","owner","sourceCommit","identity","batchIdentity","batchSha256"}
                or type(reference["schemaVersion"]) is not int or reference["schemaVersion"]!=1 or reference["owner"]!=OWNER
                or reference["sourceCommit"]!=SOURCE_COMMIT or reference["identity"]!=identity
                or not re.fullmatch(r"[a-f0-9]{64}",reference["batchIdentity"])
                or not re.fullmatch(r"[a-f0-9]{64}",reference["batchSha256"])):
            raise ValueError("Conflicting world completion reference")
        batch=read_completion(owned_output("execution/batch-"+reference["batchIdentity"]+".json"),reference["batchSha256"])
        if (batch_identity(batch)!=reference["batchIdentity"] or batch["owner"]!=OWNER
                or batch["sourceCommit"]!=SOURCE_COMMIT or batch.get("state")!="authorized-partial-batch-saved"
                or batch.get("wholeWorldComplete") is not False or batch.get("compilation")!="not-performed"
                or batch.get("renderedInspection")!="not-performed" or batch.get("playableAcceptance")!="not-established"
                or batch.get("startupMapRegistration")!="unchanged"
                or not re.fullmatch(r"[a-f0-9]{64}",batch["authorizationSha256"])
                or not re.fullmatch(r"[a-f0-9]{64}",batch["generatorIdentity"])
                or not isinstance(batch.get("jobs"),list) or not batch["jobs"]
                or not isinstance(batch["jobIdentities"],list) or len(batch["jobs"])!=len(batch["jobIdentities"])):
            raise ValueError("Incomplete or conflicting completion batch")
        identities=[]
        for item,record in zip(batch["jobIdentities"],batch["jobs"]):
            if (not isinstance(record,dict) or record.get("state")!="saved"
                    or item!={"job":record["job"],"identity":record["identity"]}
                    or record["provenance"]["generatorIdentity"]!=batch["generatorIdentity"]):
                raise ValueError("Incomplete completed job inventory")
            identities.append(record["identity"])
        if len(set(identities))!=len(identities) or identities.count(identity)!=1:
            raise ValueError("Missing or ambiguous completed world")
        return batch["jobs"][identities.index(identity)]
    except (KeyError,TypeError,ValueError) as error:
        raise RuntimeError("Invalid world completion evidence; explicit recovery review required") from error


@contextmanager
def serial_worker_lock():
    path=owned_output("execution/worker.lock")
    path.parent.mkdir(parents=True,exist_ok=True)
    fd=os.open(path,os.O_CREAT|os.O_EXCL|os.O_WRONLY)
    try:
        os.write(fd,json.dumps({"pid":os.getpid(),"owner":OWNER,"startedAt":time.time()}).encode())
        yield
    finally:
        os.close(fd)
        path.unlink()
    # A crash leaves the lock. Inspect its PID and journal before a human
    # explicitly authorizes removing this exact lock; never auto-steal it.


def slug(identifier):
    readable=re.sub(r"[^A-Za-z0-9_]","_",identifier)[:80]
    return readable+"_"+digest(identifier)[:10]


def generator_identity():
    return digest({path.name:file_hash(path) for path in sorted(SCRIPT_DIR.glob("*.py"))})


LOADED_GENERATOR_IDENTITY = generator_identity()


def world_provenance(value,generator_hash):
    # Bind inherited evidence directly into the world identity so a saved
    # journal cannot shed its template or children while retaining that identity.
    return {"schemaVersion":1,"job":value["id"],"generatorIdentity":generator_hash,
            "recipeSha256":digest(value),"sources":deepcopy(value["sources"]),
            "template":deepcopy(value.get("template")),"contract":deepcopy(value.get("contract"))}


def world_package(job,identity):
    return NAMESPACE+"/R_"+identity[:24]+"/Maps/L_"+slug(job)


def package_files(package,is_map=False):
    if not re.fullmatch(r"/Game/BaddieGeneratedWorld/[A-Za-z0-9_/]+",package):
        raise ValueError("Foreign native package evidence")
    base=REPO/"apps/companion-unreal/Content"/package.removeprefix("/Game/")
    primary=base.with_suffix(".umap" if is_map else ".uasset")
    if not primary.is_file():
        raise ValueError("Missing real native package "+package)
    files=[primary]+[base.with_suffix(suffix) for suffix in (".uexp",".ubulk",".uptnl") if base.with_suffix(suffix).is_file()]
    return {path.relative_to(REPO).as_posix():file_hash(path) for path in files}


def inspect_recipe(value,refs):
    """Execution-time fail-closed contracts; not run during preparation."""
    inspect_content(value)
    for source in value["sources"]:
        if source.get("sha256")!=refs.entries[source["path"]]["sha256"]:
            raise ValueError("Job source identity differs from pinned reference")
    for mesh in value.get("meshes",[]):
        vertices=mesh.get("vertices",[])
        if not vertices or not mesh.get("faces"):
            raise ValueError("Empty canonical mesh "+mesh["id"])
        for point in vertices:
            ue_point(point)
        for face in mesh["faces"]:
            if len(face)!=3 or any(type(i) is not int or i<0 or i>=len(vertices) for i in face):
                raise ValueError("Invalid canonical triangle "+mesh["id"])
        if mesh.get("collision","none") not in ("none","convex"):
            raise ValueError("Unsupported native collision contract")
    for part in value.get("parts",[]):
        if part.get("collision","none") not in ("none","convex"):
            raise ValueError("Unsupported native part collision contract")
        ue_point(part["position"])
        if (len(part["size"])!=3 or any(not math.isfinite(v) or v<=0 for v in part["size"])
                or len(part.get("rotation",[0,0,0]))!=3
                or any(not math.isfinite(v) for v in part.get("rotation",[0,0,0]))):
            raise ValueError("Invalid native instance dimensions/rotation")
        shape_mesh(part["shape"])
        if part.get("rotation",[0,0,0])[0]!=0 or part.get("rotation",[0,0,0])[2]!=0:
            raise ValueError("Bake non-yaw rotations before HISM instancing")
    for spec in [m["material"] for m in value.get("meshes",[])]+[p["material"] for p in value.get("parts",[])]:
        if spec.get("emission",0)!=0 or not re.fullmatch(r"#[a-fA-F0-9]{6}",spec.get("colour","")):
            raise ValueError("Invalid/unpowered native material contract")
    if value["category"] in ("place","waterfront","interior"):
        inspect_place_contract(value,refs)
    if value["category"]=="rural":
        contract=value.get("contract")
        if (not value.get("template") or not contract or not contract.get("approvedMetricTerrainSha256")
                or not contract.get("travelEndpoints") or not contract.get("navigation")):
            raise ValueError("Rural needs initialized native Landscape and approved metric/travel/navigation inputs")
    if value["id"] in ("city/native-navigation","city/outdoor-lighting") and not value.get("template"):
        raise ValueError("Navigation/lighting needs an approved initialized native template, not mesh-only scenery")
    if value["id"]=="city/streamed-world-composition" and not value.get("contract",{}).get("streamingLevels"):
        raise ValueError("Composition must reference journaled native child maps; no empty whole-world stand-in")


class NativeAuthor:
    def __init__(self, unreal, refs, authorization):
        self.u,self.refs,self.authorization=unreal,refs,authorization
        self.assets=unreal.get_editor_subsystem(unreal.EditorAssetSubsystem)
        self.actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
        self.levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
        self.editor=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
        self.subobjects=unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
        self.generator_hash=generator_identity()
        self.packages=[]
        self.pending_worlds=[]

    def clean_editor(self):
        dirty=list(self.u.EditorLoadingAndSavingUtils.get_dirty_map_packages())
        dirty+=list(self.u.EditorLoadingAndSavingUtils.get_dirty_content_packages())
        if dirty:
            raise RuntimeError("Dirty packages; save/discard separately before generation: "+str([p.get_name() for p in dirty]))
        if self.editor.get_game_world() is not None:
            raise RuntimeError("Stop PIE separately before generation")

    def owned_package(self,path):
        if not re.fullmatch(r"/Game/BaddieGeneratedWorld/[A-Za-z0-9_/]+",path):
            raise ValueError("Foreign or malformed Unreal package target")
        return path

    def claim(self,asset,identity):
        self.assets.set_metadata_tag(asset,"Baddie.WorldGeneration.Owner",OWNER)
        self.assets.set_metadata_tag(asset,"Baddie.WorldGeneration.Identity",identity)
        self.assets.set_metadata_tag(asset,"Baddie.WorldGeneration.SourceCommit",SOURCE_COMMIT)

    def existing(self,path,identity):
        self.owned_package(path)
        if not self.assets.does_asset_exist(path):
            return None
        asset=self.assets.load_asset(path)
        if (self.assets.get_metadata_tag(asset,"Baddie.WorldGeneration.Owner")!=OWNER
                or self.assets.get_metadata_tag(asset,"Baddie.WorldGeneration.Identity")!=identity):
            raise RuntimeError("Refusing to overwrite conflicting content "+path)
        if not isinstance(asset,self.u.World):
            journal_path=owned_output("execution/assets/"+identity+".json")
            if not journal_path.exists():
                raise RuntimeError("Unjournaled native asset needs explicit recovery review "+path)
            receipt=json.loads(journal_path.read_text(encoding="utf-8"))
            package_file=REPO/"apps/companion-unreal/Content"/(path.removeprefix("/Game/")+".uasset")
            if (receipt.get("owner")!=OWNER or receipt.get("identity")!=identity
                    or receipt.get("package")!=path or file_hash(package_file)!=receipt["sha256"]
                    or receipt.get("files")!=package_files(path)):
                raise RuntimeError("Immutable native asset file changed "+path)
        return asset

    def save_asset(self,asset,path,identity):
        self.claim(asset,identity)
        if not self.assets.save_loaded_asset(asset,only_if_is_dirty=False):
            raise RuntimeError("Native asset save failed "+path)
        self.packages.append(path)
        package_file=REPO/"apps/companion-unreal/Content"/(path.removeprefix("/Game/")+".uasset")
        atomic_json(owned_output("execution/assets/"+identity+".json"),
                    {"owner":OWNER,"identity":identity,"package":path,"sha256":file_hash(package_file),"files":package_files(path)})
        return asset

    def generated_dependency_inventory(self,packages):
        inventory={}
        for package in sorted(packages):
            dependency=self.assets.load_asset(self.owned_package(package))
            if dependency is None or isinstance(dependency,self.u.World):
                raise ValueError("Unexpected saved mesh/material dependency "+package)
            identity=self.assets.get_metadata_tag(dependency,"Baddie.WorldGeneration.Identity")
            if not re.fullmatch(r"[a-f0-9]{64}",identity or ""):
                raise ValueError("Malformed generated dependency identity "+package)
            self.existing(package,identity)
            inventory[package]={"identity":identity,"files":package_files(package)}
        return inventory

    def verify_saved_world(self,path,identity,visiting=None):
        if not re.fullmatch(r"[a-f0-9]{64}",identity or ""):
            raise ValueError("Malformed saved-world recipe identity")
        visiting=set() if visiting is None else visiting
        if path in visiting:
            raise ValueError("Cyclic saved-world dependency")
        visiting.add(path)
        asset=self.existing(path,identity)
        if not isinstance(asset,self.u.World):
            raise ValueError("Dependency is not an owned native World "+path)
        journal=owned_output("execution/jobs/"+identity+".json")
        receipt=json.loads(journal.read_text(encoding="utf-8"))
        if (not isinstance(receipt,dict) or receipt.get("owner")!=OWNER or receipt.get("sourceCommit")!=SOURCE_COMMIT
                or receipt.get("identity")!=identity or receipt.get("mapPackage")!=path
                or receipt.get("state")!="saved" or receipt.get("files")!=package_files(path,True)):
            raise ValueError("Unsaved, conflicting or changed world dependency "+path)
        completion=completed_world(identity)
        if digest(receipt)!=digest(completion):
            raise ValueError("World journal differs from immutable completion; explicit recovery review required "+path)
        provenance=receipt.get("provenance")
        if (not isinstance(provenance,dict)
                or set(provenance)!={"schemaVersion","job","generatorIdentity","recipeSha256","sources","template","contract"}
                or provenance["schemaVersion"]!=1
                or not re.fullmatch(r"[a-f0-9]{64}",provenance.get("generatorIdentity") or "")
                or not re.fullmatch(r"[a-f0-9]{64}",provenance.get("recipeSha256") or "")
                or digest(provenance)!=identity):
            raise ValueError("Incomplete or altered world provenance; explicit recovery review required "+path)
        if (receipt.get("job")!=provenance["job"] or receipt.get("sources")!=provenance["sources"]
                or receipt.get("contract")!=provenance["contract"]
                or path!=world_package(provenance["job"],identity)
                or receipt.get("streamingLevels",[])!=(provenance["contract"] or {}).get("streamingLevels",[])):
            raise ValueError("Saved world differs from its bound provenance "+path)
        if provenance["template"] is not None:
            self.verify_template(provenance["template"])
        dependencies=completion.get("generatedDependencies")
        if (not isinstance(dependencies,dict) or completion.get("packages")!=sorted([path,*dependencies])
                or self.generated_dependency_inventory(dependencies)!=dependencies):
            raise ValueError("Incomplete or changed generated dependencies; explicit recovery review required "+path)
        for child in receipt.get("streamingLevels",[]):
            self.verify_saved_world(child["package"],child["identity"],visiting)
        visiting.remove(path)
        return receipt

    def material(self,spec):
        # Emission remains zero. Only runtime utility receipts may power scenery.
        if spec.get("emission",0)!=0:
            raise ValueError("Scene materials cannot assume utility power")
        identity=digest({"generator":self.generator_hash,"material":spec})
        path=NAMESPACE+"/Materials/M_"+identity[:24]
        existing=self.existing(path,identity)
        if existing:
            self.packages.append(path);return existing
        u=self.u
        material=u.AssetToolsHelpers.get_asset_tools().create_asset(path.rsplit("/",1)[1],path.rsplit("/",1)[0],u.Material,u.MaterialFactoryNew())
        if material is None:
            raise RuntimeError("Material creation failed")
        lib=u.MaterialEditingLibrary
        colour=lib.create_material_expression(material,u.MaterialExpressionConstant3Vector,-350,0)
        raw=spec["colour"].lstrip("#")
        if len(raw)!=6:
            raise ValueError("Expected canonical RGB colour")
        def linear(value):
            channel=int(value,16)/255
            return channel/12.92 if channel<=.04045 else ((channel+.055)/1.055)**2.4
        colour.set_editor_property("constant",u.LinearColor(linear(raw[:2]),linear(raw[2:4]),linear(raw[4:]),1))
        lib.connect_material_property(colour,"",u.MaterialProperty.MP_BASE_COLOR)
        for index,(name,prop) in enumerate((("roughness",u.MaterialProperty.MP_ROUGHNESS),("metalness",u.MaterialProperty.MP_METALLIC))):
            expression=lib.create_material_expression(material,u.MaterialExpressionConstant,-350,140+index*120)
            expression.set_editor_property("r",float(spec.get(name,0)))
            lib.connect_material_property(expression,"",prop)
        material.set_editor_property("two_sided",bool(spec.get("twoSided",False)))
        # This editor material operation can compile shaders. Receipt must
        # explicitly permit native material/mesh derived-data processing.
        lib.recompile_material(material)
        return self.save_asset(material,path,identity)

    def mesh(self,vertices,faces,material,collision="none"):
        if collision not in ("none","convex"):
            raise ValueError("Unsupported native collision recipe; need approved authored collision")
        identity=digest({"generator":self.generator_hash,"vertices":vertices,"faces":faces,"collision":collision})
        path=NAMESPACE+"/Meshes/SM_"+identity[:24]
        existing=self.existing(path,identity)
        if existing:
            self.packages.append(path);return existing
        u=self.u
        filename=owned_output("execution/import-source/"+identity+".obj")
        filename.parent.mkdir(parents=True,exist_ok=True)
        text=obj_text(vertices,faces)
        if filename.exists():
            if filename.read_text(encoding="utf-8")!=text:
                raise RuntimeError("Conflicting immutable mesh import input")
        else:
            with filename.open("x",encoding="utf-8",newline="\n") as stream:
                stream.write(text)
        task=u.AssetImportTask()
        for key,value in {"filename":str(filename),"destination_path":path.rsplit("/",1)[0],
                          "destination_name":path.rsplit("/",1)[1],"automated":True,
                          "replace_existing":False,"save":False,"factory":u.FbxFactory()}.items():
            task.set_editor_property(key,value)
        options=u.FbxImportUI()
        for key,value in {"automated_import_should_detect_type":False,"import_mesh":True,
                          "import_as_skeletal":False,"import_materials":False,"import_textures":False,
                          "import_animations":False,"mesh_type_to_import":u.FBXImportType.FBXIT_STATIC_MESH}.items():
            options.set_editor_property(key,value)
        data=options.get_editor_property("static_mesh_import_data")
        for key,value in {"combine_meshes":True,"convert_scene":False,"convert_scene_unit":False,
                          "force_front_x_axis":False,"import_uniform_scale":1.0,
                          "import_translation":u.Vector(0,0,0),"import_rotation":u.Rotator(0,0,0),
                          "build_nanite":False,"auto_generate_collision":collision=="convex",
                          "generate_lightmap_u_vs":True,
                          "normal_import_method":u.FBXNormalImportMethod.FBXNIM_COMPUTE_NORMALS}.items():
            data.set_editor_property(key,value)
        task.set_editor_property("options",options)
        u.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
        imported=list(task.get_editor_property("imported_object_paths"))
        if len(imported)!=1 or imported[0].split(".")[0]!=path:
            raise RuntimeError("Unexpected OBJ import packages "+str(imported))
        mesh=self.assets.load_asset(path)
        if not isinstance(mesh,u.StaticMesh):
            raise RuntimeError("Importer did not return a native StaticMesh")
        # Fail closed on importer orientation/units. This is an execution-time
        # contract check, not a rendered-acceptance or playability assertion.
        bounds=mesh.get_bounding_box()
        minimum,maximum=bounds.get_editor_property("min"),bounds.get_editor_property("max")
        for index,axis in enumerate(("x","y","z")):
            if (abs(getattr(minimum,axis)-min(p[index] for p in vertices))>.1
                    or abs(getattr(maximum,axis)-max(p[index] for p in vertices))>.1):
                raise RuntimeError("Native mesh importer changed Baddie axis/scale contract")
        # Material is deliberately not part of mesh identity; instances override
        # it and do not mutate a reused mesh's slots.
        return self.save_asset(mesh,path,identity)

    def spawn(self,cls,identifier,location=None):
        actor=self.actors.spawn_actor_from_class(cls,location or self.u.Vector(0,0,0),self.u.Rotator(0,0,0),transient=False)
        if actor is None:
            raise RuntimeError("Native actor spawn failed "+identifier)
        actor.set_actor_label(slug(identifier))
        actor.set_editor_property("tags",[self.u.Name(OWNER),self.u.Name("Baddie.CanonicalId="+identifier),self.u.Name("Baddie.Readiness=Closed")])
        return actor

    def component(self,actor,cls):
        handles=self.subobjects.k2_gather_subobject_data_for_instance(actor)
        if not handles:
            raise RuntimeError("No persistent actor subobject handle")
        params=self.u.AddNewSubobjectParams(parent_handle=handles[0],new_class=cls,blueprint_context=None)
        handle,reason=self.subobjects.add_new_subobject(params)
        lib=self.u.SubobjectDataBlueprintFunctionLibrary
        if not lib.is_handle_valid(handle):
            raise RuntimeError("Native component creation failed "+str(reason))
        return lib.get_associated_object(lib.get_data(handle))

    def instantiate_parts(self,parts):
        bins=defaultdict(list)
        for part in parts:
            if part.get("rotation",[0,0,0])[0]!=0 or part.get("rotation",[0,0,0])[2]!=0:
                raise ValueError("HISM parts require yaw-only; bake XYZ Euler as explicit meshes")
            key=(part["shape"],digest(part["material"]),part.get("collision","none"),
                 math.floor(part["position"][0]/240),math.floor(part["position"][2]/240))
            bins[key].append(part)
        instances=[]
        for (shape,_,collision,tx,tz),values in sorted(bins.items()):
            material=self.material(values[0]["material"])
            v,f=shape_mesh(shape)
            mesh=self.mesh([ue_point(p) for p in v],f,material,collision)
            # StaticMeshActor gives a persistent root without Blueprint creation.
            actor=self.spawn(self.u.StaticMeshActor,f"instances:{shape}:{tx}:{tz}:"+digest(values)[:12])
            actor.get_component_by_class(self.u.StaticMeshComponent).set_mobility(self.u.ComponentMobility.STATIC)
            component=self.component(actor,self.u.HierarchicalInstancedStaticMeshComponent)
            component.set_mobility(self.u.ComponentMobility.STATIC)
            component.set_static_mesh(mesh);component.set_material(0,material)
            component.set_collision_enabled(self.u.CollisionEnabled.NO_COLLISION if collision=="none" else self.u.CollisionEnabled.QUERY_AND_PHYSICS)
            component.set_cull_distances(80000,100000)
            component.set_editor_property("cast_shadow",True)
            for index,part in enumerate(values):
                x,y,z=part["size"]
                transform=self.u.Transform(location=self.u.Vector(*ue_point(part["position"])),
                    rotation=self.u.Rotator(0,math.degrees(part.get("rotation",[0,0,0])[1]),0),
                    scale=self.u.Vector(z,x,y))
                result=component.add_instance(transform,world_space=True)
                if result!=index:
                    raise RuntimeError("Native instance identity order changed")
                instances.append({"canonicalId":part["id"],"parcelId":part.get("parcelId"),
                                  "actor":actor.get_name(),"component":component.get_name(),"instanceIndex":index})
        return instances

    def instantiate_meshes(self,meshes):
        # Merge same-finish geometry in bounded 240 m cells. Canonical feature
        # IDs and triangle ranges survive in the execution journal.
        bins=defaultdict(list)
        for mesh in meshes:
            centre=[sum(p[i] for p in mesh["vertices"])/len(mesh["vertices"]) for i in range(3)]
            key=(digest(mesh["material"]),mesh.get("collision","none"),math.floor(centre[0]/240),math.floor(centre[2]/240))
            bins[key].append(mesh)
        mappings=[]
        for (_,collision,tx,tz),values in sorted(bins.items()):
            v,f=[],[]
            features=[]
            for item in values:
                offset=len(v);start=len(f)
                v += [ue_point(p) for p in item["vertices"]]
                f += [[i+offset for i in face] for face in item["faces"]]
                features.append({"canonicalId":item["id"],"firstTriangle":start,"triangleCount":len(f)-start})
            material=self.material(values[0]["material"])
            asset=self.mesh(v,f,material,collision)
            actor=self.spawn(self.u.StaticMeshActor,f"mesh-cell:{tx}:{tz}:"+digest(features)[:12])
            component=actor.get_component_by_class(self.u.StaticMeshComponent)
            component.set_mobility(self.u.ComponentMobility.STATIC)
            component.set_static_mesh(asset);component.set_material(0,material)
            component.set_collision_enabled(self.u.CollisionEnabled.NO_COLLISION if collision=="none" else self.u.CollisionEnabled.QUERY_AND_PHYSICS)
            mappings.append({"actor":actor.get_name(),"mesh":asset.get_path_name(),"features":features})
        return mappings

    def existing_level(self,value):
        identity=digest(world_provenance(value,self.generator_hash))
        path=world_package(value["id"],identity)
        journal_path=owned_output("execution/jobs/"+identity+".json")
        # A missing map does not erase evidence of an earlier attempt.
        if journal_path.exists():
            try:
                previous=json.loads(journal_path.read_text(encoding="utf-8"))
            except (OSError, UnicodeError, json.JSONDecodeError) as error:
                raise RuntimeError("Invalid map journal; inspect before any recovery") from error
            if isinstance(previous,dict) and previous.get("state")=="saved":
                # Asset metadata plus exact journal/file hash, not presence alone.
                return self.verify_saved_world(path,identity)
            raise RuntimeError("Unjournaled or interrupted map; inspect before any recovery")
        if self.existing(path,identity):
            raise RuntimeError("Unjournaled or interrupted map; inspect before any recovery")
        completion_path=world_completion_path(identity)
        if completion_path.exists() or completion_path.is_symlink():
            raise RuntimeError("Orphaned world completion; explicit recovery review required")
        return None

    def create_level(self,value):
        previous=self.existing_level(value)
        if previous is not None:
            return previous
        provenance=world_provenance(value,self.generator_hash)
        identity=digest(provenance)
        path=world_package(value["id"],identity)
        journal_path=owned_output("execution/jobs/"+identity+".json")
        self.clean_editor()
        record={"owner":OWNER,"sourceCommit":SOURCE_COMMIT,"job":value["id"],"identity":identity,
                "state":"preparing","mapPackage":path,"sources":value["sources"],"provenance":provenance,"startedAt":time.time()}
        atomic_json(journal_path,record)
        try:
            if value.get("template"):
                # Templates must already be real native maps with approved file
                # evidence; never spawn an empty Landscape or unbrushed nav box.
                template=value["template"]
                self.verify_template(template)
                created=self.levels.new_level_from_template(path,template["package"])
            else:
                created=self.levels.new_level(path,is_partitioned_world=False)
            if not created:
                raise RuntimeError("Native map creation failed")
            world=self.editor.get_editor_world()
            self.claim(world,identity)
            record["state"]="authoring";atomic_json(journal_path,record)
            if value.get("contract",{}).get("streamingLevels"):
                if self.authorization.get("nativeLevelComposition") is not True:
                    raise PermissionError("Native level composition was not explicitly authorized")
                record["streamingLevels"]=[]
                for child in value["contract"]["streamingLevels"]:
                    package=child["package"]
                    self.verify_saved_world(package,child["identity"])
                    level=self.u.EditorLevelUtils.add_level_to_world(world,package,self.u.LevelStreamingDynamic)
                    if level is None:
                        raise RuntimeError("Native streaming level registration failed")
                    # Authoring registry only. Runtime travel owner requests
                    # load/visibility after admission and readiness; not here.
                    level.set_should_be_loaded(False);level.set_should_be_visible(False)
                    record["streamingLevels"].append(child)
            record["instances"]=self.instantiate_parts(value["parts"])
            record["meshes"]=self.instantiate_meshes(value["meshes"])
            record["contract"]=value.get("contract")
            # Runtime actor names are not supplied by this authoring worker.
            # No PlayerStart, gateway, mission/NPC readiness or account binding.
            if not self.levels.save_current_level():
                raise RuntimeError("Native .umap save failed")
            map_file=REPO/"apps/companion-unreal/Content"/(path.removeprefix("/Game/")+".umap")
            record.update(state="saved",mapFileSha256=file_hash(map_file),files=package_files(path,True),packages=sorted(set(self.packages+[path])),
                          generatedDependencies=self.generated_dependency_inventory(set(self.packages)),
                          savedAt=time.time(),runtimeAdmission=False,renderedInspection="not-performed",playableAcceptance="not-established")
            # Keep the disk journal non-saved until the batch and this world's
            # immutable reference are durable. Interrupted publication holds.
            self.pending_worlds.append(record)
            return record
        except Exception as error:
            record.update(state="failed",error=str(error),packages=sorted(set(self.packages)),failedAt=time.time())
            atomic_json(journal_path,record)
            raise

    def complete_batch(self,path,result):
        save_immutable_completion(path,result)
        reference={"schemaVersion":1,"owner":OWNER,"sourceCommit":SOURCE_COMMIT,
                   "batchIdentity":batch_identity(result),"batchSha256":file_hash(path)}
        for record in self.pending_worlds:
            save_immutable_completion(world_completion_path(record["identity"]),{**reference,"identity":record["identity"]})
        for record in self.pending_worlds:
            atomic_json(owned_output("execution/jobs/"+record["identity"]+".json"),record)
        # Reused worlds keep their original immutable reference, including
        # when a new authorization or another parent includes them in a batch.
        for record in result["jobs"]:
            self.verify_saved_world(record["mapPackage"],record["identity"])

    def verify_template(self,template):
        if not isinstance(template,dict):
            raise ValueError("Missing initialized template evidence")
        package=template["package"]
        self.owned_package(package)
        if not package.startswith(NAMESPACE+"/Inputs/") or not self.assets.does_asset_exist(package):
            raise ValueError("Approved native template must be delivered inside the owned Inputs namespace")
        expected=template["files"]
        if not isinstance(expected,dict) or not expected:
            raise ValueError("Template needs complete native package/sidecar hashes")
        if len({relative.casefold() for relative in expected})!=len(expected):
            raise ValueError("Ambiguous initialized template file identities")
        content=REPO/"apps/companion-unreal/Content/BaddieGeneratedWorld/Inputs"
        for relative,sha in expected.items():
            if (not re.fullmatch(r"[A-Za-z0-9_]+(?:/[A-Za-z0-9_]+)*\.(umap|uasset|uexp|ubulk|uptnl)",relative)
                    or not re.fullmatch(r"[a-f0-9]{64}",sha or "")):
                raise ValueError("Malformed initialized template file evidence")
            target=(content/relative).resolve()
            if not target.is_relative_to(content.resolve()) or file_hash(target)!=sha:
                raise ValueError("Native template package/sidecar differs from approved input")
        expected_map=package.removeprefix(NAMESPACE+"/Inputs/")+".umap"
        if expected_map not in expected:
            raise ValueError("Template's real .umap was not hash-pinned")
        observed={}
        prefix=content.relative_to(REPO).as_posix()+"/"
        for relative in expected:
            if not relative.endswith((".umap",".uasset")):
                continue
            is_map=relative.endswith(".umap")
            if (content/relative).with_suffix(".uasset" if is_map else ".umap").exists():
                raise ValueError("Ambiguous initialized template package "+relative)
            dependency=NAMESPACE+"/Inputs/"+str(Path(relative).with_suffix("")).replace("\\","/")
            observed.update({path.removeprefix(prefix):sha for path,sha in package_files(dependency,is_map).items()})
        if observed!=expected:
            raise ValueError("Missing or unexpected initialized template dependency sidecars")
        if template.get("partitioned") is not False or template.get("externalActors") is not False:
            raise ValueError("Only approved non-partitioned, non-externalized native templates are supported")
        if template.get("runtimeAuthority") is not False or not template.get("approvedActorInventory"):
            raise ValueError("Template must include approved native scenery actor inventory and no runtime authority")
        if not isinstance(self.assets.load_asset(package),self.u.World):
            raise ValueError("Native template is not a real World")


def execute(authorization_path,authorization_sha256,expected_pid,selected_jobs):
    """Explicit call only. No CLI/default selection that can start engine work."""
    import unreal
    if os.getpid()!=expected_pid:
        raise RuntimeError("Editor PID changed")
    actual=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).resolve()
    if os.path.normcase(str(actual))!=os.path.normcase(str((REPO/"apps/companion-unreal").resolve())):
        raise RuntimeError("Wrong native project")
    if not unreal.SystemLibrary.get_engine_version().startswith("5.8.3-"):
        raise RuntimeError("Inspect changed engine APIs before generation")
    refs=References()
    if generator_identity()!=LOADED_GENERATOR_IDENTITY:
        raise RuntimeError("Loaded generator source is stale; inspect and reload only this package before another batch")
    authorization=require_handoff(authorization_path,authorization_sha256,refs)
    required=("engineGeneration","nativeMeshImport","nativeMaterialShaderProcessing","saveNativePackages")
    if (not authorization.get("humanAuthorizationEvidence") or authorization.get("expectedPid")!=expected_pid
            or authorization.get("generatorIdentity")!=generator_identity()
            or not all(authorization.get(key) is True for key in required)):
        raise PermissionError("Direct human engine/import/generation authorization is required")
    refs.verify(authorization.get("referenceManifestReview"))
    if not selected_jobs or len(selected_jobs)!=len(set(selected_jobs)):
        raise ValueError("Select a nonempty unique job list")
    if not set(selected_jobs).issubset(set(authorization.get("jobs",[]))):
        raise PermissionError("Selected jobs exceed the authorized batch")
    handoffs={}
    for item in authorization.get("handoffs",[]):
        value=require_handoff(item["path"],item["sha256"],refs)
        for record in value.get("jobs",[]):
            if record["id"] in handoffs:
                raise ValueError("Conflicting handoff job ownership")
            handoffs[record["id"]]={**record,"sources":record.get("sources",value["sources"])}
    jobs={value["id"]:value for value in build_jobs(refs,handoffs)}
    chosen=[jobs[identifier] for identifier in selected_jobs]
    blocked={value["id"]:value["blockers"] for value in chosen if value["blockers"]}
    if blocked:
        raise RuntimeError("Selected jobs lack canonical/native inputs: "+json.dumps(blocked))
    if any(value.get("template") for value in chosen) and authorization.get("nativeTemplateDuplication") is not True:
        raise PermissionError("Native template duplication requires explicit authorization")
    for value in chosen:
        inspect_recipe(value,refs)
    # All source/handoff/authority guards resolve before the first mutation.
    with serial_worker_lock():
        author=NativeAuthor(unreal,refs,authorization);author.clean_editor()
        batch={"owner":OWNER,"sourceCommit":SOURCE_COMMIT,"authorizationSha256":authorization_sha256,
               "generatorIdentity":author.generator_hash,
               "jobIdentities":[{"job":value["id"],"identity":digest(world_provenance(value,author.generator_hash))} for value in chosen]}
        batch_path=owned_output("execution/batch-"+batch_identity(batch)+".json")
        result={**batch,"state":"authorized-partial-batch-saved",
                "wholeWorldComplete":False,"compilation":"not-performed","renderedInspection":"not-performed",
                "playableAcceptance":"not-established","startupMapRegistration":"unchanged"}
        if batch_path.exists() or batch_path.is_symlink():
            previous=read_completion(batch_path)
            if (not isinstance(previous,dict) or any(previous.get(key)!=item for key,item in result.items())
                    or not isinstance(previous.get("jobs"),list) or len(previous["jobs"])!=len(chosen)):
                raise RuntimeError("Conflicting or partial completed batch; explicit recovery review required")
            records=[author.verify_saved_world(world_package(item["job"],item["identity"]),item["identity"])
                     for item in batch["jobIdentities"]]
            if digest(previous)!=digest({**result,"jobs":records}):
                raise RuntimeError("Completed batch differs from verified worlds; explicit recovery review required")
            print(json.dumps(previous))
            return previous
        for value in chosen:
            author.existing_level(value)
            if value.get("template"):
                author.verify_template(value["template"])
            children=value.get("contract",{}).get("streamingLevels",[])
            if children and authorization.get("nativeLevelComposition") is not True:
                raise PermissionError("Native level composition was not explicitly authorized")
            for child in children:
                author.verify_saved_world(child["package"],child["identity"])
        records=[]
        for value in chosen:
            author.packages=[]
            records.append(author.create_level(value))
        result["jobs"]=records
        author.complete_batch(batch_path,result)
        print(json.dumps(result))
        return result

"""Owned derivative of the frozen Danfo kit; no facade generator or live mutation."""
import argparse
from datetime import datetime, timezone
import hashlib
import json
import math
import os
from pathlib import Path
import runpy
import sys
import time
import traceback

import bpy
from mathutils import Vector

JOB = "lagos-danfo-detail-v1"
ROOT = Path("C:/dev/cribtest-game/output/cribtest11-visible-world-2026-10-11/street-props")
SOURCE = ROOT / "v1/LagosStreetProps.blend"
SOURCE_HASH = "0c9d3787cb028dc6bbcdcb18eca7379a99b6861d93dc72a03b7319c495732549"
HELPERS = ROOT / "lagos_street_props.py"
HELPERS_HASH = "c25297c16a6fb24752d9fb13fb651d5be7683c92a530e3a4758365061b20e877"
OUTPUT = ROOT / "realism/danfo-detail-v1"


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def write_json(path, record):
    temporary = path.with_name(path.name + ".tmp")
    temporary.write_text(json.dumps(record, indent=2) + "\n", encoding="utf-8")
    temporary.replace(path)


def progress(step, **values):
    report = {"job": JOB, "pid": os.getpid(), "step": step,
              "updated_at": datetime.now(timezone.utc).isoformat(), **values}
    write_json(OUTPUT / "progress.json", report)
    print(json.dumps(report), flush=True)


def material_detail(materials):
    glass = materials["GlassDark"]
    shader = glass.node_tree.nodes.get("Principled BSDF")
    shader.inputs["Base Color"].default_value = (0.09, 0.19, 0.20, 1)
    shader.inputs["Roughness"].default_value = 0.12
    shader.inputs["Metallic"].default_value = 0
    shader.inputs["Transmission Weight"].default_value = 0.68
    shader.inputs["IOR"].default_value = 1.48
    shader.inputs["Coat Weight"].default_value = 0.10
    for name in ("DanfoYellow", "YellowFaded"):
        material = materials[name]
        nodes, links = material.node_tree.nodes, material.node_tree.links
        shader = nodes.get("Principled BSDF")
        shader.inputs["Coat Weight"].default_value = .24
        shader.inputs["Coat Roughness"].default_value = .26
        geometry = nodes.new("ShaderNodeNewGeometry")
        fine = nodes.new("ShaderNodeTexNoise")
        fine.inputs["Scale"].default_value = 420
        fine.inputs["Detail"].default_value = 2
        links.new(geometry.outputs["Position"], fine.inputs["Vector"])
        bump = nodes.new("ShaderNodeBump")
        bump.inputs["Distance"].default_value = .00012
        bump.inputs["Strength"].default_value = .15
        links.new(fine.outputs["Fac"], bump.inputs["Height"])
        links.new(bump.outputs["Normal"], shader.inputs["Normal"])
        split = nodes.new("ShaderNodeSeparateXYZ")
        links.new(geometry.outputs["Position"], split.inputs["Vector"])
        lower = nodes.new("ShaderNodeMapRange")
        lower.inputs["From Min"].default_value = .47
        lower.inputs["From Max"].default_value = .95
        lower.inputs["To Min"].default_value = 1
        lower.inputs["To Max"].default_value = 0
        links.new(split.outputs["Z"], lower.inputs["Value"])
        mix = nodes.new("ShaderNodeMixRGB")
        base = tuple(shader.inputs["Base Color"].default_value)
        mix.inputs[1].default_value = base
        mix.inputs[2].default_value = tuple(value * .78 for value in base[:3]) + (1,)
        links.new(lower.outputs["Result"], mix.inputs[0])
        links.new(mix.outputs["Color"], shader.inputs["Base Color"])
        rough = nodes.new("ShaderNodeMapRange")
        rough.inputs["From Min"].default_value = 0
        rough.inputs["From Max"].default_value = 1
        rough.inputs["To Min"].default_value = .34
        rough.inputs["To Max"].default_value = .58
        links.new(lower.outputs["Result"], rough.inputs["Value"])
        links.new(rough.outputs["Result"], shader.inputs["Roughness"])
    materials["Steel"].node_tree.nodes.get("Principled BSDF").inputs["Metallic"].default_value = 1
    materials["Steel"].node_tree.nodes.get("Principled BSDF").inputs["Roughness"].default_value = .36
    rubber = materials["Rubber"].node_tree.nodes.get("Principled BSDF")
    rubber.inputs["Roughness"].default_value = .78


def wheel_profile(entry, mesh, side, axle):
    # Tyre ring shoulder/sidewall profile, with shallow recessed tread sipes.
    # Axle height and max radius remain 0.34m: ground contact is unchanged.
    profile = [(0.845, .205), (.849, .265), (.858, .308), (.877, .330),
               (.896, .339), (.915, .340), (.935, .340), (.955, .340),
               (.975, .340), (.994, .339), (1.014, .332), (1.038, .316),
               (1.061, .273), (1.069, .205)]
    vertices, faces = [], []
    segments = 192
    for ring, (distance, radius) in enumerate(profile):
        for index in range(segments):
            angle = math.tau * index / segments
            groove = .0035 if 4 <= ring <= 10 and ((index + ring // 2) % 6 == 0 or ring in (6, 8)) else 0
            r = radius - groove
            vertices.append((side * distance, axle + r * math.cos(angle), .34 + r * math.sin(angle)))
            if ring:
                a = (ring - 1) * segments + index
                b = (ring - 1) * segments + (index + 1) % segments
                faces.append((a, b, b + segments, a + segments))
    for index in range(segments):
        following = (index + 1) % segments
        faces.append((index, (len(profile) - 1) * segments + index,
                      (len(profile) - 1) * segments + following, following))
    mesh(entry, "ProfileTyre_%s_%s" % (side, axle), vertices, faces, "Rubber", smooth=True)


def ring_x(entry, mesh, name, side, axle, profile, material):
    segments = 64
    vertices, faces = [], []
    for ring, (distance, radius) in enumerate(profile):
        for index in range(segments):
            angle = math.tau * index / segments
            vertices.append((side * distance, axle + radius * math.cos(angle), .34 + radius * math.sin(angle)))
            if ring:
                a = (ring - 1) * segments + index
                b = (ring - 1) * segments + (index + 1) % segments
                faces.append((a, b, b + segments, a + segments))
    return mesh(entry, name, vertices, faces, material, smooth=True)


def export_with_uv(scene, entry, destination):
    # New evaluated copies only; retain loop UVs unlike frozen v1 merged export.
    graph = bpy.context.evaluated_depsgraph_get()
    objects, meshes = [], []
    for obj in scene.objects:
        obj.select_set(False)
    try:
        for part in entry["objects"]:
            evaluated = part.evaluated_get(graph)
            data = bpy.data.meshes.new_from_object(evaluated, preserve_all_data_layers=True, depsgraph=graph)
            obj = bpy.data.objects.new("Export_DanfoDetail_" + part.name, data)
            scene.collection.objects.link(obj)
            obj.matrix_world = entry["root"].matrix_world.inverted() @ evaluated.matrix_world
            obj.select_set(True)
            objects.append(obj)
            meshes.append(data)
        bpy.context.view_layer.objects.active = objects[0]
        bpy.ops.object.join()
        joined = bpy.context.view_layer.objects.active
        joined.name = "SM_DanfoDetail_v1"
        bpy.ops.export_scene.fbx(filepath=str(destination), use_selection=True, object_types={"MESH"},
                                 global_scale=1, apply_unit_scale=True, apply_scale_options="FBX_SCALE_UNITS",
                                 axis_forward="-Y", axis_up="Z", bake_anim=False,
                                 mesh_smooth_type="FACE", path_mode="AUTO", use_custom_props=False)
        result = {"identity": joined.name, "vertices": len(joined.data.vertices),
                  "faces": len(joined.data.polygons), "uv_layers": [uv.name for uv in joined.data.uv_layers],
                  "materials": [material.name for material in joined.data.materials]}
        bpy.data.objects.remove(joined, do_unlink=True)
        return result
    finally:
        for data in meshes:
            try:
                if data.users == 0:
                    bpy.data.meshes.remove(data)
            except ReferenceError:
                # Object joining may already remove zero-user copied datablocks.
                pass


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--owned-background-job", required=True)
    parser.add_argument("--probe-script", required=True)
    args = parser.parse_args(sys.argv[sys.argv.index("--") + 1:])
    if not bpy.app.background or bpy.data.filepath or os.getpid() == 3224:
        raise RuntimeError("Only a fresh unsaved owned background process may author derivative")
    if args.owned_background_job != JOB or os.environ.get("CRIBTEST_BLENDER_OWNER_JOB") != JOB:
        raise RuntimeError("Derivative ownership handshake mismatch")
    if digest(SOURCE) != SOURCE_HASH or digest(HELPERS) != HELPERS_HASH:
        raise RuntimeError("Frozen source mismatch")
    saved = OUTPUT / "DanfoDetail_v1.blend"
    if saved.exists() or (OUTPUT / "DanfoDetail_v1.fbx").exists():
        raise FileExistsError("Derivative output exists; do not overwrite")
    try:
        progress("appending_frozen_danfo_into_derivative", source=str(SOURCE), source_sha256=SOURCE_HASH)
        namespace = runpy.run_path(str(HELPERS))
        mesh, box, tube, slab = (namespace[name] for name in ("mesh", "box", "tube", "slab"))
        scene = bpy.data.scenes.new("Danfo Detail v1 - Additive Authoring")
        bpy.context.window.scene = scene
        scene.unit_settings.system, scene.unit_settings.scale_length = "METRIC", 1
        scene.unit_settings.length_unit = "METERS"
        with bpy.data.libraries.load(str(SOURCE), link=False) as (available, loaded):
            if "Danfo" not in available.collections:
                raise RuntimeError("Frozen Danfo collection missing")
            loaded.collections = ["Danfo"]
        collection = loaded.collections[0]
        collection.name = "DanfoDetail_v1_Editable"
        scene.collection.children.link(collection)
        root = next(obj for obj in collection.objects if obj.type == "EMPTY")
        root.name = "DanfoDetail_v1_ROOT"
        root["authoring_job"] = JOB
        root["source_asset"] = "Danfo"
        root["placement"] = "unplaced additive derivative; no canonical or account ID"
        root.location = (0, 0, 0)
        collection.instance_offset = (0, 0, 0)
        materials = {mat.name: mat for mat in bpy.data.materials if mat.name in
                     {"DanfoYellow", "YellowFaded", "StripeBlack", "Rubber", "Steel", "GlassDark", "Seat",
                      "LampIvory", "LampAmber", "LampRed", "IvoryPaint"}}
        mesh.__globals__["MATERIALS"] = materials
        entry = {"name": "DanfoDetail_v1", "collection": collection, "root": root, "objects": []}
        removed = []
        for obj in list(collection.objects):
            if obj.type == "MESH" and (obj.name.startswith("Danfo_Tyre_") or obj.name.startswith("Danfo_Hub_")
                or obj.name.startswith("Danfo_WindowSeal_") or obj.name == "Danfo_WindscreenSeal"):
                removed.append(obj.name)
                bpy.data.objects.remove(obj, do_unlink=True)
        entry["objects"] = [obj for obj in collection.objects if obj.type == "MESH"]
        # Replace solid dark backing boxes with open perimeter seals.
        for side in (-1, 1):
            for index, (left, right) in enumerate([(-2.03, -1.20), (-1.13, -.29), (-.22, .63), (.70, 1.48)]):
                for label, centre, size in (
                    ("Lower", (side*.928, (left+right)/2, 1.348), (.032, right-left, .031)),
                    ("Upper", (side*.928, (left+right)/2, 1.972), (.032, right-left, .031)),
                    ("A", (side*.928, left+.018, 1.66), (.032, .036, .65)),
                    ("B", (side*.928, right-.018, 1.66), (.032, .036, .65))):
                    box(entry, "Seal_%s_%s_%s" % (side,index,label), centre, size, "Rubber", .005)
            outer = [(side*.935, 1.51, 1.30), (side*.883, 2.148, 1.30),
                     (side*.827, 1.942, 2.015), (side*.919, 1.51, 2.015)]
            inner = [(side*.941, 1.559, 1.357), (side*.888, 2.090, 1.357),
                     (side*.840, 1.919, 1.958), (side*.925, 1.559, 1.958)]
            for index in range(4):
                following = (index+1)%4
                slab(entry, "CabSeal_%s_%s"%(side,index), [outer[index],outer[following],inner[following],inner[index]],
                     (-side*.025,0,0), "Rubber", .004)
            slab(entry, "CabGlass_%s"%side, inner, (-side*.008,0,0), "GlassDark", .002)
            tube(entry, "CabFrame_%s"%side, [outer[0],outer[3]], .027, "DanfoYellow", 10)
            tube(entry, "CabWindowLatch_%s"%side, [(side*.954,1.60,1.40),(side*.954,1.71,1.40)], .009, "Steel", 8)
        outer = [(-.88,2.17,1.31),(.88,2.17,1.31),(.82,1.94,2.015),(-.82,1.94,2.015)]
        inner = [(-.815,2.179,1.365),(.815,2.179,1.365),(.765,1.968,1.958),(-.765,1.968,1.958)]
        for index in range(4):
            following=(index+1)%4
            slab(entry,"WindscreenPerimeter_%s"%index,[outer[index],outer[following],inner[following],inner[index]],
                 (0,-.026,0),"Rubber",.004)
        for axle in (-1.32,1.30):
            for side in (-1,1):
                wheel_profile(entry,mesh,side,axle)
                ring_x(entry,mesh,"Rim_%s_%s"%(side,axle),side,axle,
                       [(1.062,.190),(1.070,.203),(1.079,.204),(1.086,.190),(1.084,.155),
                        (1.069,.129),(1.063,.055)],"Steel")
                tube(entry,"HubCentre_%s_%s"%(side,axle),[(side*1.062,axle,.34),(side*1.085,axle,.34)],.063,"Steel",32)
                for index in range(8):
                    angle=math.tau*index/8
                    y,z=axle+.150*math.cos(angle),.34+.150*math.sin(angle)
                    tube(entry,"RimVent_%s_%s_%s"%(side,axle,index),[(side*1.077,y,z),(side*1.085,y,z)],.018,"StripeBlack",12)
        # Steering wheel and column are visible through the now open glazing.
        tube(entry,"SteeringColumn",[(-.48,1.87,1.14),(-.48,1.58,1.38)],.029,"StripeBlack",12)
        centre=Vector((-.48,1.58,1.38))
        u,v=Vector((1,0,0)),Vector((0,.73,.68))
        points=[tuple(centre+.18*(math.cos(math.tau*i/40)*u+math.sin(math.tau*i/40)*v)) for i in range(41)]
        tube(entry,"SteeringRim",points,.016,"Rubber",8)
        for angle in (0,math.tau/3,2*math.tau/3):
            end=centre+.165*(math.cos(angle)*u+math.sin(angle)*v)
            tube(entry,"SteeringSpoke_%s"%angle,[tuple(centre),tuple(end)],.011,"StripeBlack",8)
        material_detail(materials)
        for obj in entry["objects"]:
            for modifier in obj.modifiers:
                if modifier.type=="BEVEL" and obj.name not in {"Danfo_Roof"}:
                    modifier.width=min(modifier.width,.012)
        bpy.context.view_layer.update()
        exported=export_with_uv(scene,entry,OUTPUT/"DanfoDetail_v1.fbx")
        stage=bpy.data.collections.new("DetailPreviewOnly_NotWorldPlacement")
        scene.collection.children.link(stage)
        groundmat=bpy.data.materials.new("Neutral authoring ground")
        groundmat.diffuse_color=(.19,.20,.20,1)
        materials["Ground"]=groundmat
        preview={"name":"DetailPreview","collection":stage,"root":None,"objects":[]}
        box(preview,"Ground",(0,0,-.06),(30,30,.12),"Ground",0)
        world=bpy.data.worlds.new("DetailDaylight")
        world.use_nodes=True
        world.node_tree.nodes.get("Background").inputs["Color"].default_value=(.58,.66,.75,1)
        world.node_tree.nodes.get("Background").inputs["Strength"].default_value=.45
        scene.world=world
        light=bpy.data.lights.new("DetailSun","SUN")
        light.energy,light.angle=2,math.radians(8)
        sun=bpy.data.objects.new(light.name,light)
        stage.objects.link(sun)
        sun.rotation_euler=(math.radians(24),math.radians(-32),math.radians(-30))
        views=[]
        for name,eye,target,lens in (
            ("01-Danfo-Detail-Quarter",(5.2,6.8,3.9),(0,0,1),48),
            ("02-Danfo-Cabin-Wheel-Close",(3.8,4.6,2.0),(.3,1.0,1.22),58)):
            data=bpy.data.cameras.new(name)
            camera=bpy.data.objects.new(name,data)
            stage.objects.link(camera)
            camera.location=eye
            camera.rotation_euler=(Vector(target)-camera.location).to_track_quat("-Z","Y").to_euler()
            data.lens=lens
            views.append(camera)
        scene.camera=views[0]
        scene.render.engine="CYCLES"
        scene.cycles.device,scene.cycles.samples="CPU",16
        scene.cycles.use_denoising=True
        scene.cycles.max_bounces,scene.cycles.transmission_bounces=6,4
        scene.render.threads_mode,scene.render.threads="FIXED",2
        scene.render.resolution_x,scene.render.resolution_y=960,540
        scene.render.resolution_percentage=100
        scene.render.image_settings.file_format="PNG"
        scene.render.image_settings.color_mode,scene.render.image_settings.color_depth="RGB","8"
        scene.view_settings.view_transform="AgX"
        scene["authoring_job"]=JOB
        bpy.context.view_layer.update()
        bpy.ops.wm.save_as_mainfile(filepath=str(saved))
        bpy.ops.wm.open_mainfile(filepath=str(saved))
        scene=bpy.data.scenes["Danfo Detail v1 - Additive Authoring"]
        bpy.context.window.scene=scene
        bpy.context.view_layer.update()
        probe=runpy.run_path(args.probe_script)["inspect_scene"](limit=12)
        write_json(OUTPUT/"scene-probe-reopened.json",probe)
        if probe["assets"]["missing_count"]:
            raise RuntimeError("Derivative has missing dependencies")
        manifest={"job":JOB,"pid":os.getpid(),"source_blend":str(SOURCE),"source_sha256":SOURCE_HASH,
                  "helper_script_sha256":HELPERS_HASH,"script_sha256":digest(Path(__file__)),
                  "blend":{"path":str(saved),"sha256":digest(saved)},
                  "fbx":{"path":str(OUTPUT/"DanfoDetail_v1.fbx"),"sha256":digest(OUTPUT/"DanfoDetail_v1.fbx"),**exported},
                  "units":"metres","front":"+Y","ground_contact":"Axle centres unchanged at Z=.34, tyre maximum radius .34m",
                  "canonical_mapping":"Derivative source_asset=Danfo only; no canonical placement/account ID created",
                  "removed_derivative_parts":removed,
                  "changes":["Open perimeter seals instead of solid window backing", "Front-side cab glass/seals/frame/latches",
                             "Profiled tyres with shallow sipes; annular rim, hub and vent details", "Steering rim/spokes/column",
                             "Restrained bevels and paint coat/lower-road-film roughness", "Evaluated FBX preserves metre UV layers"],
                  "material_provenance":"Project authored shader fallback; no licensed scan inputs found or used",
                  "material_limit":"Shader detail is not photographic fidelity; existing opaque rear-window backing remains for subsequent review",
                  "uv_basis":"Planar metre UVs preserved in exported evaluated mesh, not atlas/baked PBR textures",
                  "render":{"engine":"CYCLES","device":"CPU","threads":2,"samples":16,"resolution":[960,540]},
                  "frames":[],"visual_acceptance":"pending actual PNG review","errors":[]}
        write_json(OUTPUT/"provenance.json",manifest)
        progress("saved_exported_reopened_and_probed",blend=manifest["blend"],fbx=manifest["fbx"])
        for name in ("01-Danfo-Detail-Quarter","02-Danfo-Cabin-Wheel-Close"):
            if (OUTPUT/"STOP").exists():
                raise RuntimeError("Owned stop marker between frames")
            scene.camera=scene.objects[name]
            final=OUTPUT/(name+".png")
            temporary=OUTPUT/(name+".partial.png")
            scene.render.filepath=str(temporary)
            progress("rendering",camera=name,frames=manifest["frames"])
            started=time.monotonic()
            bpy.ops.render.render(write_still=True,scene=scene.name)
            seconds=time.monotonic()-started
            image=bpy.data.images.load(str(temporary),check_existing=False)
            try:
                if list(image.size)!=[960,540] or not image.has_data:
                    raise RuntimeError("Derivative render did not decode at requested size")
            finally:
                bpy.data.images.remove(image)
            temporary.replace(final)
            manifest["frames"].append({"path":str(final),"sha256":digest(final),"seconds":seconds,"camera":name})
            write_json(OUTPUT/"provenance.json",manifest)
        if digest(SOURCE)!=SOURCE_HASH or digest(HELPERS)!=HELPERS_HASH:
            raise RuntimeError("Immutable source changed")
        progress("derivative_delivered_pending_image_review",blend=manifest["blend"],fbx=manifest["fbx"],
                 frames=manifest["frames"],errors=[])
    except Exception:
        progress("failed",error=traceback.format_exc())
        raise


if __name__=="__main__":
    main()

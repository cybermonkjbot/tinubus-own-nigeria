import * as THREE from "three";
import { mergeGeometries } from "three/addons/utils/BufferGeometryUtils.js";
import { acquireGeometryVariant, type GeometryLease } from "../../../../../packages/character-runtime/src/geometryLease";
import { cityMiniParkPoint, type CityMiniPark } from "../../../../../packages/utils/src/companionCityMiniParks";
import type { CompanionAssetDelivery } from "../../../../../packages/utils/src/companionAssets";
import { acquireCompanionGltf, type GltfLease, type SceneAsset } from "../appearance/gltfAssets";
import { createLagosAssetPart, disposeLagosAssetPart } from "../lagos-places/assetParts";

type ParkBody = { park: CityMiniPark; root?: THREE.Group; fallback?: THREE.Group; vehicles?: THREE.Group;
  people: THREE.Group[]; shadows: boolean; releaseSign?: () => void };

/** Parked scenery shares the published Danfo/NPC caches. It does not create rides, fares or resident identities. */
export function createCityMiniParks(parks: readonly CityMiniPark[], delivery: () => CompanionAssetDelivery, changed: () => void) {
  const group = new THREE.Group(); group.name = "Danfo Mini parks";
  const bodies: ParkBody[] = parks.map(park => ({ park, people: [], shadows: false }));
  const cube = new THREE.BoxGeometry(1, 1, 1), wheel = new THREE.CylinderGeometry(.39, .39, .22, 14);
  const signPlane = new THREE.PlaneGeometry(1, 1);
  wheel.rotateZ(Math.PI / 2);
  const materials = new Map<string, THREE.MeshStandardMaterial>(), batchLeases = new Map<THREE.Mesh, GeometryLease>();
  const signs = new Map<string, { material: THREE.MeshStandardMaterial; texture?: THREE.CanvasTexture; users: number }>();
  // Park scenery and its one-shot NPC poses keep fixed local transforms.
  // Parent movement and skinning still use the normal world-matrix updates.
  const cacheLocalTransform = (node: THREE.Object3D) => {
    if (!node.matrixAutoUpdate) return;
    node.updateMatrix(); node.matrixAutoUpdate = false;
  };
  const mat = (colour: string, metalness = 0, roughness = .85) => {
    const key = `${colour}:${metalness}:${roughness}`;
    let material = materials.get(key);
    if (!material) { material = new THREE.MeshStandardMaterial({ color: colour, roughness, metalness }); materials.set(key, material); }
    return material;
  };
  const add = (owner: THREE.Group, name: string, x: number, y: number, z: number, w: number, h: number, d: number,
    colour: string, geometry: THREE.BufferGeometry = cube) => {
    const mesh = new THREE.Mesh(geometry, mat(colour)); mesh.name = name;
    mesh.position.set(x, y, z); mesh.scale.set(w, h, d); mesh.receiveShadow = true; mesh.castShadow = h > .08;
    owner.add(mesh); return mesh;
  };
  const batch = (owner: THREE.Group) => {
    const bins = new Map<THREE.Material, THREE.Mesh[]>();
    for (const child of [...owner.children]) {
      if (!(child instanceof THREE.Mesh) || child.geometry !== cube) continue;
      child.updateMatrix();
      const material = child.material as THREE.Material, values = bins.get(material) ?? [];
      values.push(child); bins.set(material, values); owner.remove(child);
    }
    for (const [material, parts] of bins) {
      // Geometry depends on these ordered local matrices, not the bus colour
      // or the park's placement. Equal batches share until their last release.
      const key = parts.map(part => part.matrix.elements.map(value => Object.is(value, -0) ? "-0" : value).join(",")).join(";");
      const lease = acquireGeometryVariant(cube, key, () => {
        const copies: THREE.BufferGeometry[] = [];
        try {
          for (const part of parts) {
            const copy = cube.clone(); copies.push(copy); copy.applyMatrix4(part.matrix);
          }
          const geometry = mergeGeometries(copies);
          if (!geometry) throw new Error("Mini park geometry could not be assembled.");
          return geometry;
        } finally { copies.forEach(copy => copy.dispose()); }
      });
      let mesh: THREE.Mesh | undefined;
      try {
        mesh = new THREE.Mesh(lease.geometry, material);
        mesh.castShadow = true; mesh.receiveShadow = true; owner.add(mesh);
        batchLeases.set(mesh, lease);
      } catch (error) { mesh?.removeFromParent(); lease.release(); throw error; }
    }
  };
  const releaseBatch = (node: THREE.Object3D) => {
    if (!(node instanceof THREE.Mesh)) return;
    const lease = batchLeases.get(node);
    if (lease) { batchLeases.delete(node); lease.release(); }
  };
  const sign = (owner: THREE.Group, text: string, x: number, y: number, z: number, width: number) => {
    let resource = signs.get(text);
    if (!resource) {
      let texture: THREE.CanvasTexture | undefined;
      if (typeof document !== "undefined") {
        const canvas = document.createElement("canvas"); canvas.width = 768; canvas.height = 160;
        const context = canvas.getContext("2d");
        if (context) {
          context.fillStyle = "#d9ba58"; context.fillRect(0, 0, 768, 160);
          context.fillStyle = "#29382e"; context.textAlign = "center"; context.textBaseline = "middle";
          context.font = "bold 54px sans-serif"; context.fillText(text, 384, 80, 720);
          texture = new THREE.CanvasTexture(canvas); texture.colorSpace = THREE.SRGBColorSpace;
        }
      }
      const material = new THREE.MeshStandardMaterial({ color: texture ? "#ffffff" : "#d9ba58", map: texture, roughness: .8, side: THREE.DoubleSide });
      resource = { material, texture, users: 0 }; signs.set(text, resource);
    }
    const panel = new THREE.Mesh(signPlane, resource.material); panel.name = text;
    panel.position.set(x, y, z + .04); panel.scale.set(width, .65, 1); owner.add(panel);
    const owned = resource; owned.users++;
    let released = false;
    return () => {
      if (released) return; released = true;
      if (--owned.users > 0) return;
      signs.delete(text); owned.material.dispose(); owned.texture?.dispose();
      // This factory owns the canvas, and its last sign has left the scene.
      if (owned.texture) { owned.texture.image.width = 0; owned.texture.image.height = 0; }
    };
  };
  const fallbackBus = (owner: THREE.Group, variant: number) => {
    const yellow = ["#dab13a", "#e7bc31", "#cfa335"][variant % 3];
    add(owner, "Danfo lower body", 0, .86, 0, 1.9, .75, 4.8, yellow);
    add(owner, "Danfo cabin glazing", 0, 1.49, -.08, 1.78, .68, 4.52, "#304a4d");
    add(owner, "Danfo roof", 0, 1.99, -.1, 1.94, .14, 4.65, yellow);
    for (const side of [-1, 1]) {
      for (const z of [-2.1, -1.2, -.3, .65, 1.65]) add(owner, "Window pillar", side * .91, 1.5, z, .065, .8, .085, yellow);
      for (const y of [.88, 1.01]) add(owner, "Danfo black stripe", side * .957, y, 0, .02, .055, 4.6, "#252c28");
      for (const z of [-1.4, 1.35]) add(owner, "Tyre", side * .95, .39, z, 1, 1, 1, "#252c28", wheel);
      add(owner, "Mirror", side * 1.06, 1.58, 1.85, .16, .19, .12, "#3e4942");
      add(owner, "Headlamp", side * .63, .93, 2.413, .32, .18, .025, "#dedbc7");
      add(owner, "Rear light", side * .7, .9, -2.413, .17, .22, .025, "#874c3e");
    }
    add(owner, "Split windscreen", 0, 1.52, 2.285, .045, .68, .06, yellow);
    for (const z of [-2.43, 2.43]) add(owner, "Bumper", 0, .53, z, 1.94, .15, .12, "#414a43");
    batch(owner);
  };
  let provider: CompanionAssetDelivery | undefined, busLease: GltfLease | undefined, npcLease: GltfLease | undefined;
  let busAsset: SceneAsset | undefined, npcAsset: SceneAsset | undefined, disposed = false, dirty = false;
  let busMeshes: THREE.Mesh[] | undefined;
  let failedBus = false, failedNpc = false, near = false;
  const wake = () => { dirty = true; changed(); };
  const clearPeople = (body: ParkBody) => {
    body.people.forEach(disposeLagosAssetPart);
    body.people.length = 0;
  };
  const clearFallback = (body: ParkBody) => {
    const fallback = body.fallback;
    if (!fallback) return;
    fallback.removeFromParent();
    fallback.traverse(releaseBatch);
    fallback.clear(); body.fallback = undefined;
  };
  const ensureFallback = (body: ParkBody) => {
    if (!body.root || body.vehicles || body.fallback) return;
    const fallback = new THREE.Group(); body.fallback = fallback;
    try {
      for (const bus of body.park.buses) {
        const vehicle = new THREE.Group(); vehicle.name = "Parked Danfo";
        vehicle.position.set(bus.x, 0, bus.z); vehicle.rotation.y = bus.yaw;
        fallback.add(vehicle); fallbackBus(vehicle, bus.variant);
      }
      fallback.traverse(node => { cacheLocalTransform(node); if (node instanceof THREE.Mesh) node.castShadow = body.shadows; });
      body.root.add(fallback);
    } catch (error) { clearFallback(body); throw error; }
  };
  const clearBody = (body: ParkBody) => {
    clearPeople(body); clearFallback(body);
    (body.root?.userData.access as THREE.Mesh | undefined)?.removeFromParent();
    body.root?.traverse(node => {
      if (node instanceof THREE.InstancedMesh) node.dispose();
      releaseBatch(node);
    });
    body.root?.removeFromParent(); body.root?.clear(); body.root = undefined; body.vehicles = undefined;
    body.releaseSign?.(); body.releaseSign = undefined;
  };
  const clearBuses = () => {
    for (const body of bodies) {
      body.vehicles?.traverse(node => { if (node instanceof THREE.InstancedMesh) node.dispose(); });
      body.vehicles?.removeFromParent(); body.vehicles = undefined;
    }
    busAsset = undefined; busMeshes = undefined; busLease?.release(); busLease = undefined;
    // Restore every visible park in the same update, including those already
    // visited by the update loop when a later park's model assembly failed.
    bodies.forEach(ensureFallback);
  };
  const release = () => {
    bodies.forEach(clearBody); busAsset = undefined; busMeshes = undefined; npcAsset = undefined;
    busLease?.release(); npcLease?.release(); busLease = undefined; npcLease = undefined;
    batchLeases.forEach(lease => lease.release()); batchLeases.clear();
  };
  const build = (body: ParkBody) => {
    const park = body.park, root = new THREE.Group();
    root.name = park.name; root.position.set(park.x, 0, park.z); root.rotation.y = park.yaw;
    root.userData.miniParkId = park.id; body.root = root;
    add(root, "Bus park hardstand", 0, -.038, 0, park.width, .08, park.depth, "#817b68");
    // The access lane stays flush and clear, including the road-facing entrance.
    const entry = cityMiniParkPoint(park, 0, 9), dx = park.approach[0] - entry[0], dz = park.approach[2] - entry[2];
    const access = new THREE.Mesh(cube, mat("#948c77")); access.name = "Mini park access";
    access.position.set((entry[0] + park.approach[0]) / 2, -.032, (entry[2] + park.approach[2]) / 2);
    access.scale.set(5.5, .065, Math.hypot(dx, dz)); access.rotation.y = Math.atan2(dx, dz); group.add(access);
    cacheLocalTransform(access);
    root.userData.access = access;
    for (const bus of park.buses) {
      for (const side of [-1, 1]) add(root, "Parking bay stripe", bus.x + side * 1.45, .012, bus.z, .05, .015, 5.8, "#d9cfaa");
    }
    for (const z of [-2.3, 2.3]) {
      add(root, "Waiting bench", -10.1, .46, z, .8, .08, 1.8, "#65715b");
      for (const leg of [-.62, .62]) add(root, "Bench leg", -10.1, .23, z + leg, .55, .46, .07, "#4f5d50");
    }
    // Open-sided cover and upright sign occupy the same registered waiting-area reserve.
    add(root, "Waiting canopy", -9.7, 2.65, 0, 2.1, .1, 6.6, "#a4966d");
    for (const z of [-3.2, 3.2]) add(root, "Canopy post", -10.1, 1.3, z, .1, 2.6, .1, "#4f5d50");
    add(root, "Sign post", -10.1, 1.5, 0, .1, 3, .1, "#4f5d50");
    add(root, "Waste bin", 10.2, .42, -.5, .65, .84, .65, "#566956");
    batch(root); body.releaseSign = sign(root, park.name, -8.7, 3, 0, 4.2);
    root.traverse(node => { cacheLocalTransform(node); if (node instanceof THREE.Mesh) node.castShadow = body.shadows; });
    group.add(root);
  };
  const addBuses = (body: ParkBody) => {
    if (!busAsset || !body.root || body.vehicles) return;
    // The leased source is immutable. Inspect it once, not for each nearby park.
    if (!busMeshes) {
      busAsset.scene.updateMatrixWorld(true);
      let valid = false;
      const meshes: THREE.Mesh[] = [];
      busAsset.scene.traverse(node => {
        if (node.userData.modelId === "danfo" && node.userData.front === "+Z" && node.userData.units === "metres") valid = true;
        if (node instanceof THREE.Mesh && !(node instanceof THREE.SkinnedMesh)) meshes.push(node);
      });
      if (!valid) throw new Error("Parked Danfo coordinate metadata differs.");
      if (!meshes.length) throw new Error("Parked Danfo asset has no geometry.");
      busMeshes = meshes;
    }
    const vehicles = new THREE.Group(); vehicles.name = "Published parked Danfos";
    const matrix = new THREE.Matrix4(), position = new THREE.Vector3(), rotation = new THREE.Quaternion();
    const up = new THREE.Vector3(0, 1, 0), scale = new THREE.Vector3(1, 1, 1);
    const placements = body.park.buses.map(bus => new THREE.Matrix4().compose(
      position.set(bus.x, 0, bus.z), rotation.setFromAxisAngle(up, bus.yaw), scale));
    try {
      for (const source of busMeshes) {
        const mesh = new THREE.InstancedMesh(source.geometry, source.material, placements.length);
        vehicles.add(mesh);
        for (let index = 0; index < placements.length; index++) {
          mesh.setMatrixAt(index, matrix.multiplyMatrices(placements[index], source.matrixWorld));
        }
        mesh.instanceMatrix.needsUpdate = true; mesh.computeBoundingSphere(); mesh.receiveShadow = true; mesh.castShadow = body.shadows;
      }
    } catch (error) {
      vehicles.traverse(node => { if (node instanceof THREE.InstancedMesh) node.dispose(); });
      vehicles.clear(); throw error;
    }
    vehicles.traverse(cacheLocalTransform);
    body.root.add(vehicles); body.vehicles = vehicles; clearFallback(body);
  };
  const addPeople = (body: ParkBody) => {
    if (!npcAsset || !body.root || body.people.length) return;
    // Two conductors face the approach; waiting customers stand in the wide inter-bus aisle.
    for (const [index, point] of [[3.5, 6.6], [-3.5, 6.6], [-3.6, -.9], [3.6, -.9]].entries()) {
      const npcId = index % 2 ? "street-tout-shirt" : "street-tout-cap";
      const part = createLagosAssetPart({ id: `${body.park.id}:waiting:${index}`, key: "bestieTownSquareNpcs", npcId,
        point: [point[0], 0, point[1]], yaw: index < 2 ? 0 : Math.PI / 2, sourceYaw: -Math.PI / 2 }, npcAsset);
      const actor = part.actor!, mixer = new THREE.AnimationMixer(actor);
      try {
        const clip = part.clips.find(clip => clip.name === `${npcId}_${index < 2 ? "gesture" : "idle"}`)!;
        const action = mixer.clipAction(clip); action.play(); action.time = Math.min(.55, clip.duration * .3); mixer.update(0);
        part.group.traverse(node => {
          cacheLocalTransform(node); node.userData = { ambientMiniParkId: body.park.id };
          if (node instanceof THREE.Mesh) node.castShadow = body.shadows;
        });
        part.group.name = index < 2 ? "Waiting conductor" : "Waiting customer";
        // These poses never animate. Retain only the posed scene; the one-shot
        // mixer and its bindings can be collected. Stopping would reset the pose.
        body.root.add(part.group); body.people.push(part.group);
      } catch (error) { mixer.stopAllAction(); mixer.uncacheRoot(actor); disposeLagosAssetPart(part.group); throw error; }
    }
  };
  return { group,
    update(target: THREE.Vector3, daylight: number) {
      if (disposed || !Number.isFinite(target.x) || !Number.isFinite(target.z)) return false;
      const next = delivery();
      if (next !== provider) { release(); provider = next; failedBus = false; failedNpc = false; dirty = true; }
      if (busLease && !next.available("bestieVehicleDanfo")) { clearBuses(); dirty = true; }
      if (npcLease && !next.available("bestieTownSquareNpcs")) {
        bodies.forEach(clearPeople); npcAsset = undefined; npcLease.release(); npcLease = undefined; dirty = true;
      }
      const active = bodies.some(body => Math.hypot(body.park.x - target.x, body.park.z - target.z) < 300);
      if (active && !near) { failedBus = false; failedNpc = false; } near = active;
      if (!active && (busLease || npcLease)) { release(); dirty = true; }
      if (active && !busLease && !failedBus && next.available("bestieVehicleDanfo")) {
        const lease = acquireCompanionGltf("bestieVehicleDanfo", next); busLease = lease;
        lease.promise.then(asset => { if (disposed || busLease !== lease) return; busAsset = asset; wake(); })
          .catch(() => { if (disposed || busLease !== lease) return; busLease.release(); busLease = undefined; failedBus = true; wake(); });
      }
      if (active && !npcLease && !failedNpc && next.available("bestieTownSquareNpcs")) {
        const lease = acquireCompanionGltf("bestieTownSquareNpcs", next); npcLease = lease;
        lease.promise.then(asset => { if (disposed || npcLease !== lease) return; npcAsset = asset; wake(); })
          .catch(() => { if (disposed || npcLease !== lease) return; npcLease.release(); npcLease = undefined; failedNpc = true; wake(); });
      }
      for (const body of bodies) {
        const distance = Math.hypot(body.park.x - target.x, body.park.z - target.z);
        if (body.root && distance > 300) { clearBody(body); dirty = true; }
        if (!body.root && distance < 240) {
          try { build(body); } catch (error) { clearBody(body); throw error; }
          dirty = true;
        }
        const shadows = daylight > .12 && distance < 150;
        if (body.root && shadows !== body.shadows) {
          body.shadows = shadows; body.root.traverse(node => { if (node instanceof THREE.Mesh) node.castShadow = shadows; }); dirty = true;
        }
        try { if (body.root && busAsset && !body.vehicles) { addBuses(body); dirty = true; } }
        catch { failedBus = true; clearBuses(); dirty = true; }
        if (body.root && !body.vehicles && !body.fallback) { ensureFallback(body); dirty = true; }
        try { if (body.root && npcAsset && !body.people.length) { addPeople(body); dirty = true; } }
        catch { bodies.forEach(clearPeople); failedNpc = true; npcAsset = undefined; npcLease?.release(); npcLease = undefined; }
      }
      const result = dirty; dirty = false; return result;
    },
    dispose() {
      if (disposed) return; disposed = true; release(); group.removeFromParent(); group.clear();
      cube.dispose(); wheel.dispose(); signPlane.dispose(); materials.forEach(material => material.dispose()); materials.clear();
    },
  };
}

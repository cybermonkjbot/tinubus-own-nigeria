import * as THREE from "three";
import design from "../../../../../packages/assets/city/lagos-billboards-v1.json";
import { validCompanionBillboardAd, isCompanionAdBillboardId, type CompanionBillboardAd, type CompanionBillboardVisibility } from "../../../../../packages/utils/src/companionBillboardAds";
import type { ArchitecturePlacement } from "./hostedArchitecture";

type Screen = { root: THREE.Group; mesh: THREE.Mesh<THREE.PlaneGeometry, THREE.MeshStandardMaterial>; width: number; height: number };
type Artwork = { canvas: HTMLCanvasElement; texture: THREE.CanvasTexture };
const corners = [-.5, .5] as const;
const exposurePoints = [[0, 0], [-.35, -.35], [.35, -.35], [-.35, .35], [.35, .35]] as const;

/** Static panels retain their authored mounts, dimensions and normal. They never face or follow the camera. */
export function createCityBillboardAds(mains: Readonly<{ value: number }>, changed: () => void) {
  const screens = new Map<string, Screen>();
  let ad: CompanionBillboardAd | null = null, artwork: Artwork | null = null;
  let generation = 0, disposed = false, power = 1, daylight = 1;
  const projected = new THREE.Vector3(), point = new THREE.Vector3(), cameraPoint = new THREE.Vector3();
  const normal = new THREE.Vector3(), direction = new THREE.Vector3(), rotation = new THREE.Quaternion(), raycaster = new THREE.Raycaster();
  const view = new THREE.Frustum(), viewMatrix = new THREE.Matrix4(), candidates: THREE.Mesh[] = [];
  const intersections: THREE.Intersection[] = [];
  const occludes = (hit: THREE.Intersection) => {
    const mesh = hit.object as THREE.Mesh;
    const material = Array.isArray(mesh.material) ? mesh.material[hit.face?.materialIndex ?? 0] : mesh.material;
    return material?.visible !== false && (material?.opacity ?? 1) > .05;
  };
  function releaseTexture() {
    if (!artwork) return;
    const released = artwork; artwork = null;
    for (const screen of screens.values()) {
      const material = screen.mesh.material;
      if (material.map === released.texture || material.emissiveMap === released.texture) {
        screen.mesh.visible = false; material.map = null; material.emissiveMap = null; material.needsUpdate = true;
      }
    }
    released.texture.dispose();
    // This canvas belongs only to this display. Drop its backing store as well.
    released.canvas.width = 0; released.canvas.height = 0;
  }
  function artworkTexture(image: HTMLImageElement, creative: CompanionBillboardAd): Artwork | null {
    const canvas = document.createElement("canvas"); canvas.width = 1280; canvas.height = 320;
    const context = canvas.getContext("2d"); if (!context) { canvas.width = 0; canvas.height = 0; return null; }
    context.fillStyle = "#101820"; context.fillRect(0, 0, canvas.width, canvas.height);
    context.fillStyle = "#bdc5ce"; context.font = "32px Arial, sans-serif"; context.fillText("Sponsored", 38, 43);
    context.fillStyle = "#ffffff"; context.font = "bold 46px Arial, sans-serif";
    context.fillText(creative.advertiser, 38, 95, 610);
    context.font = "42px Arial, sans-serif";
    const words = creative.title.trim().split(/\s+/); let line = "", y = 162;
    for (const word of words) {
      if (line && context.measureText(`${line} ${word}`).width > 595) {
        context.fillText(line, 38, y, 595); y += 43; line = word;
        if (y > 248) { line = ""; break; }
      } else line = line ? `${line} ${word}` : word;
    }
    if (line && y <= 248) context.fillText(line, 38, y, 595);
    const scale = Math.min(560 / image.naturalWidth, 272 / image.naturalHeight);
    const width = image.naturalWidth * scale, height = image.naturalHeight * scale;
    context.drawImage(image, 680 + (560 - width) / 2, 24 + (272 - height) / 2, width, height);
    const value = new THREE.CanvasTexture(canvas); value.colorSpace = THREE.SRGBColorSpace;
    return { canvas, texture: value };
  }
  function refresh() {
    power = THREE.MathUtils.clamp(Number.isFinite(mains.value) ? mains.value : 0, 0, 1) * (.75 + .25 * (1 - daylight));
    const texture = artwork?.texture ?? null;
    for (const [boardId, screen] of screens) {
      const showing = !!texture && ad?.boardId === boardId && ad.expiresAt > Date.now();
      screen.mesh.visible = showing;
      const nextTexture = showing ? texture : null;
      if (screen.mesh.material.map !== nextTexture) screen.mesh.material.needsUpdate = true;
      screen.mesh.material.map = nextTexture;
      screen.mesh.material.emissiveMap = nextTexture;
      screen.mesh.material.emissiveIntensity = .8 * power;
      screen.mesh.material.color.setScalar(0);
    }
  }
  return {
    mounted(placement: ArchitecturePlacement, root: THREE.Group) {
      const spec = design.placements.find(value => value.assetKey === placement.assetKey);
      if (!spec || !isCompanionAdBillboardId(spec.id)) return undefined;
      const material = new THREE.MeshStandardMaterial({ color: "#000000", emissive: "#ffffff", roughness: .5, toneMapped: true });
      const mesh = new THREE.Mesh(new THREE.PlaneGeometry(spec.width, spec.height), material);
      mesh.name = `${spec.label} brand display`; mesh.position.set(spec.x, spec.screenBottom + spec.height / 2, spec.screenZ + .085);
      mesh.visible = false; mesh.castShadow = false; mesh.receiveShadow = false; root.add(mesh);
      // The opaque panel covers only the display. Original art and the status light stay intact underneath.
      const screen: Screen = { root, mesh, width: spec.width, height: spec.height };
      screens.set(spec.id, screen); refresh();
      return () => { screens.delete(spec.id); mesh.removeFromParent(); mesh.geometry.dispose(); material.dispose(); };
    },
    setAd(next?: CompanionBillboardAd | null) {
      if (next === ad && (!ad || ad.expiresAt > Date.now())) return;
      const value = validCompanionBillboardAd(next) && next.expiresAt > Date.now() ? next : null;
      if (value === ad) return;
      ad = value; const request = ++generation; releaseTexture(); refresh(); changed();
      if (!value) return;
      const image = new Image();
      image.onload = () => {
        if (disposed || generation !== request || !image.naturalWidth || !image.naturalHeight || value.expiresAt <= Date.now()) return;
        artwork = artworkTexture(image, value); refresh(); changed();
      };
      image.onerror = () => { if (!disposed && generation === request) { releaseTexture(); refresh(); changed(); } };
      image.src = value.imageDataUri;
    },
    update(value: number) { daylight = THREE.MathUtils.clamp(Number.isFinite(value) ? value : 1, 0, 1); refresh(); },
    get wakeAfter() { return ad && artwork ? 250 : Infinity; },
    picked(object: THREE.Object3D) { return ad && screens.get(ad.boardId)?.mesh === object ? ad : null; },
    visibility(scene: THREE.Scene, camera: THREE.PerspectiveCamera, width: number, height: number): CompanionBillboardVisibility | null {
      if (!ad) return null;
      const result = { boardId: ad.boardId, ticketId: ad.ticketId, visible: false };
      const screen = screens.get(ad.boardId);
      if (!screen?.mesh.visible || !artwork || power < .2 || ad.expiresAt <= Date.now() || width <= 0 || height <= 0) return result;
      for (let owner: THREE.Object3D | null = screen.root; owner; owner = owner.parent) if (!owner.visible) return result;
      camera.getWorldPosition(cameraPoint); screen.mesh.getWorldQuaternion(rotation);
      normal.set(0, 0, 1).applyQuaternion(rotation); screen.mesh.getWorldPosition(point);
      if (normal.dot(direction.copy(cameraPoint).sub(point).normalize()) < .35) return result;
      let minX = Infinity, minY = Infinity, maxX = -Infinity, maxY = -Infinity;
      for (const x of corners) for (const y of corners) {
        projected.set(x * screen.width, y * screen.height, 0); screen.mesh.localToWorld(projected); projected.project(camera);
        if (projected.z < -1 || projected.z > 1) return result;
        const px = (projected.x + 1) * width / 2, py = (1 - projected.y) * height / 2;
        minX = Math.min(minX, px); minY = Math.min(minY, py); maxX = Math.max(maxX, px); maxY = Math.max(maxY, py);
      }
      const area = (maxX - minX) * (maxY - minY);
      const visibleArea = Math.max(0, Math.min(width, maxX) - Math.max(0, minX)) * Math.max(0, Math.min(height, maxY) - Math.max(0, minY));
      if (maxX - minX < 320 || maxY - minY < 72 || visibleArea < area * .5) return result;
      // Traverse visible geometry once, not the entire city for each exposure ray.
      try {
        view.setFromProjectionMatrix(viewMatrix.multiplyMatrices(camera.projectionMatrix, camera.matrixWorldInverse));
        scene.traverseVisible(object => {
          if (object instanceof THREE.Mesh && view.intersectsObject(object)) candidates.push(object);
        });
        let clear = 0;
        for (const [x, y] of exposurePoints) {
          point.set(x * screen.width, y * screen.height, 0); screen.mesh.localToWorld(point);
          projected.copy(point).project(camera);
          if (Math.abs(projected.x) > 1 || Math.abs(projected.y) > 1) continue;
          projected.copy(point).sub(cameraPoint); raycaster.far = projected.length() + .02;
          raycaster.set(cameraPoint, projected.normalize());
          intersections.length = 0;
          const hit = raycaster.intersectObjects(candidates, false, intersections).find(occludes);
          if (hit?.object === screen.mesh) clear++;
        }
        return { ...result, visible: clear >= 3 };
      } finally {
        // Scratch must not retain detached city meshes or their leased assets.
        candidates.length = 0; intersections.length = 0;
      }
    },
    dispose() { disposed = true; generation++; ad = null; releaseTexture(); screens.clear(); candidates.length = 0; intersections.length = 0; },
  };
}

import * as THREE from "three";
import { mergeGeometries } from "three/addons/utils/BufferGeometryUtils.js";
import { racingPoints, racingRules } from "../../../../../packages/utils/src/companionRacing";

/** Shared metre-scale circuit. Road and flush edges retain the driving contract. */
export function createCircuitGeometry() {
  const group = new THREE.Group(); group.name = "Lagos Circuit";
  const geometries = new Set<THREE.BufferGeometry>(), materials = new Set<THREE.Material>(), textures = new Set<THREE.Texture>();
  const batches = new Map<THREE.Material, THREE.BufferGeometry[]>();
  const material = (color: string, roughness = .9, metalness = 0) => {
    const value = new THREE.MeshStandardMaterial({ color, roughness, metalness }); materials.add(value); return value;
  };
  const asphalt = material("#9c9f9d"), grass = material("#8f9c74"), concrete = material("#c2c2b3");
  const white = material("#edece2"), dark = material("#263632"), steel = material("#a6ada9", .42, .65);
  const paint = material("#385e53", .7), glass = material("#344c52", .23, .4), bark = material("#6c6650");
  const leaves = [material("#425d3f"), material("#58754b"), material("#71845a")];
  const cube = new THREE.BoxGeometry(1, 1, 1), sphere = new THREE.IcosahedronGeometry(1, 1);
  geometries.add(cube); geometries.add(sphere);
  const transform = new THREE.Object3D();
  const append = (geometry: THREE.BufferGeometry, mat: THREE.Material, x: number, y: number, z: number, w: number, h: number, d: number, yaw = 0) => {
    transform.position.set(x, y, z); transform.scale.set(w, h, d); transform.rotation.set(0, yaw, 0); transform.updateMatrix();
    const copy = geometry.clone().applyMatrix4(transform.matrix), batch = batches.get(mat) ?? []; batch.push(copy); batches.set(mat, batch);
  };
  const box = (x: number, y: number, z: number, w: number, h: number, d: number, mat: THREE.Material, yaw = 0) => append(cube, mat, x, y, z, w, h, d, yaw);
  const texture = (width: number, height: number, draw: (ctx: CanvasRenderingContext2D) => void) => {
    const canvas = document.createElement("canvas"); canvas.width = width; canvas.height = height;
    const context = canvas.getContext("2d"); if (context) draw(context);
    const map = new THREE.CanvasTexture(canvas); map.colorSpace = THREE.SRGBColorSpace; map.anisotropy = 4; textures.add(map); return map;
  };
  // Deterministic aggregate, tiled in world metres rather than once per segment.
  const roadMap = texture(128, 128, ctx => {
    const pixels = ctx.createImageData(128, 128); let seed = 4711;
    for (let i = 0; i < pixels.data.length; i += 4) {
      seed = (Math.imul(seed, 1664525) + 1013904223) >>> 0; const value = 88 + (seed >>> 28);
      pixels.data[i] = value; pixels.data[i + 1] = value + 2; pixels.data[i + 2] = value + 1; pixels.data[i + 3] = 255;
    }
    ctx.putImageData(pixels, 0, 0);
  });
  roadMap.wrapS = roadMap.wrapT = THREE.RepeatWrapping; asphalt.map = roadMap;
  box(0, -.065, 0, racingRules.halfX * 2, .1, racingRules.halfZ * 2, grass);

  // Mitered strip joins remove overlapping road slabs and corner flicker.
  const points = racingPoints.slice(0, -1), count = points.length;
  const offsets = points.map((point, i) => {
    const before = points[(i + count - 1) % count], after = points[(i + 1) % count];
    const incoming = new THREE.Vector2(point[0] - before[0], point[2] - before[2]).normalize();
    const outgoing = new THREE.Vector2(after[0] - point[0], after[2] - point[2]).normalize();
    const normal = new THREE.Vector2(outgoing.y, -outgoing.x);
    const miter = new THREE.Vector2(incoming.y, -incoming.x).add(normal).normalize();
    return miter.multiplyScalar(1 / miter.dot(normal));
  });
  const strip = (low: number, high: number, y: number, mat: THREE.Material, segments = 1, alternate?: THREE.Material) => {
    for (let i = 0; i < count; i++) {
      const j = (i + 1) % count, a = points[i], b = points[j];
      const parts = segments === 1 ? 1 : Math.ceil(Math.hypot(b[0] - a[0], b[2] - a[2]) / segments);
      for (let part = 0; part < parts; part++) {
        const positions: number[] = [], uvs: number[] = [];
        for (const t of [part / parts, (part + 1) / parts]) for (const lateral of [low, high]) {
          const x = THREE.MathUtils.lerp(a[0] + offsets[i].x * lateral, b[0] + offsets[j].x * lateral, t);
          const z = THREE.MathUtils.lerp(a[2] + offsets[i].y * lateral, b[2] + offsets[j].y * lateral, t);
          positions.push(x, y, z); uvs.push(x / 1.5, z / 1.5);
        }
        const geometry = new THREE.BufferGeometry(); geometry.setAttribute("position", new THREE.Float32BufferAttribute(positions, 3));
        geometry.setAttribute("uv", new THREE.Float32BufferAttribute(uvs, 2)); geometry.setIndex([0, 2, 1, 1, 2, 3]); geometry.computeVertexNormals();
        const selected = alternate && part % 2 ? alternate : mat, batch = batches.get(selected) ?? []; batch.push(geometry); batches.set(selected, batch);
      }
    }
  };
  strip(-racingRules.halfWidth, racingRules.halfWidth, .012, asphalt);
  for (const side of [-1, 1]) {
    const ordered = (a: number, b: number) => [Math.min(a * side, b * side), Math.max(a * side, b * side)];
    const edge = ordered(7.1, 7.25), kerb = ordered(7.4, 8), runoff = ordered(8.02, 9.7);
    strip(edge[0], edge[1], .017, white); strip(kerb[0], kerb[1], .019, white, 2.4, dark);
    strip(runoff[0], runoff[1], .002, concrete);
  }
  // Grid boxes and checker finish remain at the server's exact x = -65 line.
  for (let x = 0; x < 2; x++) for (let z = 0; z < 16; z++) box(-65 + x * .7, .025, 64.5 + z, .7, .008, 1, (x + z) % 2 ? dark : white);
  for (let i = 0; i < racingRules.players; i++) {
    const x = -84 - Math.floor(i / 2) * 5, z = 72 + (i % 2 ? 2 : -2);
    for (const side of [-1, 1]) box(x, .023, z + side * 1.2, 3.6, .008, .09, white);
    box(x + 1.8, .023, z, .09, .008, 2.4, white);
  }
  box(0, .005, 86, 24, .025, 7, concrete);
  const rubberMark = material("#434848");
  for (const z of [69, 74]) box(79, .02, z, 18, .002, .16, rubberMark);

  const signMaterial = (label: string, small: string) => {
    const map = texture(1024, 128, ctx => {
      ctx.fillStyle = "#244139"; ctx.fillRect(0, 0, 1024, 128);
      ctx.fillStyle = "#ebeadd"; ctx.font = "600 49px system-ui"; ctx.textAlign = "center"; ctx.fillText(label, 512, 59);
      ctx.fillStyle = "#c1d0c8"; ctx.font = "22px system-ui"; ctx.fillText(small, 512, 97);
      for (let i = 0; i < 4; i++) for (let j = 0; j < 4; j++) if ((i + j) % 2 === 0) {
        ctx.fillStyle = "#d5d9ca"; ctx.fillRect(16 + i * 18, 28 + j * 18, 18, 18); ctx.fillRect(936 + i * 18, 28 + j * 18, 18, 18);
      }
    });
    const mat = new THREE.MeshStandardMaterial({ map, roughness: .85, side: THREE.DoubleSide }); materials.add(mat); return mat;
  };
  const gantry = (label: string, subtitle: string) => {
    const gate = new THREE.Group(); group.add(gate);
    const part = (x: number, y: number, z: number, w: number, h: number, d: number, mat: THREE.Material) => {
      const mesh = new THREE.Mesh(cube, mat); mesh.position.set(x, y, z); mesh.scale.set(w, h, d); mesh.castShadow = true; gate.add(mesh); return mesh;
    };
    for (const x of [-9.8, 9.8]) {
      part(x, .12, 0, .7, .24, .7, concrete);
      for (const z of [-.23, .23]) part(x, 2.7, z, .095, 5.4, .095, steel);
      for (let y = .6; y < 5; y += .65) part(x, y, 0, .075, .075, .53, steel);
    }
    for (const y of [4.85, 5.45]) for (const z of [-.23, .23]) part(0, y, z, 19.7, .08, .08, steel);
    for (let x = -9; x <= 9; x++) { const brace = part(x, 5.15, .23, .07, .83, .07, steel); brace.rotation.z = x % 2 ? -.85 : .85; }
    const geometry = new THREE.PlaneGeometry(17, .95); geometries.add(geometry);
    const bannerMaterial = signMaterial(label, subtitle); bannerMaterial.side = THREE.FrontSide;
    for (const side of [-1, 1]) {
      const banner = new THREE.Mesh(geometry, bannerMaterial); banner.position.set(0, 5.14, side * .29);
      banner.rotation.y = side === -1 ? Math.PI : 0; gate.add(banner);
    }
    // Gate transforms still move as one checkpoint, but metal members share a draw.
    gate.updateMatrixWorld(true);
    for (const mat of [steel, concrete]) {
      const parts = gate.children.filter((child): child is THREE.Mesh => child instanceof THREE.Mesh && child.material === mat);
      const copies = parts.map(child => child.geometry.clone().applyMatrix4(child.matrix));
      const merged = mergeGeometries(copies, false); copies.forEach(value => value.dispose());
      parts.forEach(child => gate.remove(child));
      if (merged) { geometries.add(merged); const mesh = new THREE.Mesh(merged, mat); mesh.castShadow = true; gate.add(mesh); }
    }
    return gate;
  };
  const start = gantry("LAGOS CIRCUIT", "START / FINISH"); start.position.set(-65, 0, 72); start.rotation.y = Math.PI / 2;
  const checkpoint = gantry("CHECKPOINT", "LAGOS CIRCUIT"); checkpoint.name = "Next race checkpoint";

  // Buildings remain beyond the venue's admitted walking bounds.
  for (let row = 0; row < 5; row++) {
    const y = row * .55 + .28, z = -101 - row * 1.1;
    box(0, y, z, 75, .5, 1.1, concrete);
    for (let seat = 0; seat < 56; seat++) {
      const x = -35 + seat * 1.25;
      if (Math.abs(x) < 2 || Math.abs(x - 20) < 1.5 || Math.abs(x + 20) < 1.5) continue;
      box(x, y + .36, z, .86, .12, .65, row % 2 ? paint : white);
      box(x, y + .67, z - .28, .86, .55, .1, row % 2 ? paint : white);
    }
  }
  for (const x of [-37, -18, 0, 18, 37]) {
    box(x, 3.1, -108, .22, 6.2, .22, steel); box(x, 5.9, -102, .16, .22, 14, steel);
  }
  box(0, 6.1, -102, 78, .18, 14, paint);
  box(45, 2.8, 107, 68, 5.6, 14, concrete); box(45, 5.75, 107, 70, .3, 15, dark);
  for (let bay = 0; bay < 7; bay++) {
    const x = 17 + bay * 9.3;
    box(x, 1.62, 99.97, 7.8, 3.1, .06, paint);
    for (let slat = 0; slat < 12; slat++) box(x, .3 + slat * .235, 99.92, 7.8, .017, .014, steel);
    box(x, 4.4, 99.92, 7.8, .85, .04, glass);
    for (const side of [-1, 1]) box(x + side * 3.85, 4.4, 99.88, .05, .94, .08, steel);
  }
  for (const side of [-1, 1]) {
    for (let x = -150; x <= 150; x += 5) {
      if (side === 1 && Math.abs(x) < 8) continue;
      box(x, 1.3, side * 96, .06, 2.6, .06, steel);
    }
    for (const y of [.4, .7, 1, 1.5, 2, 2.55]) {
      if (side === 1) for (const x of [-79, 79]) box(x, y, 96, 142, .028, .028, steel);
      else box(0, y, -96, 300, .028, .028, steel);
    }
    for (let z = -90; z <= 90; z += 5) box(side * 156, 1.3, z, .06, 2.6, .06, steel);
    for (const y of [.4, .7, 1, 1.5, 2, 2.55]) box(side * 156, y, 0, .028, .028, 180, steel);
  }
  // Varied crowns behind the rails, without new obstacles on the driving route.
  for (let i = 0; i < 26; i++) {
    const side = i % 2 ? 1 : -1, x = side * (164 + (i % 3) * 4), z = -88 + Math.floor(i / 2) * 14;
    const height = 5.2 + Math.sin(i * 3.1) * 1.2;
    box(x, height * .42, z, .3, height * .84, .32, bark);
    for (let lobe = 0; lobe < 5; lobe++) {
      const angle = lobe * Math.PI * .4 + i;
      append(sphere, leaves[(i + lobe) % 3], x + Math.cos(angle) * 1.35, height + (lobe % 2) * .7, z + Math.sin(angle) * 1.35, 1.7, 1.7 + (lobe % 2) * .6, 1.65, angle);
    }
  }
  for (const x of [-145, 145]) for (const z of [-98, 98]) {
    box(x, 4.5, z, .18, 9, .18, steel); box(x, 8.8, z, 3, .14, .32, steel);
    for (const offset of [-1, 0, 1]) { box(x + offset, 8.72, z + .12, .65, .3, .26, dark); box(x + offset, 8.67, z + .27, .57, .16, .02, white); }
  }
  // Batch by finish. Hundreds of seats and kerbs do not create hundreds of submissions.
  for (const [mat, batch] of batches) {
    // Box geometry is indexed; strip and sphere geometry use matching attributes.
    const geometry = mergeGeometries(batch, false); batch.forEach(value => value.dispose());
    if (!geometry) continue; geometries.add(geometry);
    const mesh = new THREE.Mesh(geometry, mat); mesh.castShadow = mat !== asphalt && mat !== grass; mesh.receiveShadow = true; group.add(mesh);
  }
  function showCheckpoint(index: number | null) {
    checkpoint.visible = index !== null && index !== 0;
    if (index === null || index === 0) return;
    const a = racingPoints[index], b = racingPoints[index + 1];
    checkpoint.position.set(a[0], 0, a[2]); checkpoint.rotation.y = Math.atan2(b[0] - a[0], b[2] - a[2]);
  }
  showCheckpoint(null);
  return { group, showCheckpoint, dispose() { group.removeFromParent(); geometries.forEach(value => value.dispose()); materials.forEach(value => value.dispose()); textures.forEach(value => value.dispose()); } };
}

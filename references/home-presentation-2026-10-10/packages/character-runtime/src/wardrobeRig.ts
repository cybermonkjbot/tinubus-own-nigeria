import * as THREE from "three";
import { withAppearanceRigLifetime } from "./appearanceRigLifetime";
import type { CompanionAppearance } from "../../utils/src/companion";
import { companionBodyColor } from "../../utils/src/companionAppearance";
import { isDress, type FabricTexture, type GarmentStyle } from "../../utils/src/companionWardrobe";
import { acquireGeometryVariant, type GeometryLease } from "./geometryLease";
import { createAppearanceSnapshot } from "./appearanceSnapshot";

const wardrobeAppearanceFields = [
  "garmentStyle", "look", "topColor", "bodyColor", "baseId", "fabricTexture", "clothingPattern", "patternColor",
] as const;

// Recipe keys share shapes across rigs; the last lease removes each cached shape.
const wardrobeGeometryFamily = {};

const fabrics: Record<FabricTexture, { roughness: number; bump: number; repeat: number }> = {
  cotton: { roughness: .88, bump: .004, repeat: 10 }, linen: { roughness: .96, bump: .012, repeat: 6 },
  knit: { roughness: .9, bump: .018, repeat: 4 }, denim: { roughness: .84, bump: .01, repeat: 9 },
  satin: { roughness: .28, bump: .002, repeat: 14 }, velvet: { roughness: .98, bump: .007, repeat: 16 },
};

type FabricTextureLease = { texture: THREE.DataTexture; release(): void };
type FabricTextureEntry = { template: THREE.DataTexture; users: number };
const fabricTextures = new Map<FabricTexture, FabricTextureEntry>();

function createFabricTexture(id: FabricTexture) {
  const size = 64, data = new Uint8Array(size * size * 4);
  for (let y = 0; y < size; y++) for (let x = 0; x < size; x++) {
    const warp = Math.sin(x * Math.PI / 4), weft = Math.sin(y * Math.PI / 4);
    const weave = id === "knit" ? Math.cos(x * Math.PI / 8 + Math.sin(y * Math.PI / 8) * 2)
      : id === "denim" ? Math.sin((x + y) * Math.PI / 4) : warp * weft;
    const grain = Math.sin(x * 23 + y * 41) * Math.sin(x * 17 - y * 13);
    const value = Math.round(128 + weave * 50 + grain * (id === "linen" ? 36 : 15));
    const offset = (y * size + x) * 4; data[offset] = data[offset + 1] = data[offset + 2] = value; data[offset + 3] = 255;
  }
  const texture = new THREE.DataTexture(data, size, size);
  texture.wrapS = texture.wrapT = THREE.RepeatWrapping; texture.repeat.setScalar(fabrics[id].repeat);
  texture.magFilter = THREE.LinearFilter; texture.minFilter = THREE.LinearMipmapLinearFilter; texture.generateMipmaps = true; texture.needsUpdate = true;
  return texture;
}

/** Share immutable texels, not texture listeners that could retain another renderer. */
function acquireFabricTexture(id: FabricTexture): FabricTextureLease {
  const entry = fabricTextures.get(id) ?? { template: createFabricTexture(id), users: 0 };
  // Three r180 clones share their Source and, within a renderer, matching GPU storage.
  // Each rig still disposes its own wrapper and its renderer's listeners.
  const texture = entry.template.clone();
  fabricTextures.set(id, entry); entry.users += 1;
  let released = false;
  return {
    texture,
    release() {
      if (released) return; released = true;
      texture.dispose();
      if (--entry.users === 0) { fabricTextures.delete(id); entry.template.dispose(); }
    },
  };
}

/** Garments use this participant's bones; pooled assets and animation tracks stay unchanged. */
export function createWardrobeRig(model: THREE.Object3D) {
  const chest = model.getObjectByName("Chest"), hips = model.getObjectByName("Hips");
  model.updateWorldMatrix(true, true);
  const restWorld = new Map<THREE.Object3D, THREE.Matrix4>();
  const bindingNames = ["Hips", "DeformHips", "DeformLegL", "DeformLegR", ...["L", "R"].flatMap(side =>
    [`Shoulder${side}`, `Elbow${side}`, `Hand${side}`, `DeformShoulder${side}`, `DeformElbow${side}`])];
  for (const name of bindingNames) {
    const node = model.getObjectByName(name);
    if (node) restWorld.set(node, node.matrixWorld.clone());
  }
  const outer: { node: THREE.Object3D; id: string; visible: boolean }[] = [];
  const tanks: { node: THREE.Object3D; visible: boolean }[] = [], trousers: { node: THREE.Object3D; visible: boolean }[] = [];
  const materials = new Map<THREE.MeshStandardMaterial, { roughness: number; bumpMap: THREE.Texture | null; bumpScale: number; compile: THREE.MeshStandardMaterial["onBeforeCompile"]; cacheKey: THREE.Material["customProgramCacheKey"] }>();
  const attached: THREE.Mesh[] = [], skeletons: THREE.Skeleton[] = [], ownedMaterials: THREE.Material[] = [];
  const geometry = new Map<string, GeometryLease>(), textureCache = new Map<FabricTexture, FabricTextureLease>();
  const uniform = { wardrobePattern: { value: 0 }, wardrobePatternColor: { value: new THREE.Color("#eee9df") } };
  let disposed = false;
  const dispose = () => {
    if (disposed) return; disposed = true;
    [...outer, ...tanks, ...trousers].forEach(({ node, visible }) => { node.visible = visible; });
    materials.forEach((original, material) => { material.roughness = original.roughness; material.bumpMap = original.bumpMap; material.bumpScale = original.bumpScale; material.onBeforeCompile = original.compile; material.customProgramCacheKey = original.cacheKey; material.needsUpdate = true; });
    attached.forEach(node => node.removeFromParent()); skeletons.forEach(skeleton => skeleton.dispose());
    geometry.forEach(lease => lease.release()); geometry.clear();
    textureCache.forEach(lease => lease.release()); textureCache.clear(); ownedMaterials.forEach(material => material.dispose());
  };
  const shapeFor = (key: string, create: () => THREE.BufferGeometry) => {
    let lease = geometry.get(key);
    if (!lease) { lease = acquireGeometryVariant(wardrobeGeometryFamily, key, create); geometry.set(key, lease); }
    return lease.geometry;
  };
  function fabricMaterial(material: THREE.MeshStandardMaterial) {
    if (materials.has(material)) return;
    const original = { roughness: material.roughness, bumpMap: material.bumpMap, bumpScale: material.bumpScale, compile: material.onBeforeCompile, cacheKey: material.customProgramCacheKey };
    materials.set(material, original);
    material.onBeforeCompile = (shader, renderer) => {
      original.compile.call(material, shader, renderer);
      Object.assign(shader.uniforms, uniform);
      shader.vertexShader = `varying vec3 vWardrobePosition;\n${shader.vertexShader}`
        .replace("#include <begin_vertex>", "#include <begin_vertex>\nvWardrobePosition = position;");
      shader.fragmentShader = `varying vec3 vWardrobePosition; uniform float wardrobePattern; uniform vec3 wardrobePatternColor;\n${shader.fragmentShader}`
        .replace("#include <color_fragment>", `#include <color_fragment>
          vec2 cloth = vWardrobePosition.xy * 18.0;
          float lineX = smoothstep(-0.12, 0.12, sin(cloth.x));
          float lineY = smoothstep(-0.12, 0.12, sin(cloth.y));
          float pattern = 0.0;
          if (wardrobePattern > 0.5 && wardrobePattern < 1.5) pattern = lineY;
          else if (wardrobePattern > 1.5 && wardrobePattern < 2.5) pattern = (lineX + lineY) * .4;
          else if (wardrobePattern > 2.5) pattern = 1.0 - smoothstep(.18, .25, length(fract(cloth * .32) - .5));
          diffuseColor.rgb = mix(diffuseColor.rgb, wardrobePatternColor, pattern);
        `);
    };
    material.customProgramCacheKey = () => "bestie-wardrobe-v1";
    material.needsUpdate = true;
  }
  try {
    model.traverse(node => {
      const garment = node.userData.garment_id;
      if (node.userData.garment_slot === "outerwear" || /^(Tailored_bomber|Weighted_bomber|Ribbed_cuff|Jacket_ribbing|Fitted_pocket|Pocket_|Warm_silver002)/.test(node.name)) {
        outer.push({ node, id: typeof garment === "string" ? garment : "rose-bomber", visible: node.visible });
      }
      if (garment === "cream-tank" || node.name === "Cream_top") tanks.push({ node, visible: node.visible });
      if (garment === "wide-trousers" || /^(Weighted_continuous_trousers|Twill_seams|Belt)/.test(node.name)) trousers.push({ node, visible: node.visible });
      if (node instanceof THREE.Mesh) for (const material of Array.isArray(node.material) ? node.material : [node.material]) {
        if (material instanceof THREE.MeshStandardMaterial && ["Jacket fabric", "Jacket ribbing", "Charcoal twill", "Twill seams"].includes(material.name)) fabricMaterial(material);
      }
    });
    const cloth = new THREE.MeshStandardMaterial({ name: "Wardrobe cloth", color: "#b97e74", roughness: .88, side: THREE.DoubleSide });
    ownedMaterials.push(cloth);
    const skin = new THREE.MeshStandardMaterial({ name: "Wardrobe limb fur", color: "#efc18e", roughness: 1 });
    ownedMaterials.push(skin);
    const trim = new THREE.MeshStandardMaterial({ name: "Wardrobe trim", color: "#eee9df", roughness: .65 });
    ownedMaterials.push(trim);
    fabricMaterial(cloth);
    function textureFor(id: FabricTexture) {
      let lease = textureCache.get(id);
      if (!lease) { lease = acquireFabricTexture(id); textureCache.set(id, lease); }
      return lease.texture;
    }
    function add(parent: THREE.Object3D | undefined, shape: THREE.BufferGeometry, material: THREE.Material) {
      const mesh = new THREE.Mesh(shape, material); mesh.name = "BestieWardrobe"; mesh.castShadow = true; mesh.receiveShadow = true;
      attached.push(mesh); parent?.add(mesh); return mesh;
    }
    function shell(points: [number, number][], depth: number, pleats = 0) {
      const shape = new THREE.LatheGeometry(points.map(([radius, y]) => new THREE.Vector2(radius, y)), 64);
      const positions = shape.getAttribute("position");
      for (let i = 0; i < positions.count; i++) {
        const x = positions.getX(i), z = positions.getZ(i), y = positions.getY(i);
        const fold = 1 + pleats * Math.cos(Math.atan2(x, z) * 20) * Math.max(0, -y);
        positions.setXYZ(i, x * fold, y, z * depth * fold);
      }
      shape.computeVertexNormals(); return shape;
    }
    function skinMesh(parent: THREE.Object3D, shape: THREE.BufferGeometry, material: THREE.Material, bones: THREE.Bone[]) {
      const skeleton = new THREE.Skeleton(bones, bones.map(bone => restWorld.get(bone)!.clone().invert()));
      const mesh = new THREE.SkinnedMesh(shape, material);
      try {
        parent.add(mesh); mesh.bind(skeleton, restWorld.get(parent)!); mesh.name = "BestieWardrobe";
        mesh.castShadow = mesh.receiveShadow = true; mesh.frustumCulled = false;
      } catch (error) { mesh.removeFromParent(); skeleton.dispose(); throw error; }
      skeletons.push(skeleton); attached.push(mesh); return mesh;
    }
    function arm(side: string, material: THREE.Material, upperRadius: number, wristRadius: number) {
      const shoulder = model.getObjectByName(`Shoulder${side}`), elbow = model.getObjectByName(`Elbow${side}`), hand = model.getObjectByName(`Hand${side}`);
      const upperBone = model.getObjectByName(`DeformShoulder${side}`), lowerBone = model.getObjectByName(`DeformElbow${side}`);
      if (!shoulder || !elbow || !hand || !(upperBone instanceof THREE.Bone) || !(lowerBone instanceof THREE.Bone)) return null;
      const inverse = restWorld.get(shoulder)!.clone().invert();
      const joint = new THREE.Vector3().setFromMatrixPosition(restWorld.get(elbow)!).applyMatrix4(inverse);
      const end = new THREE.Vector3().setFromMatrixPosition(restWorld.get(hand)!).applyMatrix4(inverse);
      const key = ["arm", side, upperRadius, wristRadius, ...joint.toArray(), ...end.toArray()].join(":");
      const shape = shapeFor(key, () => {
        const curve = new THREE.CatmullRomCurve3([new THREE.Vector3(0, .055, 0), joint.clone().multiplyScalar(.45), joint, joint.clone().lerp(end, .5), end]);
        const segments = 24, sides = 16;
        const shape = new THREE.TubeGeometry(curve, segments, upperRadius, sides, false);
        const positions = shape.getAttribute("position"), indices: number[] = [], weights: number[] = [];
        for (let ring = 0; ring <= segments; ring++) {
          const t = ring / segments, center = curve.getPointAt(t);
          const radius = THREE.MathUtils.lerp(upperRadius, wristRadius, t) * (ring === 0 ? .65 : 1);
          const lowerWeight = THREE.MathUtils.smoothstep(t, .3, .56);
          for (let segment = 0; segment <= sides; segment++) {
            const index = ring * (sides + 1) + segment;
            const point = new THREE.Vector3().fromBufferAttribute(positions, index).sub(center).multiplyScalar(radius / upperRadius).add(center);
            positions.setXYZ(index, point.x, point.y, point.z);
            indices.push(0, 1, 0, 0); weights.push(1 - lowerWeight, lowerWeight, 0, 0);
          }
        }
        shape.computeVertexNormals();
        shape.setAttribute("skinIndex", new THREE.Uint16BufferAttribute(indices, 4)); shape.setAttribute("skinWeight", new THREE.Float32BufferAttribute(weights, 4));
        return shape;
      });
      return skinMesh(shoulder, shape, material, [upperBone, lowerBone]);
    }
    const longSleeves: THREE.Mesh[] = [], shortSleeves: THREE.Mesh[] = [], bareArms: THREE.Mesh[] = [], legs: THREE.Mesh[] = [];
    const torso = add(chest, shapeFor("torso", () => shell([[.34, -.02], [.36, .04], [.34, .2], [.38, .55], [.4, .68], [.3, .79], [.16, .84]], .74)), cloth);
    const roundNeck = torso.geometry, wrapNeck = shapeFor("wrap-neck", () => {
      const wrapNeck = roundNeck.clone();
      const neckline = wrapNeck.getAttribute("position");
      for (let i = 0; i < neckline.count; i++) {
        const x = neckline.getX(i), y = neckline.getY(i), z = neckline.getZ(i);
        if (y > .78 && z > 0) neckline.setY(i, y - .2 * Math.max(0, 1 - Math.abs(x) / .28) * Math.min(1, z / .1));
      }
      wrapNeck.computeVertexNormals();
      return wrapNeck;
    });
    for (const side of ["L", "R"]) {
      const shoulder = model.getObjectByName(`Shoulder${side}`), elbow = model.getObjectByName(`Elbow${side}`);
      const limb = (parent: THREE.Object3D | undefined, length: number, radius: number, material: THREE.Material) => {
        const mesh = add(parent, shapeFor(`limb:${length}:${radius}`, () => new THREE.CapsuleGeometry(radius, Math.max(.01, length - radius * 2), 6, 16)), material);
        mesh.position.y = -length / 2; return mesh;
      };
      const bareArm = arm(side, skin, .14, .085), sleeve = arm(side, cloth, .165, .115);
      if (bareArm) bareArms.push(bareArm);
      else bareArms.push(limb(shoulder, .54, .14, skin), limb(elbow, .64, .115, skin));
      if (sleeve) longSleeves.push(sleeve);
      else longSleeves.push(limb(shoulder, .56, .17, cloth), limb(elbow, .62, .14, cloth));
      shortSleeves.push(limb(shoulder, .28, .16, cloth));
      legs.push(limb(model.getObjectByName(`Leg${side}`), .68, .145, skin), limb(model.getObjectByName(`Knee${side}`), .74, .105, skin));
    }
    const shirtDetails: THREE.Mesh[] = [];
    for (const side of [-1, 1]) {
      const collar = add(chest, shapeFor("collar", () => new THREE.BoxGeometry(.17, .16, .025)), cloth);
      collar.position.set(side * .105, .755, .23); collar.rotation.z = side * .55; shirtDetails.push(collar);
    }
    for (let y = .13; y < .72; y += .13) { const button = add(chest, shapeFor("button", () => new THREE.SphereGeometry(.018, 10, 8)), trim); button.position.set(0, y, .285); shirtDetails.push(button); }
    const sash = add(hips, shapeFor("sash", () => shell([[.371, .025], [.358, .115]], .85)), trim);
    const bow = add(hips, shapeFor("bow", () => new THREE.TorusKnotGeometry(.05, .012, 32, 6)), trim); bow.position.set(.32, .06, .17);
    const wrapSeam = add(chest, shapeFor("wrap-seam", () => new THREE.TubeGeometry(new THREE.CatmullRomCurve3([
      new THREE.Vector3(.23, .77, .2), new THREE.Vector3(.06, .59, .291), new THREE.Vector3(-.19, .35, .256), new THREE.Vector3(-.24, .05, .205),
    ]), 24, .008, 6, false)), trim);
    const skirtCache = new Map<GarmentStyle, THREE.Mesh>();
    function skirtFor(style: GarmentStyle) {
      const cached = skirtCache.get(style); if (cached) return cached;
      const long = style === "evening-gown", pleated = style === "pleated-dress";
      model.updateWorldMatrix(true, true);
      const bones = ["DeformHips", "DeformLegL", "DeformLegR"].map(name => model.getObjectByName(name));
      const skinBones = hips && bones.every((bone): bone is THREE.Bone => bone instanceof THREE.Bone) ? bones : undefined;
      const shape = shapeFor(`skirt:${style}:${!!skinBones}`, () => {
        const shape = shell([[long ? .67 : .64, long ? -1.36 : -.83], [.6, long ? -1.25 : -.76], [.46, -.35], [.36, .04], [.34, .1]], .84, pleated ? .11 : .014);
        if (skinBones) {
          const positions = shape.getAttribute("position"), indices: number[] = [], weights: number[] = [];
          for (let i = 0; i < positions.count; i++) {
            const legWeight = Math.min(.38, Math.max(0, -positions.getY(i) - .12) * .38);
            indices.push(0, positions.getX(i) < 0 ? 1 : 2, 0, 0); weights.push(1 - legWeight, legWeight, 0, 0);
          }
          shape.setAttribute("skinIndex", new THREE.Uint16BufferAttribute(indices, 4)); shape.setAttribute("skinWeight", new THREE.Float32BufferAttribute(weights, 4));
        }
        return shape;
      });
      let mesh: THREE.Mesh;
      if (hips && skinBones) {
        mesh = skinMesh(hips, shape, cloth, skinBones);
      } else { mesh = new THREE.Mesh(shape, cloth); hips?.add(mesh); }
      mesh.name = "BestieDressSkirt"; mesh.castShadow = mesh.receiveShadow = true; mesh.frustumCulled = false;
      if (!attached.includes(mesh)) attached.push(mesh); skirtCache.set(style, mesh); return mesh;
    }
    attached.forEach(node => { node.visible = false; });
    const appearance = createAppearanceSnapshot(wardrobeAppearanceFields);
    return withAppearanceRigLifetime<Partial<CompanionAppearance>>({
      apply(input: Partial<CompanionAppearance> = {}, force = false) {
        if (disposed) return;
        if (!appearance.begin(input, force)) return;
        const style = input.garmentStyle ?? "bomber", dress = isDress(style), bomber = style === "bomber";
        const hasIvory = outer.some(part => part.id === "ivory-bomber");
        outer.forEach(({ node, id }) => { node.visible = bomber && (!hasIvory || id === (input.look === "ivory" ? "ivory-bomber" : "rose-bomber")); });
        tanks.forEach(({ node, visible }) => { node.visible = bomber && visible; });
        trousers.forEach(({ node, visible }) => { node.visible = !dress && visible; });
        torso.visible = !bomber; bareArms.forEach(node => { node.visible = !bomber; });
        torso.geometry = style === "wrap-dress" ? wrapNeck : roundNeck;
        wrapSeam.visible = style === "wrap-dress";
        shortSleeves.forEach(node => { node.visible = style === "t-shirt"; });
        longSleeves.forEach(node => { node.visible = style === "shirt"; });
        shirtDetails.forEach(node => { node.visible = style === "shirt"; });
        legs.forEach(node => { node.visible = dress; });
        skirtCache.forEach(node => { node.visible = false; });
        if (dress) skirtFor(style).visible = true;
        sash.visible = dress; bow.visible = style === "wrap-dress";
        torso.scale.set(style === "evening-gown" || style === "wrap-dress" ? .96 : 1, 1, 1);
        cloth.color.set(input.topColor ?? (input.look === "ivory" ? "#e7dfcc" : "#b97e74"));
        skin.color.set(companionBodyColor(input) ?? "#efc18e"); trim.color.set(input.patternColor ?? "#eee9df");
        uniform.wardrobePattern.value = ["plain", "stripes", "gingham", "dots"].indexOf(input.clothingPattern ?? "plain");
        uniform.wardrobePatternColor.value.set(input.patternColor ?? "#eee9df");
        const settings = input.fabricTexture ? fabrics[input.fabricTexture] : null;
        const fabricTexture = input.fabricTexture ? textureFor(input.fabricTexture) : null;
        materials.forEach((original, material) => {
          const map = fabricTexture ?? original.bumpMap;
          if (!!material.bumpMap !== !!map) material.needsUpdate = true;
          material.bumpMap = map; material.bumpScale = settings?.bump ?? original.bumpScale; material.roughness = settings?.roughness ?? original.roughness;
        });
        // All materials have switched before unused wrappers and sources can retire.
        // A failed material update keeps its leases for retry or disposal.
        for (const [id, lease] of textureCache) if (id !== input.fabricTexture) {
          textureCache.delete(id); lease.release();
        }
        appearance.capture(input);
      },
      dispose,
    });
  } catch (error) { dispose(); throw error; }
}

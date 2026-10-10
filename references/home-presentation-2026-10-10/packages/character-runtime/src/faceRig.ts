import * as THREE from "three";
import { companionBodyColor, normalizeAppearance } from "../../utils/src/companionAppearance";
import type { GroupCallAppearance } from "./types";
import { createAppearanceSnapshot } from "./appearanceSnapshot";
import { acquireGeometryVariant, type GeometryLease } from "./geometryLease";
import { withAppearanceRigLifetime } from "./appearanceRigLifetime";

const faceAppearanceFields = [
  "baseId", "bodyColor", "eyeShape", "eyeColor", "browStyle", "noseShape", "noseColor",
  "mouthShape", "cheekStyle", "markingStyle", "markingColor", "eyewear", "accessoryVisible", "necklaceVisible", "hipChainVisible",
] as const;

const glassesGeometryFamily = {};

function createGlasses(head: THREE.Object3D | undefined) {
  const group = new THREE.Group(); group.name = "BestieGlasses"; group.visible = false;
  const leases: GeometryLease[] = [];
  let frame: THREE.MeshStandardMaterial | undefined;
  let disposed = false;
  const dispose = () => {
    if (disposed) return;
    disposed = true;
    group.removeFromParent(); group.clear();
    for (const lease of leases) lease.release();
    leases.length = 0;
    frame?.dispose(); frame = undefined;
  };
  try {
    // Keep the hidden graph for bounds and portrait capture. Only shapes are shared.
    const ring = acquireGeometryVariant(glassesGeometryFamily, "torus:.163:.009:8:40:2pi", () => new THREE.TorusGeometry(.163, .009, 8, 40));
    leases.push(ring);
    const bar = acquireGeometryVariant(glassesGeometryFamily, "cylinder:.008:.008:1:8:1:false:0:2pi", () => new THREE.CylinderGeometry(.008, .008, 1, 8));
    leases.push(bar);
    frame = new THREE.MeshStandardMaterial({ color: "#6c5844", metalness: .75, roughness: .3 });
    for (const side of [-1, 1]) {
      const rim = new THREE.Mesh(ring.geometry, frame); rim.position.set(side * .202, .409, .428); group.add(rim);
      const temple = new THREE.Mesh(bar.geometry, frame); temple.position.set(side * .377, .41, .25); temple.scale.y = .36; temple.rotation.x = Math.PI / 2; group.add(temple);
    }
    const bridge = new THREE.Mesh(bar.geometry, frame); bridge.position.set(0, .418, .43); bridge.scale.y = .08; bridge.rotation.z = Math.PI / 2; group.add(bridge);
    head?.add(group);
    return { group, dispose };
  } catch (error) { dispose(); throw error; }
}

// Markings follow the authored head coordinates, including fur and orbital skin.
// Geometry, texture maps, eyelid morphs and speech animation stay unchanged.
const markingFragment = `
  vec3 p = vBestieFace;
  float front = smoothstep(0.08, 0.26, p.z);
  float marking = 0.0;
  if (bestieMarking > 0.5 && bestieMarking < 1.5) {
    marking = (1.0 - smoothstep(0.85, 1.0, length(vec2(p.x / 0.34, (p.y - 0.21) / 0.18)))) * front;
  } else if (bestieMarking > 1.5 && bestieMarking < 2.5) {
    marking = (1.0 - smoothstep(0.82, 1.08, length(vec2((abs(p.x) - 0.22) / 0.18, (p.y - 0.40) / 0.20)))) * front;
  } else if (bestieMarking > 2.5) {
    float stripeX = min(abs(p.x), abs(abs(p.x) - 0.105));
    float stripeY = p.y - (0.65 - abs(p.x) * 0.2);
    float brow = (1.0 - smoothstep(0.65, 1.0, length(vec2(stripeX / 0.022, stripeY / 0.10))))
      * (1.0 - smoothstep(0.14, 0.18, abs(p.x)));
    float cheek = (1.0 - smoothstep(0.014, 0.027, abs(mod(p.y + abs(p.x) * 0.35, 0.09) - 0.045)))
      * smoothstep(0.27, 0.34, abs(p.x)) * smoothstep(0.22, 0.25, p.y) * (1.0 - smoothstep(0.43, 0.49, p.y));
    marking = max(brow, cheek) * front;
  }
  marking = max(marking, bestieEarMark);
  diffuseColor.rgb = mix(diffuseColor.rgb, bestieMarkColor, marking);
  float cheeks = (1.0 - smoothstep(0.65, 1.0, length(vec2((abs(p.x) - 0.28) / 0.10, (p.y - 0.25) / 0.065)))) * front * (1.0 - bestieEarMark);
  if (bestieCheeks > 0.5 && bestieCheeks < 1.5) diffuseColor.rgb = mix(diffuseColor.rgb, vec3(0.66, 0.22, 0.19), cheeks * 0.45);
  if (bestieCheeks > 1.5) {
    vec2 dots = mod(vec2(abs(p.x), p.y) + vec2(0.005, 0.008), 0.036) - 0.018;
    float freckle = (1.0 - smoothstep(0.005, 0.009, length(dots))) * cheeks;
    diffuseColor.rgb = mix(diffuseColor.rgb, vec3(0.18, 0.075, 0.035), freckle * 0.8);
  }
`;

/** One reversible face per cloned character. Does not own blink or speech nodes. */
export function createFaceRig(model: THREE.Object3D) {
  const head = model.getObjectByName("Head");
  const poses = new Map<THREE.Object3D, { position: THREE.Vector3; scale: THREE.Vector3; quaternion: THREE.Quaternion }>();
  const remember = (name: string) => {
    const node = model.getObjectByName(THREE.PropertyBinding.sanitizeNodeName(name));
    if (node) poses.set(node, { position: node.position.clone(), scale: node.scale.clone(), quaternion: node.quaternion.clone() });
    return node;
  };
  const eyes = [remember("EyeL"), remember("EyeR")];
  const brows = [remember("Groomed eyebrow L"), remember("Groomed eyebrow R")];
  const nose = remember("Nose"), smile = remember("Smile");
  const mouthParts = [remember("Mouth opening"), remember("Lower mouth warmth")];
  const necklace = model.getObjectByName("AccessoryNecklace"), hipChain = model.getObjectByName("AccessoryChain");
  const owned: { node: THREE.Mesh; original: THREE.Material | THREE.Material[]; materials: THREE.MeshStandardMaterial[] }[] = [];
  const uniform = {
    bestieMarking: { value: 0 }, bestieCheeks: { value: 0 }, bestieMarkColor: { value: new THREE.Color("#393634") },
    bestieEyeTint: { value: new THREE.Color() }, bestieTintEye: { value: 0 },
  };
  const skins: { material: THREE.MeshStandardMaterial; originalColor: THREE.Color; node: THREE.Mesh; matrix: THREE.Matrix4; ear: { value: number }; isEar: boolean }[] = [];
  const noses: { material: THREE.MeshStandardMaterial; color: THREE.Color }[] = [];
  head?.updateWorldMatrix(true, true);
  head?.traverse(node => {
    if (!(node instanceof THREE.Mesh)) return;
    const source = Array.isArray(node.material) ? node.material : [node.material];
    if (!source.some(material => ["Peach cream fur", "Cream guard hairs", "Amber iris", "Peach nose", "Ear velvet"].includes(material.name))) return;
    let parent: THREE.Object3D | null = node, isEar = false;
    while (parent && parent !== head) { if (parent.name.startsWith("Ear_")) isEar = true; parent = parent.parent; }
    const materials: THREE.MeshStandardMaterial[] = [];
    const copies = source.map(original => {
      if (!(original instanceof THREE.MeshStandardMaterial)) return original;
      const material = original.clone(); materials.push(material);
      if (material.name === "Peach nose") { noses.push({ material, color: material.color.clone() }); return material; }
      if (material.name === "Amber iris") {
        material.onBeforeCompile = shader => {
          Object.assign(shader.uniforms, { bestieEyeTint: uniform.bestieEyeTint, bestieTintEye: uniform.bestieTintEye });
          shader.fragmentShader = `uniform vec3 bestieEyeTint; uniform float bestieTintEye;\n${shader.fragmentShader}`
            .replace("#include <color_fragment>", `#include <color_fragment>
              float irisLight = dot(diffuseColor.rgb, vec3(0.2126, 0.7152, 0.0722));
              diffuseColor.rgb = mix(diffuseColor.rgb, bestieEyeTint * clamp(irisLight * 2.8, 0.12, 1.5), bestieTintEye);`);
        };
        material.customProgramCacheKey = () => "bestie-iris-v1";
        return material;
      }
      if (!["Peach cream fur", "Cream guard hairs", "Ear velvet"].includes(material.name)) return material;
      const matrix = new THREE.Matrix4().copy(head.matrixWorld).invert().multiply(node.matrixWorld), ear = { value: 0 };
      skins.push({ material, originalColor: material.color.clone(), node, matrix, ear, isEar });
      material.onBeforeCompile = shader => {
        Object.assign(shader.uniforms, uniform, { bestieFaceMatrix: { value: matrix }, bestieEarMark: ear });
        shader.vertexShader = `uniform mat4 bestieFaceMatrix; varying vec3 vBestieFace;\n${shader.vertexShader}`
          .replace("#include <morphtarget_vertex>", "#include <morphtarget_vertex>\nvBestieFace = (bestieFaceMatrix * vec4(transformed, 1.0)).xyz;");
        shader.fragmentShader = `varying vec3 vBestieFace; uniform float bestieMarking; uniform float bestieCheeks; uniform float bestieEarMark; uniform vec3 bestieMarkColor;\n${shader.fragmentShader}`
          .replace("#include <color_fragment>", `#include <color_fragment>\n${markingFragment}`);
      };
      material.customProgramCacheKey = () => "bestie-face-markings-v1";
      return material;
    });
    owned.push({ node, original: node.material, materials });
    node.material = Array.isArray(node.material) ? copies : copies[0];
  });
  const glasses = createGlasses(head);
  const appearance = createAppearanceSnapshot(faceAppearanceFields);
  const headInverse = new THREE.Matrix4();
  return withAppearanceRigLifetime<GroupCallAppearance>({
    apply(input: GroupCallAppearance = {}, force = false) {
      // Authored shop eyewear owns the same attachment when equipped.
      glasses.group.visible = input.eyewear === "round" && !model.getObjectByName("ShopItem:round-glasses");
      if (!appearance.begin(input, force)) return;
      const value = normalizeAppearance({ ...input, look: input.look ?? "rose" });
      poses.forEach((pose, node) => { node.position.copy(pose.position); node.scale.copy(pose.scale); node.quaternion.copy(pose.quaternion); });
      eyes.forEach((eye, index) => {
        if (!eye) return;
        const shape = value.eyeShape;
        eye.scale.x *= shape === "almond" ? 1.08 : shape === "round" ? .97 : 1;
        eye.scale.y *= shape === "round" ? 1.16 : shape === "almond" ? .83 : shape === "sleepy" ? .72 : 1;
        if (shape === "almond") eye.rotation.z += index === 0 ? -.08 : .08;
      });
      brows.forEach((brow, index) => {
        if (!brow) return;
        brow.scale.y *= value.browStyle === "bold" ? 1.7 : value.browStyle === "soft" ? .7 : 1;
        if (value.browStyle === "curious" && index === 1) { brow.position.y += .025; brow.rotation.z += .15; }
      });
      if (nose) nose.scale.multiply(new THREE.Vector3(value.noseShape === "button" ? .78 : value.noseShape === "wide" ? 1.4 : 1, value.noseShape === "button" ? 1.3 : 1, 1));
      const mouthWidth = value.mouthShape === "wide" ? 1.35 : value.mouthShape === "small" ? .72 : 1;
      if (smile) smile.scale.x *= mouthWidth;
      mouthParts.forEach(part => { if (part) part.scale.x *= mouthWidth; });
      uniform.bestieMarking.value = ["none", "muzzle", "mask", "stripes"].indexOf(value.markingStyle ?? "none");
      uniform.bestieCheeks.value = ["none", "blush", "freckles"].indexOf(value.cheekStyle ?? "none");
      uniform.bestieMarkColor.value.set(value.markingColor ?? "#795342");
      uniform.bestieTintEye.value = value.eyeColor ? 1 : 0;
      if (value.eyeColor) uniform.bestieEyeTint.value.set(value.eyeColor);
      noses.forEach(({ material, color }) => { material.color.copy(color); if (value.noseColor) material.color.set(value.noseColor); });
      head?.updateWorldMatrix(true, true);
      if (head) headInverse.copy(head.matrixWorld).invert();
      const bodyColor = companionBodyColor(input);
      skins.forEach(({ material, originalColor, node, matrix, ear, isEar }) => {
        matrix.copy(headInverse).multiply(node.matrixWorld);
        material.color.copy(originalColor);
        if (material.name !== "Ear velvet" && bodyColor) material.color.set(bodyColor);
        ear.value = isEar && value.baseId === "panda" ? 1 : 0;
      });
      if (necklace) necklace.visible = value.necklaceVisible !== false;
      if (hipChain) hipChain.visible = value.hipChainVisible !== false;
      appearance.capture(input);
    },
    dispose() {
      poses.forEach((pose, node) => { node.position.copy(pose.position); node.scale.copy(pose.scale); node.quaternion.copy(pose.quaternion); });
      owned.forEach(({ node, original, materials }) => { node.material = original; materials.forEach(material => material.dispose()); });
      glasses.dispose();
    },
  });
}

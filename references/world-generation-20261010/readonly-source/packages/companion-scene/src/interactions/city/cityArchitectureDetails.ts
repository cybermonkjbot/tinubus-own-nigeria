import * as THREE from "three";
import { companionCityPlan as plan, companionCityParcelPoint, type CompanionCityParcel } from "../../../../../packages/utils/src/companionCity";
import { companionCityArchitectureParts, type CityArchitecturePart, type CityArchitectureMaterial,
  type CityArchitectureGeometry } from "../../../../../packages/utils/src/companionCityArchitectureDetails";
import civicTerrace from "../../../../../packages/assets/venues/civic-terrace-v1.json";
import architectureDelivery from "../../../../../packages/assets/city/lagos-architecture-delivery-v1.json";
import { signatureBuildingParcelIds } from "./signatureBuildings";
import { companionCityParkingParts } from "../../../../../packages/utils/src/companionCityParking";

type Cell = { key: string; minX: number; minZ: number; parcels: CompanionCityParcel[] };
type DetailMaterial = CityArchitectureMaterial | "litWindow" | "dampPlaster" | "dampStone" | "tidyPlaster" | "tidyStone";
type Sign = Pick<CityArchitecturePart, "text" | "print" | "printVariant">;
type InstanceRecipe = {
  geometry: CityArchitectureGeometry; material: DetailMaterial; sign?: Sign;
  shadow: boolean; receiveShadow: boolean;
  keys: readonly [string, string];
};
type CellBatch = { recipe: InstanceRecipe; shadow: boolean; indices: Uint32Array };
type Generator = { id: string; x: number; y: number; z: number };
type ResidentCell = {
  matrices: Float32Array; colours: Float32Array; acousticWalls: Uint8Array; parcelIds: string[];
  batches: readonly [ReadonlyMap<string, CellBatch>, ReadonlyMap<string, CellBatch>];
  generators: Generator[]; shadows: boolean;
};
type BatchSource = { cell: ResidentCell; indices: Uint32Array };
type Batch = { mesh: THREE.InstancedMesh; sources: BatchSource[]; count: number; recipe: InstanceRecipe };
const ENTER_DISTANCE = 420, RELEASE_DISTANCE = 560;
const SHADOW_ENTER = 150, SHADOW_RELEASE = 205;

function prepareCellBatches(recipes: readonly InstanceRecipe[], shadows: boolean,
  unshadowed?: ReadonlyMap<string, CellBatch>): ReadonlyMap<string, CellBatch> {
  const counts = new Map<string, { recipe: InstanceRecipe; shadow: boolean; count: number }>();
  for (const recipe of recipes) {
    const shadow = shadows && recipe.shadow, key = recipe.keys[shadow ? 1 : 0];
    let batch = counts.get(key);
    if (!batch) { batch = { recipe, shadow, count: 0 }; counts.set(key, batch); }
    batch.count++;
  }
  const batches = new Map<string, CellBatch>();
  for (const [key, batch] of counts) {
    const shared = unshadowed?.get(batch.recipe.keys[0]);
    // A shadow batch is a subset of its unshadowed batch. Equal counts mean
    // identical ordered membership, including when only the key changes.
    const indices = shared?.indices.length === batch.count ? shared.indices : new Uint32Array(batch.count);
    batches.set(key, { recipe: batch.recipe, shadow: batch.shadow, indices });
    batch.count = 0;
  }
  for (let index = 0; index < recipes.length; index++) {
    const recipe = recipes[index], key = recipe.keys[shadows && recipe.shadow ? 1 : 0];
    const batch = batches.get(key)!;
    // Shared buffers were filled by the unshadowed preparation pass.
    if (batch.indices !== unshadowed?.get(recipe.keys[0])?.indices) {
      batch.indices[counts.get(key)!.count++] = index;
    }
  }
  return batches;
}

function materialForPart(parcel: CompanionCityParcel, part: CityArchitecturePart): DetailMaterial {
  const groundWall = part.material === "plaster" && part.y - part.h / 2 < .05
    || /^compound (side|front|rear) wall$/.test(part.role) || part.role === "frontage boundary wall";
  if (groundWall && (part.material === "stone" || part.material === "plaster")) {
    const tidy = parcel.kind === "tower" || parcel.style === "waterfront" || ["terrace-villa", "courtyard-villa"].includes(parcel.style)
      || ["skyline", "lagoon-gardens", "garden-quarter"].includes(parcel.district);
    return part.material === "stone" ? tidy ? "tidyStone" : "dampStone" : tidy ? "tidyPlaster" : "dampPlaster";
  }
  // Only individual panes admit interior light. Doors and whole glazing bands stay passive.
  if (part.material !== "glass" || part.role !== "window glass") return part.material;
  let hash = 2166136261;
  const identity = `${parcel.id}:${part.x}:${part.y}:${part.z}`;
  for (let index = 0; index < identity.length; index++) hash = Math.imul(hash ^ identity.charCodeAt(index), 16777619);
  return (hash >>> 0) % 100 < 61 ? "litWindow" : "glass";
}

/** Replaces nearby cityScene detail and its near shells. It does not attach a render loop. */
export function createCityArchitectureDetails(mains: Readonly<{ value: number }> = { value: 1 }) {
  const group = new THREE.Group();
  group.name = "Nearby city construction";
  // Integration reads this set to hide only the corresponding far shell/roof/pitch batches.
  // It must restore those batches whenever their cell key leaves the set.
  const nearCellKeys = new Set<string>();
  group.userData.cityArchitectureNearCellKeys = nearCellKeys;
  const box = new THREE.BoxGeometry(1, 1, 1);
  const shape = new THREE.Shape();
  shape.moveTo(-.475, -.475); shape.lineTo(.475, -.475); shape.lineTo(.475, .475);
  shape.lineTo(-.475, .475); shape.closePath();
  const bevel = new THREE.ExtrudeGeometry(shape, { depth: .95, steps: 1, bevelEnabled: true,
    bevelSize: .025, bevelThickness: .025, bevelSegments: 1, curveSegments: 1 });
  bevel.translate(0, 0, -.475);
  const pipe = new THREE.CylinderGeometry(.5, .5, 1, 8);
  const gable = new THREE.BufferGeometry();
  const points = [[-.5, -.5, -.5], [.5, -.5, -.5], [-.5, -.5, .5], [.5, -.5, .5], [-.5, .5, 0], [.5, .5, 0]];
  gable.setAttribute("position", new THREE.Float32BufferAttribute(
    [0, 4, 5, 0, 5, 1, 2, 3, 5, 2, 5, 4, 0, 2, 4, 1, 5, 3].flatMap(i => points[i]), 3));
  gable.computeVertexNormals();
  const umbrella = new THREE.ConeGeometry(.5, 1, 12);
  const ball = new THREE.SphereGeometry(.5, 10, 6);
  const tyre = new THREE.TorusGeometry(.38, .12, 6, 12);
  tyre.rotateX(Math.PI / 2); tyre.scale(1, 1 / .24, 1);
  const coilPoints = Array.from({ length: 33 }, (_, i) => {
    const t = i / 32, angle = t * Math.PI * 4;
    return new THREE.Vector3(t - .5, Math.sin(angle) * .5, Math.cos(angle) * .5);
  });
  const securityCoil = new THREE.TubeGeometry(new THREE.CatmullRomCurve3(coilPoints), 32, .018, 4, false);
  securityCoil.computeBoundingBox();
  const coilSize = securityCoil.boundingBox!.getSize(new THREE.Vector3());
  const coilCenter = securityCoil.boundingBox!.getCenter(new THREE.Vector3());
  securityCoil.translate(-coilCenter.x, -coilCenter.y, -coilCenter.z);
  securityCoil.scale(1 / coilSize.x, 1 / coilSize.y, 1 / coilSize.z);
  const vehicleWheel = new THREE.CylinderGeometry(.5, .5, 1, 12);
  vehicleWheel.rotateZ(Math.PI / 2);
  const vehicleCabin = new THREE.BufferGeometry();
  const cabinPoints = [[-.5, -.5, -.5], [.5, -.5, -.5], [-.5, -.5, .5], [.5, -.5, .5],
    [-.42, .5, -.35], [.42, .5, -.35], [-.42, .5, .26], [.42, .5, .26]];
  vehicleCabin.setAttribute("position", new THREE.Float32BufferAttribute(
    [0, 1, 3, 0, 3, 2, 0, 4, 5, 0, 5, 1, 2, 3, 7, 2, 7, 6,
      0, 2, 6, 0, 6, 4, 1, 5, 7, 1, 7, 3, 4, 6, 7, 4, 7, 5].flatMap(i => cabinPoints[i]), 3));
  vehicleCabin.computeVertexNormals();
  const signPlane = new THREE.PlaneGeometry(1, 1);
  const geometries: Record<CityArchitectureGeometry, THREE.BufferGeometry> = {
    box, bevel, pipe, gable, umbrella, vehicleWheel, vehicleCabin, ball, tyre, securityCoil,
  };
  const signs = new Map<string, { material: THREE.MeshStandardMaterial; texture: THREE.CanvasTexture | null; references: number }>();
  const signKey = (part: Sign) => `${part.print ?? "sign"}:${part.printVariant ?? 0}:${part.text ?? ""}`;
  const signMaterial = (part: Sign) => {
    const text = part.text!, key = signKey(part);
    const cached = signs.get(key);
    if (cached) { cached.references++; return cached.material; }
    let texture: THREE.CanvasTexture | null = null;
    if (typeof document !== "undefined") {
      const poster = part.print === "poster", painted = part.print === "paint";
      const canvas = document.createElement("canvas"); canvas.width = poster ? 256 : 512; canvas.height = poster ? 320 : 128;
      const context = canvas.getContext("2d");
      if (context) {
        const stencil = painted || text === "POST NO BILLS" || text.startsWith("THIS LAND");
        if (!stencil) {
          context.fillStyle = part.print === "banner" ? "#8d9e68" : poster ? "#c6b995" : "#d8c49b";
          if (poster) {
            const variant = part.printVariant ?? 0;
            context.beginPath(); context.moveTo(8, 13);
            for (let x = 8; x < 250; x += 19) context.lineTo(x, 6 + (x * 13 + variant * 17) % 15);
            context.lineTo(246, 302);
            for (let x = 246; x > 7; x -= 19) context.lineTo(x, 312 - (x * 7 + variant * 11) % 19);
            context.closePath(); context.fill(); context.clip();
            for (let patch = 0; patch < 28; patch++) {
              context.fillStyle = patch % 2 ? "#ad9e7b" : "#d8ccb0";
              context.fillRect((patch * 73) % 256, (patch * 47) % 320, 7 + patch % 11, 3);
            }
            context.fillStyle = "#62745f"; context.fillRect(21, 31, 214, 54);
            context.strokeStyle = "#786952"; context.lineWidth = 2; context.strokeRect(23, 98, 210, 169);
          } else context.fillRect(0, 0, canvas.width, canvas.height);
        }
        context.fillStyle = painted ? "#464a3a" : "#333a30"; context.textAlign = "center"; context.textBaseline = "middle";
        const lines = text.split("\n");
        context.font = `bold ${poster ? 24 : lines.length > 1 ? 29 : 44}px sans-serif`;
        lines.forEach((line, index) => context.fillText(line, canvas.width / 2,
          (poster ? 171 : 64) + (index - (lines.length - 1) / 2) * (poster ? 34 : 42), canvas.width - 40));
        if (painted) {
          context.globalCompositeOperation = "destination-out";
          for (let chip = 0; chip < 45; chip++) context.fillRect((chip * 71) % canvas.width, (chip * 37) % canvas.height, 2, 3);
        }
        texture = new THREE.CanvasTexture(canvas); texture.colorSpace = THREE.SRGBColorSpace;
      }
    }
    const material = new THREE.MeshStandardMaterial({ map: texture, color: texture ? "#ffffff" : "#d8c49b",
      roughness: .93, alphaTest: .1 });
    signs.set(key, { material, texture, references: 1 }); return material;
  };
  const materials: Record<DetailMaterial, THREE.MeshStandardMaterial> = {
    plaster: new THREE.MeshStandardMaterial({ color: "#ffffff", roughness: .88 }),
    stone: new THREE.MeshStandardMaterial({ color: "#ffffff", roughness: .92 }),
    metal: new THREE.MeshStandardMaterial({ color: "#ffffff", roughness: .52, metalness: .32 }),
    glass: new THREE.MeshStandardMaterial({ color: "#ffffff", roughness: .22, metalness: .2 }),
    litWindow: new THREE.MeshStandardMaterial({ color: "#ffffff", roughness: .22, metalness: .2,
      emissive: "#efc58d", emissiveIntensity: 0 }),
    wood: new THREE.MeshStandardMaterial({ color: "#ffffff", roughness: .79 }),
    canvas: new THREE.MeshStandardMaterial({ color: "#ffffff", roughness: .96 }),
    roof: new THREE.MeshStandardMaterial({ color: "#ffffff", roughness: .65, metalness: .15 }),
    recess: new THREE.MeshStandardMaterial({ color: "#ffffff", roughness: .98 }),
    vehiclePaint: new THREE.MeshStandardMaterial({ color: "#ffffff", roughness: .23, metalness: .38 }),
    wornVehiclePaint: new THREE.MeshStandardMaterial({ color: "#ffffff", roughness: .78, metalness: .16 }),
    vehicleGlass: new THREE.MeshStandardMaterial({ color: "#ffffff", roughness: .15, metalness: .32 }),
    rubber: new THREE.MeshStandardMaterial({ color: "#ffffff", roughness: .95 }),
    groundSeam: new THREE.MeshStandardMaterial({ color: "#ffffff", roughness: .98, alphaTest: .22 }),
    dampPlaster: new THREE.MeshStandardMaterial({ color: "#ffffff", roughness: .88 }),
    dampStone: new THREE.MeshStandardMaterial({ color: "#ffffff", roughness: .92 }),
    tidyPlaster: new THREE.MeshStandardMaterial({ color: "#ffffff", roughness: .88 }),
    tidyStone: new THREE.MeshStandardMaterial({ color: "#ffffff", roughness: .92 }),
  };
  materials.groundSeam.onBeforeCompile = shader => {
    shader.vertexShader = `varying vec2 seamUv; varying vec3 seamPoint;\n${shader.vertexShader}`
      .replace("#include <begin_vertex>", `#include <begin_vertex>
        seamUv = uv;
        vec4 seamVertex = vec4(transformed, 1.);
        #ifdef USE_INSTANCING
          seamVertex = instanceMatrix * seamVertex;
        #endif
        seamPoint = (modelMatrix * seamVertex).xyz;`);
    shader.fragmentShader = `varying vec2 seamUv; varying vec3 seamPoint;\n${shader.fragmentShader}`
      .replace("#include <color_fragment>", `#include <color_fragment>
        float seamNoise = fract(sin(dot(floor(seamPoint.xz * 28.), vec2(127.1, 311.7))) * 43758.5453);
        float seamEdge = min(min(seamUv.x, 1.-seamUv.x), min(seamUv.y, 1.-seamUv.y));
        diffuseColor.a *= smoothstep(.02, .3, seamEdge + (seamNoise-.5)*.17);
        diffuseColor.rgb *= .9 + seamNoise*.16;`);
  };
  materials.groundSeam.customProgramCacheKey = () => "city-ground-seam-v1";
  // Metric grain rather than per-box UVs. There are no photographed textures or flat window decals.
  for (const name of ["plaster", "stone", "wood", "canvas", "roof", "dampPlaster", "dampStone", "tidyPlaster", "tidyStone"] as const) {
    const material = materials[name];
    const damp = name.startsWith("damp") ? .85 : name.startsWith("tidy") ? .24 : 0;
    material.onBeforeCompile = shader => {
      shader.vertexShader = `varying vec3 architecturePoint;\n${shader.vertexShader}`
        .replace("#include <begin_vertex>", `#include <begin_vertex>
          vec4 architectureVertex = vec4(transformed, 1.);
          #ifdef USE_INSTANCING
            architectureVertex = instanceMatrix * architectureVertex;
          #endif
          architecturePoint = (modelMatrix * architectureVertex).xyz;`);
      shader.fragmentShader = `varying vec3 architecturePoint;\n${shader.fragmentShader}`
        .replace("#include <color_fragment>", `#include <color_fragment>
          float grain = fract(sin(dot(floor(architecturePoint * 95.), vec3(17.13, 91.77, 43.11))) * 43758.5453);
          float wash = sin(architecturePoint.x * 1.3 + sin(architecturePoint.z * 1.9)) * sin(architecturePoint.y * .73);
          diffuseColor.rgb *= .976 + grain * .036 + wash * .014;
          float moisture = (1.-smoothstep(.08,.68+(grain-.5)*.08,architecturePoint.y))*${damp.toFixed(2)};
          diffuseColor.rgb *= 1.-moisture*.24;`);
    };
    material.customProgramCacheKey = () => `city-architecture-metric-grain-v2-${damp}`;
  }
  const cells = new Map<string, Cell>();
  for (const parcel of plan.parcels) {
    if (parcel.id === civicTerrace.parcelId || parcel.id === architectureDelivery.exterior.parcelId) continue;
    const x = Math.floor(parcel.x / plan.cellSize), z = Math.floor(parcel.z / plan.cellSize);
    const key = `${x}:${z}`;
    if (!cells.has(key)) cells.set(key, { key, minX: x * plan.cellSize, minZ: z * plan.cellSize, parcels: [] });
    cells.get(key)!.parcels.push(parcel);
  }
  const resident = new Map<string, ResidentCell>();
  const matrix = new THREE.Matrix4(), position = new THREE.Vector3(), scale = new THREE.Vector3();
  const rotation = new THREE.Quaternion(), colour = new THREE.Color(), up = new THREE.Vector3(0, 1, 0);
  const localRotation = new THREE.Quaternion(), euler = new THREE.Euler();
  const meshes = new Map<string, Batch>();
  let disposed = false, lastX = Infinity, lastZ = Infinity, sunShadows = false;
  const distance = (cell: Cell, target: THREE.Vector3) => Math.hypot(
    Math.max(cell.minX - target.x, 0, target.x - cell.minX - plan.cellSize),
    Math.max(cell.minZ - target.z, 0, target.z - cell.minZ - plan.cellSize));
  const cacheCell = (cell: Cell, shadows: boolean): ResidentCell => {
    // Hosted exteriors replace the building, not its reserved compound parking.
    const parcels = cell.parcels.map(parcel => ({ parcel, parts: signatureBuildingParcelIds.has(parcel.id)
      ? companionCityParkingParts(parcel) : companionCityArchitectureParts(parcel) }));
    const count = parcels.reduce((total, value) => total + value.parts.length, 0);
    const matrices = new Float32Array(count * 16), colours = new Float32Array(count * 3);
    const acousticWalls = new Uint8Array(count);
    const recipes = new Array<InstanceRecipe>(count), parcelIds = new Array<string>(count);
    const sharedRecipes = new Map<string, InstanceRecipe>();
    const generators: Generator[] = [];
    let index = 0;
    // Retain packed placement data and shared render recipes, not generated
    // part objects or two typed-array views for every piece of construction.
    for (const { parcel, parts } of parcels) for (const part of parts) {
      const at = companionCityParcelPoint(parcel, part.x, part.z);
      rotation.setFromAxisAngle(up, parcel.yaw);
      if (part.rotation) {
        // Yaw stays on the parcel plane; bonnet pitch follows the vehicle's local axis.
        euler.set(part.rotation[0], part.rotation[1], part.rotation[2], "YXZ");
        rotation.multiply(localRotation.setFromEuler(euler));
      }
      matrix.compose(position.set(at[0], part.y, at[2]), rotation, scale.set(part.w, part.h, part.d));
      matrix.toArray(matrices, index * 16); colour.set(part.text ? "#ffffff" : part.colour).toArray(colours, index * 3);
      const material = materialForPart(parcel, part), key = `${part.geometry}:${material}`, printKey = signKey(part);
      if (part.role === "generator housing") generators.push({ id: parcel.id, x: at[0], y: part.y, z: at[2] });
      // Only actual masonry faces occlude traffic. Gates, rails and coping stay open.
      const acousticWall = /^compound (side|front|rear) wall$/.test(part.role)
        || part.role === "frontage boundary wall" && part.h > 1;
      const receiveShadow = part.material !== "glass" && part.material !== "vehicleGlass";
      const recipeKey = `${key}:${printKey}:${part.shadow}:${receiveShadow}:${acousticWall}`;
      let recipe = sharedRecipes.get(recipeKey);
      if (!recipe) {
        recipe = { geometry: part.geometry, material, shadow: part.shadow, receiveShadow,
          sign: part.text ? { text: part.text, print: part.print, printVariant: part.printVariant } : undefined,
          keys: [`${key}:false:${printKey}`, `${key}:true:${printKey}`] };
        sharedRecipes.set(recipeKey, recipe);
      }
      acousticWalls[index] = acousticWall ? 1 : 0;
      recipes[index] = recipe; parcelIds[index] = parcel.id; index++;
    }
    // Membership is fixed until this cell leaves. Drop the per-instance recipe
    // references after preparing both modes; rebuilds only combine these groups.
    const unshadowed = prepareCellBatches(recipes, false);
    return { matrices, colours, acousticWalls, parcelIds, generators, shadows,
      batches: [unshadowed, prepareCellBatches(recipes, true, unshadowed)] };
  };
  const disposeBatch = (batch: Batch) => {
    batch.mesh.removeFromParent(); batch.mesh.dispose();
    const sign = batch.recipe.sign;
    if (sign) {
      const key = signKey(sign), resource = signs.get(key)!;
      if (--resource.references === 0) {
        resource.material.dispose(); resource.texture?.dispose(); signs.delete(key);
      }
    }
  };
  const clearMeshes = () => {
    for (const batch of meshes.values()) disposeBatch(batch);
    meshes.clear();
  };
  const rebuild = (departed: boolean) => {
    const generators: Generator[] = [];
    const batches = new Map<string, { sources: BatchSource[]; count: number; shadow: boolean; recipe: InstanceRecipe }>();
    // Batch all resident cells together. Repeated railings do not create one draw call each.
    for (const cell of resident.values()) {
      for (const generator of cell.generators) generators.push(generator);
      for (const [key, { recipe, shadow, indices }] of cell.batches[cell.shadows ? 1 : 0]) {
        let batch = batches.get(key);
        if (!batch) { batch = { sources: [], count: 0, shadow, recipe }; batches.set(key, batch); }
        batch.sources.push({ cell, indices }); batch.count += indices.length;
      }
    }
    for (const [key, { sources, count, shadow, recipe }] of batches) {
      let previous = meshes.get(key);
      if (!previous) {
        // Reuse a whole batch when only its shadow admission flips.
        const otherKey = recipe.keys[shadow ? 0 : 1];
        if (!batches.has(otherKey)) {
          previous = meshes.get(otherKey);
          if (previous) meshes.delete(otherKey);
        }
      }
      let mesh = previous?.mesh;
      const previousSources = previous?.sources;
      let contentsChanged = !previousSources || previous?.count !== count || previousSources.length !== sources.length
        || sources.some((source, index) => {
          const prior = previousSources[index];
          return source.cell !== prior.cell || source.indices !== prior.indices;
        });
      const capacity = mesh?.instanceMatrix.count ?? 0;
      if (!mesh || count > capacity || departed && count <= capacity / 4) {
        // Powers of two bound spare capacity. Shrink only after a cell departure.
        let nextCapacity = 1;
        while (nextCapacity < count) nextCapacity *= 2;
        mesh = new THREE.InstancedMesh(recipe.sign ? signPlane : geometries[recipe.geometry],
          recipe.sign ? signMaterial(recipe.sign) : materials[recipe.material], nextCapacity);
        // Only the instance buffer changes when cells enter, leave or change shadows.
        mesh.matrixAutoUpdate = false;
        mesh.instanceColor = new THREE.InstancedBufferAttribute(new Float32Array(nextCapacity * 3), 3);
        // Retain the replacement's sign reference before releasing the old mesh.
        if (previous) disposeBatch(previous);
        group.add(mesh); contentsChanged = true;
      }
      mesh.name = `City construction ${key}`;
      mesh.castShadow = shadow;
      mesh.receiveShadow = recipe.receiveShadow;
      meshes.set(key, { mesh, sources, count, recipe });
      if (!contentsChanged) continue;
      mesh.count = count;
      mesh.boundingBox = null;
      const parcelIds = new Array<string>(count), acousticWalls: number[] = [];
      const matrices = mesh.instanceMatrix.array, colours = mesh.instanceColor!.array;
      let output = 0;
      // Preserve cell/part traversal order for draw instances, picking and
      // acoustic walls. Copy scalars without allocating a view per instance.
      for (const { cell, indices } of sources) for (const index of indices) {
        parcelIds[output] = cell.parcelIds[index];
        if (cell.acousticWalls[index]) acousticWalls.push(output);
        for (let component = 0; component < 16; component++) matrices[output * 16 + component] = cell.matrices[index * 16 + component];
        for (let component = 0; component < 3; component++) colours[output * 3 + component] = cell.colours[index * 3 + component];
        output++;
      }
      mesh.userData.cityParcelIds = parcelIds;
      mesh.userData.acousticWallInstances = acousticWalls;
      mesh.instanceMatrix.needsUpdate = true;
      if (mesh.instanceColor) mesh.instanceColor.needsUpdate = true;
      mesh.computeBoundingSphere();
    }
    group.userData.lagosGenerators = generators;
    for (const [key, batch] of meshes) if (!batches.has(key)) {
      disposeBatch(batch); meshes.delete(key);
    }
  };
  return {
    group,
    update(target: THREE.Vector3, daylight: number): boolean {
      if (disposed || !Number.isFinite(target.x) || !Number.isFinite(target.z)) return false;
      // Update before the movement gate so stationary viewers see cuts and restoration.
      // The caller supplies the existing scene-power fade, including static reduced motion.
      if (Number.isFinite(daylight) && Number.isFinite(mains.value)) {
        materials.litWindow.emissiveIntensity = (1 - THREE.MathUtils.clamp(daylight, 0, 1))
          * THREE.MathUtils.clamp(mains.value, 0, 1) * .45;
      }
      const nextSun = Number.isFinite(daylight) ? daylight > .12 : sunShadows;
      const moved = Math.hypot(target.x - lastX, target.z - lastZ) >= 18;
      if (!moved && nextSun === sunShadows) return false;
      lastX = target.x; lastZ = target.z; sunShadows = nextSun;
      let changed = false, departed = false;
      for (const cell of cells.values()) {
        const range = distance(cell, target), existing = resident.get(cell.key);
        if (existing && range > RELEASE_DISTANCE) {
          resident.delete(cell.key); nearCellKeys.delete(cell.key); changed = true;
          departed = true;
        } else if (!existing && range < ENTER_DISTANCE) {
          resident.set(cell.key, cacheCell(cell, nextSun && range < SHADOW_ENTER));
          nearCellKeys.add(cell.key); changed = true;
        } else if (existing) {
          const shadows = nextSun && range < (existing.shadows ? SHADOW_RELEASE : SHADOW_ENTER);
          if (existing.shadows !== shadows) { existing.shadows = shadows; changed = true; }
        }
      }
      if (changed) rebuild(departed);
      return changed;
    },
    dispose(): void {
      if (disposed) return;
      disposed = true; clearMeshes(); resident.clear(); nearCellKeys.clear();
      group.removeFromParent();
      Object.values(geometries).forEach(geometry => geometry.dispose());
      signPlane.dispose();
      for (const resource of signs.values()) { resource.material.dispose(); resource.texture?.dispose(); }
      signs.clear();
      Object.values(materials).forEach(material => material.dispose());
    },
  };
}

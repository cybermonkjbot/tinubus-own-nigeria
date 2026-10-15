import * as THREE from "three";
import { companionCityPlan as plan, companionCityParcelPoint, companionCityParks } from "../../../../../packages/utils/src/companionCity";
import { companionCityParkWalkingRoads } from "../../../../../packages/utils/src/companionCityParks";
import { companionCityFrontageTreatment } from "../../../../../packages/utils/src/companionCityFrontages";
import {
  companionCityLandscapeSeed, companionCityLandscapeTreatment, companionCityLandscapeCoverage,
  companionCityLandscapeAdditionalFootprints,
  type CityLandscapeAmenity, type CityLandscapeTreatment,
} from "../../../../../packages/utils/src/companionCityLandscapeDetails";

type Shape = "box" | "crown" | "stem" | "disc" | "frond" | "canopy" | "pylon";
type Finish = "stone" | "wood" | "bark" | "metal" | "leaf" | "cloth" | "water";
type PartBatch = { shape: Shape; finish: Finish; values: Float64Array; used: number; count: number };
type Parts = Map<string, PartBatch>;
type Cell = { x: number; z: number; parts: Parts; fine: Parts; base?: THREE.Group; detail?: THREE.Group; shadows: boolean };

// Rows contain rotation kind, tint index, position, scale, then yaw or quaternion.
// Keep recipe numbers at JavaScript precision until writing the GPU instance buffer.
const YAW_ROW = 9, TILT_ROW = 12;

const CELL = 128;
const DETAIL_ENTER = 160;
const DETAIL_LEAVE = 215;
const VISIBLE_ENTER = 610;
const VISIBLE_LEAVE = 760;
const SHADOW_ENTER = 130;
const SHADOW_LEAVE = 180;

/** Replace the central amenity and landmark batches, never the streets or bank geometry. */
export function createCityLandscapeDetails(anchorsEnabled = false, parkPaths = false) {
  const group = new THREE.Group();
  group.name = "Public gardens, shores and landmark construction";
  group.userData.cityLandscapeCoverage = companionCityLandscapeCoverage();
  group.userData.cityLandscapeAdditionalFootprints = companionCityLandscapeAdditionalFootprints();
  const cells = new Map<string, Cell>();
  const geometries: Record<Shape, THREE.BufferGeometry> = {
    box: new THREE.BoxGeometry(1, 1, 1),
    crown: new THREE.IcosahedronGeometry(1, 2),
    stem: new THREE.CylinderGeometry(.5, .62, 1, 9),
    disc: new THREE.CylinderGeometry(.5, .5, 1, 32),
    frond: palmFrond(),
    canopy: new THREE.ConeGeometry(1, .44, 16, 1, true),
    pylon: taperedPylon(),
  };
  const materials: Record<Finish, THREE.MeshStandardMaterial> = {
    stone: new THREE.MeshStandardMaterial({ color: "#ffffff", roughness: .89 }),
    wood: new THREE.MeshStandardMaterial({ color: "#ffffff", roughness: .82 }),
    bark: new THREE.MeshStandardMaterial({ color: "#ffffff", roughness: .95 }),
    metal: new THREE.MeshStandardMaterial({ color: "#ffffff", roughness: .57, metalness: .3 }),
    leaf: new THREE.MeshStandardMaterial({ color: "#ffffff", roughness: .94, side: THREE.DoubleSide }),
    cloth: new THREE.MeshStandardMaterial({ color: "#ffffff", roughness: .96, side: THREE.DoubleSide }),
    water: new THREE.MeshStandardMaterial({ color: "#5b7875", roughness: .27, metalness: .08 }),
  };
  // Grain is evaluated in metres after instance scale. No downloaded maps or textures.
  for (const finish of ["wood", "bark", "stone", "cloth"] as const) {
    const material = materials[finish];
    material.onBeforeCompile = shader => {
      shader.vertexShader = "varying vec3 landscapeLocal;\n" + shader.vertexShader;
      shader.vertexShader = shader.vertexShader.replace("#include <begin_vertex>", `
        #include <begin_vertex>
        vec3 landscapeScale=vec3(1.);
        #ifdef USE_INSTANCING
          landscapeScale=vec3(length(instanceMatrix[0].xyz),length(instanceMatrix[1].xyz),length(instanceMatrix[2].xyz));
        #endif
        landscapeLocal=position*landscapeScale;`);
      shader.fragmentShader = "varying vec3 landscapeLocal;\n" + shader.fragmentShader;
      shader.fragmentShader = shader.fragmentShader.replace("#include <color_fragment>", `
        #include <color_fragment>
        ${finish === "wood" ? `
          float fibre=sin(landscapeLocal.z*68.+sin(landscapeLocal.x*1.7)*1.2);
          diffuseColor.rgb*=.96+fibre*.023;
        ` : finish === "bark" ? `
          float furrow=sin(landscapeLocal.x*43.+landscapeLocal.z*29.+sin(landscapeLocal.y*2.)*.6);
          diffuseColor.rgb*=.94+furrow*.048;
        ` : finish === "stone" ? `
          float aggregate=fract(sin(dot(floor(landscapeLocal.xz*24.+landscapeLocal.y*19.),vec2(12.9,78.2)))*43758.5);
          diffuseColor.rgb*=.97+aggregate*.05;
        ` : `
          float weave=sin(landscapeLocal.x*85.)*sin(landscapeLocal.z*85.);
          diffuseColor.rgb*=.98+weave*.016;
        `}`);
    };
    material.customProgramCacheKey = () => "city-landscape-" + finish + "-v1";
  }
  const matrix = new THREE.Matrix4(), rotation = new THREE.Quaternion();
  const position = new THREE.Vector3(), scale = new THREE.Vector3();
  const up = new THREE.Vector3(0, 1, 0);
  // Converted colours are immutable and shared only within this landscape.
  const tints: THREE.Color[] = [], tintIds = new Map<string, number>();
  const batch = (parts: Parts, name: string) => {
    const root = new THREE.Group(); root.name = name;
    // Keep identity local matrices; parent/world transforms still update normally.
    root.matrixAutoUpdate = false;
    try {
      for (const part of parts.values()) {
        const mesh = new THREE.InstancedMesh(geometries[part.shape], materials[part.finish], part.count);
        mesh.name = part.finish + " " + part.shape; mesh.matrixAutoUpdate = false;
        root.add(mesh);
        const values = part.values;
        for (let i = 0, offset = 0; i < part.count; i++) {
          const tilted = values[offset] === 1;
          if (tilted) rotation.set(values[offset + 8], values[offset + 9], values[offset + 10], values[offset + 11]);
          else rotation.setFromAxisAngle(up, values[offset + 8]);
          matrix.compose(position.set(values[offset + 2], values[offset + 3], values[offset + 4]), rotation,
            scale.set(values[offset + 5], values[offset + 6], values[offset + 7]));
          mesh.setMatrixAt(i, matrix); mesh.setColorAt(i, tints[values[offset + 1]]);
          offset += tilted ? TILT_ROW : YAW_ROW;
        }
        mesh.instanceMatrix.needsUpdate = true;
        if (mesh.instanceColor) mesh.instanceColor.needsUpdate = true;
        mesh.computeBoundingSphere();
        mesh.castShadow = false; mesh.receiveShadow = true;
      }
    } catch (error) {
      root.traverse(node => { if (node instanceof THREE.InstancedMesh) node.dispose(); });
      root.clear(); throw error;
    }
    return root;
  };
  const cellAt = (x: number, z: number) => {
    const ix = Math.floor(x / CELL), iz = Math.floor(z / CELL), key = ix + ":" + iz;
    if (!cells.has(key)) cells.set(key, { x: (ix + .5) * CELL, z: (iz + .5) * CELL, parts: new Map(), fine: new Map(), shadows: false });
    return cells.get(key)!;
  };
  const put = (parts: Parts, x: number, y: number, z: number, w: number, h: number, d: number, tint: string,
    finish: Finish = "stone", shape: Shape = "box", yaw = 0, tilt?: THREE.Quaternion) => {
    const key = shape + ":" + finish;
    let part = parts.get(key);
    if (!part) {
      part = { shape, finish, values: new Float64Array(8 * YAW_ROW), used: 0, count: 0 };
      parts.set(key, part);
    }
    const offset = part.used, end = offset + (tilt ? TILT_ROW : YAW_ROW);
    if (end > part.values.length) {
      const expanded = new Float64Array(Math.max(end, part.values.length * 2));
      expanded.set(part.values); part.values = expanded;
    }
    let tintId = tintIds.get(tint);
    if (tintId === undefined) { tintId = tints.length; tints.push(new THREE.Color(tint)); tintIds.set(tint, tintId); }
    const values = part.values;
    values[offset] = tilt ? 1 : 0; values[offset + 1] = tintId;
    values[offset + 2] = x; values[offset + 3] = y; values[offset + 4] = z;
    values[offset + 5] = w; values[offset + 6] = h; values[offset + 7] = d;
    if (tilt) {
      values[offset + 8] = tilt.x; values[offset + 9] = tilt.y;
      values[offset + 10] = tilt.z; values[offset + 11] = tilt.w;
    } else values[offset + 8] = yaw;
    part.used = end; part.count++;
  };
  const beamFrom = new THREE.Vector3(), beamTo = new THREE.Vector3(), beamDirection = new THREE.Vector3();
  const beamTilt = new THREE.Quaternion();
  const beam = (parts: Parts, from: number[], to: number[], width: number, tint: string,
    finish: Finish = "metal", shape: Shape = "stem") => {
    beamFrom.fromArray(from); beamTo.fromArray(to);
    const direction = beamDirection.copy(beamTo).sub(beamFrom), length = direction.length();
    if (length < .001) return;
    const midpoint = beamFrom.add(beamTo).multiplyScalar(.5);
    beamTilt.setFromUnitVectors(up, direction.divideScalar(length));
    put(parts, midpoint.x, midpoint.y, midpoint.z, width, length, width, tint, finish, shape, 0, beamTilt);
  };
  const treatmentFor = (a: CityLandscapeAmenity): CityLandscapeTreatment => {
    const space = plan.greenSpaces.find(s => a.id.startsWith(s.id + "-"));
    return companionCityLandscapeTreatment(space?.id ?? "beach:" + (a.z < 400 ? "south" : "north"));
  };

  // Preserve the old nearby compound planting removed with the architecture detail block.
  for (const p of plan.parcels) if (p.garden && !companionCityFrontageTreatment(p)?.landscaped) for (const side of [-1, 1]) {
    const at = companionCityParcelPoint(p, side * (p.width / 2 + p.garden * .55), 0);
    const base = cellAt(at[0], at[2]).parts;
    put(base, at[0], 2, at[2], .32, 4, .48, "#776650", "bark", "stem");
    put(base, at[0], 4.4, at[2], 2.1, 2.5, 2.2, p.variant % 2 ? "#708650" : "#516c43", "leaf", "crown");
  }

  for (const a of parkPaths ? companionCityParks().amenities : plan.amenities) {
    const cell = cellAt(a.x, a.z), base = cell.parts, fine = cell.fine;
    const t = treatmentFor(a), seed = companionCityLandscapeSeed(a.id);
    const varied = (seed % 97) / 97;
    const cosine = Math.cos(a.yaw), sine = Math.sin(a.yaw);
    const at = (parts: Parts, x: number, y: number, z: number, w: number, h: number, d: number, tint: string,
      finish: Finish = "stone", shape: Shape = "box", turn = 0) => {
      put(parts, a.x + x * cosine + z * sine, y,
        a.z - x * sine + z * cosine, w, h, d, tint, finish, shape, a.yaw + turn);
    };
    const localBeam = (from: number[], to: number[], width: number, tint: string) => {
      const world = (v: number[]) => [a.x + v[0] * cosine + v[2] * sine, v[1],
        a.z - v[0] * sine + v[2] * cosine];
      beam(fine, world(from), world(to), width, tint, "bark");
    };
    if (a.kind === "tree" || a.kind === "palm") {
      const palm = a.kind === "palm";
      const forest = t.kind === "forest", shore = t.kind === "shore", grove = t.kind === "grove";
      const height = palm ? 8.1 + a.variant * .4 + varied
        : (forest ? 7.2 : grove ? 5.2 : shore ? 6 : 5.5) + a.variant * .55 + varied;
      // Stem and roots stay within the existing 0.8 by 0.8 metre reserve.
      at(base, 0, height * (palm ? .5 : .44), 0, palm ? .32 : .46, height * (palm ? 1 : .88), palm ? .32 : .46, "#7f6b50", "bark", "stem");
      at(fine, 0, .11, 0, .58, .22, .58, "#6f5c43", "bark", "stem");
      at(fine, 0, -.006, 0, .73, .017, .73, "#6e6249", "stone", "disc");
      for (let i = 0; i < 5; i++) {
        const angle = i * Math.PI * 2 / 5 + varied;
        localBeam([0, .29, 0], [Math.cos(angle) * .29, .035, Math.sin(angle) * .29], .1, "#746047");
      }
      if (palm) {
        for (let i = 0; i < 9; i++) at(base, 0, height, 0, 1 + varied * .17, 1.1, 3.6 + varied,
          i % 3 === 0 ? "#839556" : t.leaf, "leaf", "frond", i * Math.PI * 2 / 9);
        for (let y = .55; y < height * .84; y += .48) at(fine, 0, y, 0, .34, .026, .34, "#948061", "bark", "disc");
        at(fine, 0, height - .16, 0, .5, .65, .5, "#756448", "bark", "crown");
      } else {
        // Leaf litter and small understory stay inside the trunk's reserve.
        if (t.kind === "forest" || t.kind === "grove") for (let i = 0; i < 5; i++) {
          const angle = i * Math.PI * 2 / 5 + varied;
          at(fine, Math.cos(angle) * .25, .14, Math.sin(angle) * .25,
            .09, .16, .1, i % 2 ? t.leaf : "#89945d", "leaf", "crown");
        }
        for (let i = 0; i < 4; i++) {
          const angle = i * Math.PI * 2 / 4 + varied, spread = (forest ? .8 : shore || grove ? 1.5 : 1.05) + varied * .5;
          at(base, Math.cos(angle) * spread, height + (i % 2) * .55, Math.sin(angle) * spread,
            (forest ? 2 : shore ? 2.8 : 2.3) + varied * .6, (forest ? 2.5 : shore ? 1.4 : 1.8) + varied * .45,
            (forest ? 1.9 : shore ? 2.7 : 2.1) + varied * .6, i % 2 ? t.leaf : "#5b7545", "leaf", "crown");
          localBeam([0, height * .68, 0], [Math.cos(angle) * spread, height - .1, Math.sin(angle) * spread], .12, "#78634b");
          for (let j = 0; j < 3; j++) at(fine, Math.cos(angle) * (spread + .55 + j * .27), height + .38 + j * .15,
            Math.sin(angle) * (spread + .55 + j * .27), .8, .55, .75, j % 2 ? t.leaf : "#789254", "leaf", "crown");
        }
      }
    } else if (a.kind === "bench") {
      at(base, 0, .43, 0, 2.38, .08, .7, t.timber, "wood");
      for (const x of [-.86, .86]) {
        at(base, x, .22, 0, .085, .44, .58, "#46524a", "metal");
        at(fine, x, .32, -.37, .07, .64, .07, "#46524a", "metal");
        at(fine, x, .7, 0, .07, .06, .58, "#46524a", "metal");
        for (const z of [-.24, .24]) at(fine, x, .036, z, .2, .065, .12, "#596052", "metal");
      }
      for (const y of [.66, .86]) at(base, 0, y, -.41, 2.38, .16, .065, t.timber, "wood");
      for (let z = -.27; z < .32; z += .145) at(fine, 0, .473, z, 2.35, .009, .014, "#61513b", "wood");
      for (const x of [-.86, .86]) for (const y of [.66, .86]) at(fine, x, y, -.451, .022, .022, .012, "#9c9d8d", "metal");
    } else if (a.kind === "flower-bed") {
      // Four walls enclose soil instead of one solid plinth with plants on top.
      at(base, 0, .13, 0, 5.5, .16, 3.45, "#65563f", "stone");
      for (const z of [-1.78, 1.78]) at(base, 0, .16, z, 5.72, .32, .15, "#aaa084");
      for (const x of [-2.78, 2.78]) at(base, x, .16, 0, .15, .32, 3.42, "#aaa084");
      const wild = t.kind === "shore" || t.kind === "common" || t.kind === "grove";
      for (let x = -2.2; x <= 2.2; x += .72) for (let z = -1.22; z < 1.3; z += .76) {
        const shift = Math.sin(x * 7 + z * 3 + varied) * .12;
        at(base, x, .44, z + shift, .4, wild ? .45 : .28, .41, t.leaf, "leaf", "crown");
        at(fine, x + shift, wild ? .75 : .63, z, .17, .09, .16, t.flower, "leaf", "crown");
        at(fine, x, .47, z, .045, .48, .045, "#698254", "leaf", "stem");
      }
      for (let x = -2.1; x < 2.5; x += .72) for (const z of [-1.86, 1.86]) at(fine, x, .16, z, .012, .29, .007, "#776f5b");
    } else if (a.kind === "beach-umbrella") {
      at(base, 0, 1.34, 0, .08, 2.68, .08, "#8a7452", "wood", "stem");
      at(base, 0, 2.7, 0, 2.35, 1, 2.35, ["#e1d5b4", "#b48970", "#7e958a"][a.variant % 3], "cloth", "canopy");
      at(fine, 0, 2.98, 0, .17, .1, .17, "#8d7959", "wood", "disc");
      for (const side of [-1, 1]) {
        at(base, side * 1.45, .36, .34, .72, .1, 1.85, "#cdbf9d", "wood");
        at(base, side * 1.45, .65, -.43, .72, .55, .095, "#cdbf9d", "wood");
        at(fine, side * 1.45, .418, .34, .64, .02, 1.72, "#e1d8bc", "cloth");
        for (const z of [-.4, 1.03]) at(base, side * 1.45, .16, z, .62, .32, .07, "#786b52", "wood");
        for (let z = -.36; z < 1.18; z += .18) at(fine, side * 1.45, .413, z, .69, .006, .012, "#8d7a5a", "wood");
      }
      // Rib ends remain above head height, within the already reserved umbrella area.
      for (let i = 0; i < 8; i++) {
        const angle = i * Math.PI * 2 / 8;
        beam(fine, [a.x, 2.88, a.z], [a.x + Math.cos(angle) * 2.22, 2.5, a.z + Math.sin(angle) * 2.22], .025, "#887758");
      }
    } else if (a.kind === "market-canopy") {
      at(base, 0, 4.1, 0, 15.7, .25, 15.7, "#997158", "metal");
      for (const x of [-6, 6]) for (const z of [-6, 6]) {
        at(base, x, 2.03, z, .2, 4.06, .2, "#596251", "metal");
        at(fine, x, .035, z, .42, .07, .42, "#777b67");
      }
      for (let x = -6; x <= 6; x += 3) at(base, x, .81, 0, 2.45, 1.62, 2.8, "#97815d", "wood");
      for (let x = -7.5; x < 7.5; x += .65) at(fine, x, 4.245, 0, .035, .025, 15.5, "#b18c6c", "metal");
      for (const z of [-6, 6]) at(fine, 0, 3.86, z, 12.2, .19, .16, "#6e715c", "metal");
    } else if (a.kind === "clock-tower") {
      at(base, 0, .17, 0, 3.8, .34, 3.8, "#b5ac96");
      at(base, 0, 4.5, 0, 2.9, 9, 2.9, "#d1c6ac");
      at(base, 0, 9.14, 0, 3.65, .28, 3.65, "#858876");
      for (const side of [-1, 1]) {
        at(base, 0, 7.1, side * 1.46, 1.6, 1.6, .035, "#e0d7bd");
        at(fine, .26, 7.1, side * 1.49, .58, .045, .025, "#3e493e", "metal");
        at(fine, 0, 7.37, side * 1.49, .045, .58, .025, "#3e493e", "metal");
        for (const x of [-.79, .79]) at(fine, x, 7.1, side * 1.49, .038, 1.65, .035, "#817e65", "metal");
      }
      for (let y = .8; y < 6; y += .6) at(fine, 0, y, 1.456, 2.85, .015, .008, "#b5a88c");
    } else if (a.kind === "fountain") {
      at(base, 0, .12, 0, 9.9, .24, 9.9, "#b4af9c", "stone", "disc");
      at(base, 0, .26, 0, 8.7, .025, 8.7, "#ffffff", "water", "disc");
      at(base, 0, .72, 0, .48, .9, .48, "#9fada1", "stone", "stem");
      for (let i = 0; i < 28; i++) {
        const angle = i * Math.PI * 2 / 28;
        at(fine, Math.cos(angle) * 4.55, .295, Math.sin(angle) * 4.55, .66, .11, .37, "#c8c0aa", "stone", "box", -angle + Math.PI / 2);
      }
    }
  }

  const fixed: Parts = new Map();
  for (const p of plan.landmarkPieces) {
    const finish: Finish = p.id.startsWith("stage") || p.id === "heritage-stage" ? "wood" : "stone";
    const wall = p.id.includes("wall");
    put(fixed, p.x, p.y, p.z, wall ? p.w - .02 : p.w, p.h, wall ? p.d - .02 : p.d, p.colour, finish, "box", p.yaw);
    const fine = cellAt(p.x, p.z).fine;
    if (wall) {
      // Face strips sit above the narrowed wall face, inside its original reserve.
      put(fine, p.x, p.y + p.h / 2 - .055, p.z, p.w, .11, p.d, "#c6b99c");
      const alongX = p.w > p.d, length = alongX ? p.w : p.d;
      for (let y = .22; y < p.h - .1; y += .22) {
        const z = alongX ? p.z + p.d / 2 - .004 : p.z;
        const x = alongX ? p.x : p.x + p.w / 2 - .004;
        put(fine, x, y, z, alongX ? length - .06 : .008, .012, alongX ? .008 : length - .06, "#897c65");
      }
      for (let d = -length / 2 + .38; d < length / 2; d += .72) {
        put(fine, p.x + (alongX ? d : p.w / 2 - .004), .9, p.z + (alongX ? p.d / 2 - .004 : d),
          alongX ? .012 : .008, 1.65, alongX ? .008 : .012, "#9b8b71");
      }
    } else if (p.id === "garden-seating" || p.id === "heritage-stage") {
      const length = p.id === "garden-seating" ? p.d : p.w;
      for (let d = -length / 2 + .8; d < length / 2; d += .8) put(fine,
        p.x + (p.id === "heritage-stage" ? d : 0), p.y + p.h / 2 + .003,
        p.z + (p.id === "garden-seating" ? d : 0), p.id === "heritage-stage" ? .015 : p.w - .03,
        .006, p.id === "heritage-stage" ? p.d - .03 : .015, "#776a53", finish);
    } else if (p.id === "stage-canopy") {
      for (const x of [-6.8, 6.8]) put(fine, p.x + x, 4.3, p.z, .16, .28, p.d - .18, "#645b46", "wood");
      for (let x = -8; x <= 8; x += 1) put(fine, p.x + x, 4.47, p.z, .04, .028, p.d - .08, "#6f654c", "wood");
    }
  }
  for (const b of plan.landmarkBeams) {
    const stay = b.id === "link-stay";
    beam(fixed, b.from, b.to, b.width, b.colour, stay ? "metal" : "stone", stay ? "stem" : b.id === "link-crosspiece" ? "box" : "pylon");
    if (stay && anchorsEnabled) {
      const at = b.to, fine = cellAt(at[0], at[2]).fine;
      put(fine, at[0], at[1] / 2, at[2], .32, at[1], .6, "#737f78", "metal");
      put(fine, at[0], at[1] + .0175, at[2], .25, .035, .47, "#a1a79a", "metal");
    }
  }
  const landmarks = batch(fixed, "Heritage Gardens and Lagoon Link silhouettes");
  group.add(landmarks);
  fixed.clear();
  addBoardwalkConstruction();
  // Authoring is complete. Release growth capacity and the construction-only lookup.
  for (const cell of cells.values()) for (const parts of [cell.parts, cell.fine]) for (const part of parts.values()) {
    if (part.used < part.values.length) part.values = part.values.slice(0, part.used);
  }
  tintIds.clear();

  function addBoardwalkConstruction() {
    const walks = (parkPaths ? companionCityParkWalkingRoads : plan.roads).filter(r => r.kind === "boardwalk");
    // Use the street renderer's same averaged normals at bends. Nothing rises above floorY.
    for (const walk of walks) {
      const normals = walk.points.map((p, i) => {
        const a = walk.points[Math.max(0, i - 1)], b = walk.points[Math.min(walk.points.length - 1, i + 1)];
        const length = Math.hypot(b[0] - a[0], b[1] - a[1]) || 1;
        return [-(b[1] - a[1]) / length, (b[0] - a[0]) / length];
      });
      for (let i = 1; i < walk.points.length; i++) {
        const a = walk.points[i - 1], b = walk.points[i], length = Math.hypot(b[0] - a[0], b[1] - a[1]);
        if (length < .01) continue;
        const yaw = -Math.atan2(b[1] - a[1], b[0] - a[0]);
        for (let d = .2; d < length - .12; d += .4) {
          const t = d / length, x = a[0] + (b[0] - a[0]) * t, z = a[1] + (b[1] - a[1]) * t;
          const normal = [normals[i - 1][0] * (1 - t) + normals[i][0] * t, normals[i - 1][1] * (1 - t) + normals[i][1] * t];
          const fine = cellAt(x, z).fine;
          put(fine, x, -.003, z, .013, .006, walk.width * .94, "#776348", "wood", "box", yaw);
          if (Math.round(d * 10) % 12 < 4) for (const side of [-1, 1]) put(fine,
            x + normal[0] * walk.width * .39 * side, -.0015, z + normal[1] * walk.width * .39 * side,
            .018, .003, .018, "#858572", "metal", "disc");
        }
        for (let d = 2; d < length - 1; d += 4) {
          const t = d / length, x = a[0] + (b[0] - a[0]) * t, z = a[1] + (b[1] - a[1]) * t;
          const fine = cellAt(x, z).fine;
          for (const side of [-1, 1]) {
            const nx = normals[i - 1][0] * (1 - t) + normals[i][0] * t, nz = normals[i - 1][1] * (1 - t) + normals[i][1] * t;
            put(fine, x + nx * walk.width * .38 * side, -.12, z + nz * walk.width * .38 * side,
              3.5, .17, .11, "#726248", "wood", "box", yaw);
            put(fine, x + nx * walk.width * .38 * side, -.36, z + nz * walk.width * .38 * side,
              .1, .7, .1, "#586355", "metal");
          }
          put(fine, x, -.2, z, .13, .13, walk.width * .87, "#646852", "metal", "box", yaw);
        }
      }
    }
  }

  const release = (root?: THREE.Group) => {
    root?.traverse(o => { if (o instanceof THREE.InstancedMesh) o.dispose(); });
    root?.removeFromParent(); root?.clear();
  };
  const shadow = (root: THREE.Group | undefined, enabled: boolean) => {
    root?.traverse(o => { if (o instanceof THREE.InstancedMesh) o.castShadow = enabled && o.material !== materials.water; });
  };
  let disposed = false, lastX = Infinity, lastZ = Infinity, night = false, landmarkShadow = false;
  return {
    group,
    update(target: THREE.Vector3, daylight: number): boolean {
      if (disposed || !Number.isFinite(target.x) || !Number.isFinite(target.z)) return false;
      const dark = Number.isFinite(daylight) && daylight < .15;
      if (Math.hypot(target.x - lastX, target.z - lastZ) < 12 && dark === night) return false;
      lastX = target.x; lastZ = target.z; night = dark;
      let changed = false;
      for (const cell of cells.values()) {
        let cellChanged = false;
        const distance = Math.hypot(cell.x - target.x, cell.z - target.z);
        if (distance < VISIBLE_ENTER && !cell.base && cell.parts.size) {
          cell.base = batch(cell.parts, "Landscape silhouettes");
          group.add(cell.base); cellChanged = true;
        } else if (distance > VISIBLE_LEAVE && cell.base) { release(cell.base); cell.base = undefined; cellChanged = true; }
        if (distance < DETAIL_ENTER && !cell.detail && cell.fine.size) {
          cell.detail = batch(cell.fine, "Nearby roots, joinery and planting");
          group.add(cell.detail); cellChanged = true;
        } else if (distance > DETAIL_LEAVE && cell.detail) { release(cell.detail); cell.detail = undefined; cellChanged = true; }
        const wanted = !dark && distance < (cell.shadows ? SHADOW_LEAVE : SHADOW_ENTER);
        if (wanted !== cell.shadows || cellChanged) {
          // New batches inherit the active cell's shadow state too.
          shadow(cell.base, wanted); shadow(cell.detail, wanted);
          if (wanted !== cell.shadows && (cell.base || cell.detail)) changed = true;
          cell.shadows = wanted;
        }
        changed = changed || cellChanged;
      }
      const nearLandmark = plan.pointsOfInterest.some(p => ["heritage-gardens", "lagoon-link"].includes(p.id)
        && Math.hypot(p.x - target.x, p.z - target.z) < (landmarkShadow ? 220 : 170));
      const wanted = !dark && nearLandmark;
      if (wanted !== landmarkShadow) { shadow(landmarks, wanted); landmarkShadow = wanted; changed = true; }
      return changed;
    },
    dispose() {
      if (disposed) return;
      disposed = true;
      for (const cell of cells.values()) { release(cell.base); release(cell.detail); }
      release(landmarks); cells.clear(); tints.length = 0; group.removeFromParent(); group.clear();
      Object.values(geometries).forEach(g => g.dispose());
      Object.values(materials).forEach(m => m.dispose());
    },
  };
}

function palmFrond() {
  const vertices: number[] = [];
  const centre = (t: number) => [0, .28 * Math.sin(t * Math.PI) - .32 * t * t, t];
  const triangle = (a: number[], b: number[], c: number[]) => vertices.push(...a, ...b, ...c);
  for (let i = 0; i < 10; i++) {
    const t = i / 10, next = (i + 1) / 10, a = centre(t), b = centre(next);
    triangle([-.015, a[1], a[2]], [.015, a[1], a[2]], [.009, b[1], b[2]]);
    triangle([-.015, a[1], a[2]], [.009, b[1], b[2]], [-.009, b[1], b[2]]);
    if (!i) continue;
    for (const side of [-1, 1]) {
      const width = .27 * Math.sin(t * Math.PI) + .015;
      triangle(a, [side * width, a[1] - .055, t + .13], [0, b[1], next]);
    }
  }
  const geometry = new THREE.BufferGeometry();
  geometry.setAttribute("position", new THREE.Float32BufferAttribute(vertices, 3)); geometry.computeVertexNormals();
  return geometry;
}

function taperedPylon() {
  const geometry = new THREE.BoxGeometry(1, 1, 1);
  const positions = geometry.getAttribute("position");
  for (let i = 0; i < positions.count; i++) {
    const taper = positions.getY(i) > 0 ? .78 : 1;
    positions.setXYZ(i, positions.getX(i) * taper, positions.getY(i), positions.getZ(i) * taper);
  }
  geometry.computeVertexNormals();
  return geometry;
}

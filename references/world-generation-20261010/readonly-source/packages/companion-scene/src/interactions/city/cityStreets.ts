import * as THREE from "three";
import { companionCityPlan as plan } from "../../../../../packages/utils/src/companionCity";
import { cityStreetProfile, sampleCityStreet } from "../../../../../packages/utils/src/companionCityStreetDetails";
import { companionCityDividedRoads, companionCityRoadSurface } from "../../../../../packages/utils/src/companionCityRoadTreatments";
import { companionCityParkLake, companionCityParkWalkingRoads } from "../../../../../packages/utils/src/companionCityParks";
import { applyCityStreetFinish } from "./cityStreetFinish";

type Surface = { count: number; cursor: number; vertices: Float32Array; colours: Float32Array; uvs: Float32Array };
/** Joined road ribbons follow the authored bends. All junctions stay flush. */
export function createCityStreets(roadTreatments = false, parkPaths = false) {
  const root = new THREE.Group(); root.name = "District streets and public spaces";
  const geometries: THREE.BufferGeometry[] = [], materials: THREE.Material[] = [];
  const surface = (): Surface => ({ count: 0, cursor: 0, vertices: new Float32Array(0), colours: new Float32Array(0), uvs: new Float32Array(0) });
  const asphalt = surface(), paving = surface(), earth = surface(), gravel = surface();
  const paths = surface(), sandPaths = surface(), boardwalks = surface(), paint = surface(), greens = surface();
  const surfaces = [greens, paths, sandPaths, boardwalks, paving, asphalt, earth, gravel, paint];
  const colour = new THREE.Color();
  // Ribbons consume these points synchronously; no geometry retains them.
  const paintPoints = [[0, 0], [0, 0]];
  const vertex = (surface: Surface, x: number, y: number, z: number, u = 0, v = 0) => {
    const index = surface.cursor++, position = index * 3, uv = index * 2;
    surface.vertices[position] = x; surface.vertices[position + 1] = y; surface.vertices[position + 2] = z;
    surface.colours[position] = colour.r; surface.colours[position + 1] = colour.g; surface.colours[position + 2] = colour.b;
    surface.uvs[uv] = u; surface.uvs[uv + 1] = v;
  };
  // Count through the same recipe before allocating final attributes. The first
  // pass skips profiles, ribbon edges, colours and generated paint points.
  const populate = (counting: boolean) => {
    const ribbon = (surface: Surface, points: number[][], width: number, y: number, tint: string | ((segment: number) => string)) => {
      if (counting) { surface.count += Math.max(0, points.length - 1) * 6; return; }
      let distance = 0, previousDistance = 0;
      let previousLeftX = 0, previousLeftZ = 0, previousRightX = 0, previousRightZ = 0;
      for (let i = 0; i < points.length; i++) {
        const point = points[i];
        if (i) distance += Math.hypot(point[0] - points[i - 1][0], point[1] - points[i - 1][1]);
        const a = points[Math.max(0, i - 1)], b = points[Math.min(points.length - 1, i + 1)];
        const length = Math.hypot(b[0] - a[0], b[1] - a[1]) || 1;
        const x = -(b[1] - a[1]) / length * width / 2, z = (b[0] - a[0]) / length * width / 2;
        const leftX = point[0] + x, leftZ = point[1] + z, rightX = point[0] - x, rightZ = point[1] - z;
        if (i) {
          colour.set(typeof tint === "string" ? tint : tint(i));
          vertex(surface, previousLeftX, y, previousLeftZ, width / 2, previousDistance);
          vertex(surface, leftX, y, leftZ, width / 2, distance);
          vertex(surface, previousRightX, y, previousRightZ, -width / 2, previousDistance);
          vertex(surface, previousRightX, y, previousRightZ, -width / 2, previousDistance);
          vertex(surface, leftX, y, leftZ, width / 2, distance);
          vertex(surface, rightX, y, rightZ, -width / 2, distance);
        }
        previousDistance = distance;
        previousLeftX = leftX; previousLeftZ = leftZ; previousRightX = rightX; previousRightZ = rightZ;
      }
    };
    for (const road of parkPaths ? companionCityParkWalkingRoads : plan.roads) {
      if (road.kind === "retained") continue;
      if (["path","sand-path","boardwalk"].includes(road.kind)) {
        const timber=road.kind==="boardwalk";
        ribbon(timber ? boardwalks : road.kind === "sand-path" ? sandPaths : paths,
          road.points,road.width,-.008,timber?"#ac9673":road.kind==="sand-path"?"#c4af86":"#c6b898");
        if(timber) for(let i=1;i<road.points.length;i++) {
          const a=road.points[i-1],b=road.points[i],length=Math.hypot(b[0]-a[0],b[1]-a[1]);
          if(length<.01)continue;
          const nx=-(b[1]-a[1])/length*road.width/2,nz=(b[0]-a[0])/length*road.width/2;
          for(let d=.8;d<length;d+=.8) {
            if (counting) { paint.count += 6; continue; }
            const x=a[0]+(b[0]-a[0])*d/length,z=a[1]+(b[1]-a[1])*d/length;
            paintPoints[0][0] = x + nx; paintPoints[0][1] = z + nz;
            paintPoints[1][0] = x - nx; paintPoints[1][1] = z - nz;
            ribbon(paint,paintPoints,.025,-.005,"#796a51");
          }
        }
        continue;
      }
      let distance = 0;
      const finishes = counting ? null : road.points.map((point, i) => {
        if (i) distance += Math.hypot(point[0] - road.points[i - 1][0], point[1] - road.points[i - 1][1]);
        return cityStreetProfile(road, sampleCityStreet(road, distance));
      });
      ribbon(paving, road.points, road.width + road.pavement * 2, -.02, i => finishes![i].paving);
      const surface = roadTreatments ? companionCityRoadSurface(road.id) : "asphalt";
      const divided = roadTreatments && companionCityDividedRoads.has(road.id);
      ribbon(surface === "earth" ? earth : surface === "gravel" ? gravel : asphalt, road.points,
        divided ? 10.2 : road.width, -.012, surface === "earth" ? "#aa7e53" : surface === "gravel" ? "#9d927c" : i => finishes![i].asphalt);
      if (divided) {
        // One 3.8 m carriage lane each way. The original reserve becomes paved shoulders, not extra lanes.
        for (const side of [-1, 1]) {
          if (counting) { paint.count += Math.max(0, road.points.length - 1) * 6; continue; }
          const points = road.points.map((p, i) => {
            const a = road.points[Math.max(0, i - 1)], b = road.points[Math.min(road.points.length - 1, i + 1)];
            const length = Math.hypot(b[0] - a[0], b[1] - a[1]) || 1;
            return [p[0] - (b[1] - a[1]) / length * side * 4.95, p[1] + (b[0] - a[0]) / length * side * 4.95];
          });
          ribbon(paint, points, .12, -.006, "#ded4ac");
        }
      }
      if (surface !== "asphalt" || divided) continue;
      if (road.width < 14) continue;
      let travelled = 0;
      for (let i = 1; i < road.points.length; i++) {
        const a = road.points[i-1], b = road.points[i], length = Math.hypot(b[0]-a[0], b[1]-a[1]);
        if (length < .01) continue;
        for (let d = Math.ceil(travelled / 16) * 16 - travelled; d < length - 1; d += 16) {
          if (counting) { paint.count += 6; continue; }
          const end = Math.min(length, d + 5);
          paintPoints[0][0] = a[0] + (b[0] - a[0]) * d / length;
          paintPoints[0][1] = a[1] + (b[1] - a[1]) * d / length;
          paintPoints[1][0] = a[0] + (b[0] - a[0]) * end / length;
          paintPoints[1][1] = a[1] + (b[1] - a[1]) * end / length;
          ribbon(paint, paintPoints, .14, -.006, "#ded4ac");
        }
        travelled += length;
      }
    }
    for (const space of plan.greenSpaces) {
      if (parkPaths && space.id === companionCityParkLake.spaceId) continue; // Terrain owns the garden fill with its real lake opening.
      if (counting) { greens.count += space.points.length * 3; continue; }
      const tint = space.kind === "garden" ? "#839766" : space.kind === "park" ? "#788b57" : space.kind === "market" ? "#b49c7b" : "#c4bca5";
      colour.set(tint);
      for (let i = 0; i < space.points.length; i++) {
        const a = space.points[i], b = space.points[(i+1)%space.points.length];
        vertex(greens, space.x, -.025, space.z);
        vertex(greens, b[0], -.025, b[1]);
        vertex(greens, a[0], -.025, a[1]);
      }
    }
  };
  populate(true);
  for (const surface of surfaces) {
    surface.vertices = new Float32Array(surface.count * 3);
    surface.colours = new Float32Array(surface.count * 3);
    surface.uvs = new Float32Array(surface.count * 2);
  }
  populate(false);
  if (surfaces.some(surface => surface.cursor !== surface.count)) throw new Error("City street vertex count changed during preparation.");
  for (const surface of surfaces) {
    if (!surface.vertices.length) continue;
    const geometry = new THREE.BufferGeometry();
    geometry.setAttribute("position",new THREE.BufferAttribute(surface.vertices,3));
    geometry.setAttribute("color",new THREE.BufferAttribute(surface.colours,3));
    geometry.setAttribute("uv",new THREE.BufferAttribute(surface.uvs,2));
    geometry.computeVertexNormals(); geometry.computeBoundingSphere();
    const material = new THREE.MeshStandardMaterial({ vertexColors: true, roughness: .94 });
    if (surface === earth || surface === gravel) {
      const soil = surface === earth;
      material.onBeforeCompile = shader => {
        shader.vertexShader = "varying vec3 streetSurfacePoint;\n" + shader.vertexShader;
        shader.vertexShader = shader.vertexShader.replace("#include <begin_vertex>",
          "#include <begin_vertex>\nstreetSurfacePoint=(modelMatrix*vec4(position,1.)).xyz;");
        shader.fragmentShader = "varying vec3 streetSurfacePoint;\n" + shader.fragmentShader;
        shader.fragmentShader = shader.fragmentShader.replace("#include <color_fragment>", `
          #include <color_fragment>
          float grain=fract(sin(dot(floor(streetSurfacePoint.xz*${soil ? "22." : "13."}),vec2(127.1,311.7)))*43758.5453);
          float wear=sin(streetSurfacePoint.x*.13)*sin(streetSurfacePoint.z*.17);
          diffuseColor.rgb*=.87+grain*${soil ? ".19" : ".27"}+wear*.06;`);
      };
      material.customProgramCacheKey = () => soil ? "city-street-earth-v1" : "city-street-gravel-v1";
    } else if (surface === asphalt || surface === paving || surface === paths || surface === sandPaths || surface === boardwalks)
      applyCityStreetFinish(material, surface === asphalt ? "asphalt" : surface === paving ? "paving"
        : surface === sandPaths ? "sand" : surface === boardwalks ? "timber" : "path");
    const mesh = new THREE.Mesh(geometry,material); mesh.receiveShadow = true;
    root.add(mesh); geometries.push(geometry); materials.push(material);
  }
  return { group: root, dispose() { root.removeFromParent(); geometries.forEach(g=>g.dispose()); materials.forEach(m=>m.dispose()); } };
}

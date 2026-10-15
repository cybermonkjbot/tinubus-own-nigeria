import * as THREE from "three";

export type LagosSurface = "clean_asphalt" | "pothole" | "laterite_mud" | "gutter_wood";
export type ScatterPlacement = { point: THREE.Vector3; along: number; lateral: number; radius: number; kind: number };
type ScatterOptions = {
  seed?: number;
  capacity?: number;
  spacing?: number;
  accept?: (placement: ScatterPlacement) => boolean;
  /** Solid clutter needs an existing collision reserve. Otherwise use only flat scatter. */
  allowSolid?: (placement: ScatterPlacement) => boolean;
};
type Patch = { x: number; z: number; radius: number; surface: LagosSurface };
const TILE = 64, COLUMNS = 3, ROWS = 2;
const UP = new THREE.Vector3(0, 1, 0);

/** One shared geometry, atlas material and instanced draw for all six prop variants. */
export class LagosScatter {
  readonly group = new THREE.Group();
  private mesh?: THREE.InstancedMesh;
  private geometry?: THREE.BufferGeometry;
  private material?: THREE.MeshStandardMaterial;
  private textures: THREE.DataTexture[] = [];
  private patches: Patch[] = [];
  // Retain the exact GPU matrix values and double-precision visibility inputs,
  // without a Matrix4, elements array, Vector3 and record for every prop.
  private placementMatrices = new Float32Array(0);
  private placementData = new Float64Array(0); // x, z, kind
  private placementCount = 0;
  private view = new THREE.Vector3(Infinity, 0, Infinity);
  private randomState: number;
  private disposed = false;
  private dirty = false;
  private daylight = { value: 1 };
  private readonly matrix = new THREE.Matrix4();
  private readonly rotation = new THREE.Quaternion();
  private readonly scale = new THREE.Vector3();

  constructor(private readonly options: ScatterOptions = {}) {
    this.randomState = (options.seed ?? 2166136261) >>> 0;
    this.group.name = "Instanced Lagos street scatter";
  }
  private random() {
    this.randomState = (Math.imul(this.randomState, 1664525) + 1013904223) >>> 0;
    return this.randomState / 4294967296;
  }
  init() {
    if (this.disposed || this.mesh) return;
    const capacity = Math.max(1, Math.min(8192, Math.floor(this.options.capacity ?? 4096)));
    const plane = new THREE.PlaneGeometry(1, 1, 6, 6).rotateX(-Math.PI / 2);
    const block = new THREE.BoxGeometry(.4, .2, .2).translate(0, .1, 0);
    const tire = new THREE.TorusGeometry(.28, .095, 4, 12).rotateX(-Math.PI / 2).translate(0, .095, 0);
    const sources = [plane, block, tire];
    let vertexCount = 0, indexCount = 0;
    for (const source of sources) {
      const count = source.getAttribute("position").count;
      vertexCount += count; indexCount += source.index?.count ?? count;
    }
    const positions = new Float32Array(vertexCount * 3), normals = new Float32Array(vertexCount * 3);
    const uvs = new Float32Array(vertexCount * 2), shapes = new Float32Array(vertexCount);
    const indices = vertexCount > 65535 ? new Uint32Array(indexCount) : new Uint16Array(indexCount);
    let vertexOffset = 0, indexOffset = 0;
    // Keep each primitive's shared vertices and seams. The concatenated index
    // stream retains the exact triangle order of the former expanded geometry.
    for (const [shape, source] of sources.entries()) {
      const p = source.getAttribute("position"), n = source.getAttribute("normal"), uv = source.getAttribute("uv");
      for (let i = 0; i < p.count; i++) {
        const vertex = vertexOffset + i, xyz = vertex * 3, xy = vertex * 2;
        positions[xyz] = p.getX(i); positions[xyz + 1] = p.getY(i); positions[xyz + 2] = p.getZ(i);
        normals[xyz] = n.getX(i); normals[xyz + 1] = n.getY(i); normals[xyz + 2] = n.getZ(i);
        uvs[xy] = uv.getX(i); uvs[xy + 1] = uv.getY(i); shapes[vertex] = shape;
      }
      const sourceIndex = source.index, count = sourceIndex?.count ?? p.count;
      for (let i = 0; i < count; i++) indices[indexOffset++] = vertexOffset + (sourceIndex ? sourceIndex.getX(i) : i);
      vertexOffset += p.count; source.dispose();
    }
    const geometry = new THREE.BufferGeometry();
    geometry.setIndex(new THREE.BufferAttribute(indices, 1));
    geometry.setAttribute("position", new THREE.BufferAttribute(positions, 3));
    geometry.setAttribute("normal", new THREE.BufferAttribute(normals, 3));
    geometry.setAttribute("uv", new THREE.BufferAttribute(uvs, 2));
    geometry.setAttribute("scatterShape", new THREE.BufferAttribute(shapes, 1));
    geometry.setAttribute("scatterKind", new THREE.InstancedBufferAttribute(new Float32Array(capacity), 1));
    const [map, normalMap] = this.atlas();
    const material = new THREE.MeshStandardMaterial({ map, normalMap,
      roughness: .94, alphaTest: .35,
      polygonOffset: true, polygonOffsetFactor: -1, polygonOffsetUnits: -1 });
    material.onBeforeCompile = shader => {
      shader.uniforms.scatterDaylight = this.daylight;
      shader.vertexShader = "attribute float scatterShape; attribute float scatterKind; varying float scatterVisible; varying float patchKind; varying vec2 patchUv; varying vec3 patchPoint;\n" + shader.vertexShader;
      shader.vertexShader = shader.vertexShader.replace("#include <uv_vertex>", `
        #include <uv_vertex>
        vec2 atlasUv=(clamp(uv,vec2(.02),vec2(.98))+vec2(mod(scatterKind,3.),floor(scatterKind/3.)))/vec2(3.,2.);
        vMapUv=atlasUv; vNormalMapUv=atlasUv;
        float shape=scatterKind<4.?0.:scatterKind<5.?1.:2.;
        scatterVisible=1.-step(.5,abs(scatterShape-shape)); patchKind=scatterKind; patchUv=uv;`);
      shader.vertexShader = shader.vertexShader.replace("#include <begin_vertex>", `#include <begin_vertex>
        transformed*=scatterVisible;
        vec4 patchVertex=vec4(transformed,1.);
        #ifdef USE_INSTANCING
          patchVertex=instanceMatrix*patchVertex;
        #endif
        patchPoint=(modelMatrix*patchVertex).xyz;`);
      shader.fragmentShader = "uniform float scatterDaylight; varying float scatterVisible; varying float patchKind; varying vec2 patchUv; varying vec3 patchPoint;\n" + shader.fragmentShader;
      shader.fragmentShader = shader.fragmentShader.replace("#include <clipping_planes_fragment>",
        "#include <clipping_planes_fragment>\nif(scatterVisible<.5)discard;");
      shader.fragmentShader = shader.fragmentShader.replace("#include <map_fragment>", `
        #include <map_fragment>
        vec2 crater=(patchUv-.5)*2.;
        float craterRadius=length(crater);
        float craterRim=.78+.055*sin(atan(crater.y,crater.x)*7.);
        float edgeGrain=fract(sin(dot(floor(patchPoint.xz*37.),vec2(127.1,311.7)))*43758.5453);
        float wetBottom=0.;
        if(patchKind>1.5 && patchKind<2.5) {
          diffuseColor.a*=smoothstep(.01,.19,craterRim-craterRadius+(edgeGrain-.5)*.1);
        }
        if(patchKind>2.5 && patchKind<3.5) {
          // Shaded crater walls, not displacement of the road's authoritative contact plane.
          vec3 eye=normalize(cameraPosition-patchPoint);
          vec3 patchDx=dFdx(patchPoint),patchDy=dFdy(patchPoint);
          vec2 uvDx=dFdx(patchUv),uvDy=dFdy(patchUv);
          float handedness=sign(uvDx.x*uvDy.y-uvDx.y*uvDy.x);
          vec3 patchTangent=normalize(patchDx*uvDy.y-patchDy*uvDx.y)*handedness;
          vec3 patchBitangent=normalize(patchDy*uvDx.x-patchDx*uvDy.x)*handedness;
          vec2 parallax=vec2(dot(eye,patchTangent),dot(eye,patchBitangent))*.09/max(.3,abs(eye.y));
          float depthRadius=length(crater+parallax*(1.-smoothstep(.2,.72,craterRadius)));
          wetBottom=1.-smoothstep(.27,.5,depthRadius);
          diffuseColor.rgb*=.57+.43*smoothstep(.18,.76,depthRadius);
          diffuseColor.a*=smoothstep(.0,.08,craterRim-craterRadius);
        }`);
      shader.fragmentShader = shader.fragmentShader.replace("#include <roughnessmap_fragment>",
        "#include <roughnessmap_fragment>\nroughnessFactor=mix(roughnessFactor,.2,wetBottom);");
      shader.fragmentShader = shader.fragmentShader.replace("#include <emissivemap_fragment>", `
        #include <emissivemap_fragment>
        // Restrained sky tint follows the same lighting as the street, with no reflection render pass.
        vec3 eyeDirection=normalize(cameraPosition-patchPoint);
        float wetFresnel=.06+.94*pow(1.-abs(eyeDirection.y),5.);
        totalEmissiveRadiance+=vec3(.10,.15,.18)*wetBottom*wetFresnel*scatterDaylight;`);
    };
    material.customProgramCacheKey = () => "lagos-scatter-r180-atlas-v2";
    const mesh = new THREE.InstancedMesh(geometry, material, capacity);
    mesh.count = 0; mesh.instanceMatrix.setUsage(THREE.DynamicDrawUsage);
    mesh.receiveShadow = false; mesh.castShadow = false;
    // Shader-selected shape bounds are enclosed by the complete shared geometry.
    mesh.name = "Sachets, laterite, potholes, blocks and tires";
    this.geometry = geometry; this.material = material; this.mesh = mesh; this.group.add(mesh);
  }
  private atlas() {
    const width = TILE * COLUMNS, height = TILE * ROWS;
    const color = new Uint8Array(width * height * 4), normal = new Uint8Array(color.length);
    const heights = new Float32Array(width * height);
    const colors = [[48, 125, 174], [178, 196, 193], [151, 88, 57], [48, 48, 42], [135, 133, 119], [40, 42, 38]];
    for (let kind = 0; kind < 6; kind++) for (let y = 0; y < TILE; y++) for (let x = 0; x < TILE; x++) {
      const px = (kind % COLUMNS) * TILE + x, py = Math.floor(kind / COLUMNS) * TILE + y, i = py * width + px;
      const u = (x + .5) / TILE * 2 - 1, v = (y + .5) / TILE * 2 - 1;
      const grain = ((Math.imul(x + kind * 137, 374761393) ^ Math.imul(y + 17, 668265263)) >>> 0) / 4294967296;
      const r = Math.hypot(u, v), rim = .78 + .07 * Math.sin(Math.atan2(v, u) * 7 + kind);
      const alpha = kind < 2 ? Math.abs(u) < .88 && Math.abs(v + .09 * Math.sin(u * 8)) < .67
        : kind < 4 ? r < rim : true;
      const fold = kind < 2 ? .14 * Math.sin((u + v * .4) * 19) : 0;
      const shade = kind === 3 ? .65 + Math.min(1, r / rim) * .55 : .88 + grain * .24 + fold;
      for (let channel = 0; channel < 3; channel++) color[i * 4 + channel] = colors[kind][channel] * shade;
      color[i * 4 + 3] = alpha ? 255 : 0;
      heights[i] = kind === 3 ? .12 + .7 * Math.min(1, r / rim) ** 3 : .5 + fold + (grain - .5) * .08;
    }
    for (let py = 0; py < height; py++) for (let px = 0; px < width; px++) {
      const i = py * width + px, x = px % TILE, y = py % TILE;
      const dx = heights[i + (x < TILE - 1 ? 1 : 0)] - heights[i - (x > 0 ? 1 : 0)];
      const dy = heights[i + (y < TILE - 1 ? width : 0)] - heights[i - (y > 0 ? width : 0)];
      const length = Math.hypot(dx * 3, dy * 3, 1);
      const offset = i * 4;
      normal[offset] = 128 - dx * 3 / length * 127; normal[offset + 1] = 128 - dy * 3 / length * 127;
      normal[offset + 2] = 128 + 127 / length; normal[offset + 3] = 255;
    }
    const textures = [color, normal].map(data => {
      const texture = new THREE.DataTexture(data, width, height, THREE.RGBAFormat);
      texture.minFilter = texture.magFilter = THREE.LinearFilter;
      texture.needsUpdate = true; return texture;
    });
    textures[0].colorSpace = THREE.SRGBColorSpace; this.textures = textures;
    return textures;
  }
  private resizePlacements(capacity: number) {
    const matrices = new Float32Array(capacity * 16), data = new Float64Array(capacity * 3);
    matrices.set(this.placementMatrices.subarray(0, this.placementCount * 16));
    data.set(this.placementData.subarray(0, this.placementCount * 3));
    this.placementMatrices = matrices; this.placementData = data;
  }
  /** Append paths, then call update once before rendering the completed batch. */
  spawnScatterAlongPath(points: THREE.Vector3[], roadWidth: number) {
    if (this.disposed || points.length < 2 || points.length > 2048 || !Number.isFinite(roadWidth) || roadWidth < .5
      || points.some(point => ![point.x, point.y, point.z].every(Number.isFinite))) return;
    this.init();
    const mesh = this.mesh!, kinds = this.geometry!.getAttribute("scatterKind");
    const spacing = Math.max(1, this.options.spacing ?? 7);
    let along = 0;
    for (let segment = 1; segment < points.length && this.placementCount < kinds.count; segment++) {
      const a = points[segment - 1], b = points[segment], dx = b.x - a.x, dz = b.z - a.z, length = Math.hypot(dx, dz);
      if (length < .01) continue;
      for (let d = spacing * .5; d < length && this.placementCount < kinds.count; d += spacing) {
        // Rejected entrances do not advance the ratio. Each complete group of 20 accepted props has 17 edge placements.
        const edge = this.placementCount % 20 < 17, side = this.random() < .5 ? -1 : 1;
        const lateral = side * roadWidth / 2 * (edge ? .85 + this.random() * .21 : this.random() * .85);
        let kind = this.random() < .5 ? Math.floor(this.random() * 2) : this.random() < .65 ? 2 : 3;
        const jitter = .75 + this.random() * .5;
        const point = new THREE.Vector3(a.x + dx * d / length - dz / length * lateral,
          THREE.MathUtils.lerp(a.y, b.y, d / length) + .009, a.z + dz * d / length + dx / length * lateral);
        const candidate = { point, along: along + d, lateral, radius: .6 * jitter, kind };
        candidate.radius = kind < 2 ? .16 * jitter : kind === 2 ? .8 * jitter : .85 * jitter;
        if (edge && this.random() < .12 && this.options.allowSolid?.(candidate)) {
          kind = this.random() < .5 ? 4 : 5; candidate.radius = .48 * jitter;
        }
        candidate.kind = kind;
        if (this.options.accept && !this.options.accept(candidate)) continue;
        const size = kind < 2 ? .22 : kind === 2 ? 1.1 : kind === 3 ? 1.2 : 1;
        this.rotation.setFromAxisAngle(UP, this.random() * Math.PI * 2);
        this.matrix.compose(point, this.rotation, this.scale.setScalar(jitter * size));
        if (this.placementCount === this.placementData.length / 3)
          this.resizePlacements(Math.min(kinds.count, Math.max(64, this.placementCount * 2)));
        mesh.setMatrixAt(this.placementCount, this.matrix); kinds.setX(this.placementCount, kind);
        this.placementMatrices.set(this.matrix.elements, this.placementCount * 16);
        const offset = this.placementCount * 3;
        this.placementData[offset] = point.x; this.placementData[offset + 1] = point.z; this.placementData[offset + 2] = kind;
        this.placementCount++;
        if (kind === 2 || kind === 3) this.patches.push({ x: point.x, z: point.z, radius: jitter * size * .4,
          surface: kind === 2 ? "laterite_mud" : "pothole" });
        this.dirty = true;
      }
      along += length;
    }
    mesh.count = this.placementCount;
    this.view.set(Infinity, 0, Infinity);
  }
  surfaceAt(x: number, z: number): LagosSurface | null {
    return this.patches.find(patch => Math.hypot(x - patch.x, z - patch.z) < patch.radius)?.surface ?? null;
  }
  setDaylight(value: number) {
    if (Number.isFinite(value)) this.daylight.value = THREE.MathUtils.clamp(value, 0, 1);
  }
  setView(target: THREE.Vector3, radius = 305) {
    if (!this.mesh || this.disposed || !Number.isFinite(target.x) || !Number.isFinite(target.z)
      || Math.hypot(target.x - this.view.x, target.z - this.view.z) < 20) return false;
    this.view.copy(target); this.mesh.count = 0;
    const kinds = this.geometry!.getAttribute("scatterKind");
    const matrices = this.mesh.instanceMatrix.array;
    for (let index = 0; index < this.placementCount; index++) {
      const offset = index * 3;
      if (!(Math.hypot(target.x - this.placementData[offset], target.z - this.placementData[offset + 1]) < radius)) continue;
      const source = index * 16, destination = this.mesh.count * 16;
      // Copy packed values without allocating a typed-array view per instance.
      for (let element = 0; element < 16; element++) matrices[destination + element] = this.placementMatrices[source + element];
      kinds.setX(this.mesh.count++, this.placementData[offset + 2]);
    }
    this.dirty = true; return this.update(0);
  }
  update(_delta: number) {
    if (!this.mesh || !this.dirty || this.disposed) return false;
    if (this.placementData.length !== this.placementCount * 3) this.resizePlacements(this.placementCount);
    this.mesh.instanceMatrix.needsUpdate = true; this.geometry!.getAttribute("scatterKind").needsUpdate = true;
    this.mesh.computeBoundingBox(); this.mesh.computeBoundingSphere(); this.dirty = false; return true;
  }
  dispose() {
    if (this.disposed) return;
    this.disposed = true; this.group.removeFromParent(); this.mesh?.dispose(); this.geometry?.dispose(); this.material?.dispose();
    this.textures.forEach(texture => texture.dispose()); this.textures = []; this.patches = [];
    this.placementMatrices = new Float32Array(0); this.placementData = new Float64Array(0); this.placementCount = 0;
    this.group.clear();
    this.mesh = undefined; this.geometry = undefined; this.material = undefined;
  }
}

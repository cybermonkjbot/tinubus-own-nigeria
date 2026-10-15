import * as THREE from "three";
import { companionCityPlan, companionCityBankZ } from "../../../../../packages/utils/src/companionCity";
import { companionCityParkLake } from "../../../../../packages/utils/src/companionCityParks";
import { companionCityTheatre } from "../../../../../packages/utils/src/companionCityTheatre";

/** Shore-aligned geometry carries bank heights into the shader without large uniform loops. */
export function createCityWater(parkPaths = false, theatreDistrict = false) {
  const { lagoon } = companionCityPlan;
  const anchors = [...new Set([...lagoon.southBank, ...lagoon.northBank].map(p=>p[0]))].sort((a,b)=>a-b);
  const xs: number[] = [];
  for (let i=1;i<anchors.length;i++) {
    const a=anchors[i-1], b=anchors[i], count=Math.ceil((b-a)/(Math.abs((a+b)/2)<2300?35:600));
    for(let j=0;j<count;j++) xs.push(a+(b-a)*j/count);
  }
  xs.push(anchors[anchors.length-1]);
  const rows = 32, vertexCount = xs.length * (rows + 1);
  const positions = new Float32Array(vertexCount * 3), banks = new Float32Array(vertexCount * 2);
  // Match Three's index-width rule, including the reserved 65535 restart value.
  const indices = vertexCount > 65535 ? new Uint32Array((xs.length - 1) * rows * 6)
    : new Uint16Array((xs.length - 1) * rows * 6);
  let vertex = 0, indexOffset = 0;
  for(const x of xs) {
    const south=companionCityBankZ(lagoon.southBank,x), north=companionCityBankZ(lagoon.northBank,x);
    for(let j=0;j<=rows;j++) {
      positions[vertex * 3] = x;
      positions[vertex * 3 + 1] = lagoon.level;
      positions[vertex * 3 + 2] = south + (north - south) * j / rows;
      banks[vertex * 2] = south; banks[vertex * 2 + 1] = north; vertex++;
    }
  }
  for(let i=1;i<xs.length;i++) for(let j=0;j<rows;j++) {
    const a=(i-1)*(rows+1)+j,b=i*(rows+1)+j;
    indices[indexOffset++] = a; indices[indexOffset++] = a + 1; indices[indexOffset++] = b;
    indices[indexOffset++] = a + 1; indices[indexOffset++] = b + 1; indices[indexOffset++] = b;
  }
  const geometry=new THREE.BufferGeometry();
  // BufferAttribute adopts these final arrays instead of copying them again.
  geometry.setAttribute("position",new THREE.BufferAttribute(positions,3));
  geometry.setAttribute("shoreBank",new THREE.BufferAttribute(banks,2));
  geometry.setIndex(new THREE.BufferAttribute(indices,1)); geometry.computeBoundingSphere();
  const uniforms = {
    time: { value: 0 },
    sunDirection: { value: new THREE.Vector3(-.48, .84, .24).normalize() },
    skyColour: { value: new THREE.Color("#aacbd7") }, daylight: { value: 1 },
    parkLake: { value: false }, lakeEnds: { value: new THREE.Vector2(companionCityParkLake.banks[0].z, companionCityParkLake.banks.at(-1)!.z) },
    theatreLake: { value: false },
    ...THREE.UniformsUtils.clone(THREE.UniformsLib.fog),
  };
  const material = new THREE.ShaderMaterial({ uniforms, fog: true,
    vertexShader: `
      uniform float time;
      uniform bool parkLake; uniform vec2 lakeEnds; uniform bool theatreLake;
      attribute vec2 shoreBank;
      varying vec2 waterBanks; varying vec3 waterPosition;
      #include <fog_pars_vertex>
      void main() {
        vec3 p = position;
        float shoreDistance = theatreLake ? 20. : parkLake ? min(min(p.x-shoreBank.x,shoreBank.y-p.x),min(p.z-lakeEnds.x,lakeEnds.y-p.z))
          : min(p.z-shoreBank.x,shoreBank.y-p.z);
        float shore = smoothstep(0., 9., shoreDistance);
        p.y += shore * (.045 * sin(p.x * .11 + p.z * .19 + time * .55)
          + .022 * sin(p.x * .32 - p.z * .17 + time * .8));
        waterBanks = shoreBank;
        waterPosition = (modelMatrix * vec4(p, 1.)).xyz;
        vec4 mvPosition = modelViewMatrix * vec4(p, 1.);
        gl_Position = projectionMatrix * mvPosition;
        #include <fog_vertex>
      }`,
    fragmentShader: `
      uniform float time;
      uniform bool parkLake; uniform vec2 lakeEnds; uniform bool theatreLake;
      uniform vec3 sunDirection; uniform vec3 skyColour; uniform float daylight;
      varying vec2 waterBanks; varying vec3 waterPosition;
      #include <fog_pars_fragment>
      float waterNoise(vec2 p) {
        vec2 i=floor(p),f=fract(p);f=f*f*(3.-2.*f);
        float a=fract(sin(dot(i,vec2(127.1,311.7)))*43758.5453);
        float b=fract(sin(dot(i+vec2(1.,0.),vec2(127.1,311.7)))*43758.5453);
        float c=fract(sin(dot(i+vec2(0.,1.),vec2(127.1,311.7)))*43758.5453);
        float d=fract(sin(dot(i+vec2(1.,1.),vec2(127.1,311.7)))*43758.5453);
        return mix(mix(a,b,f.x),mix(c,d,f.x),f.y);
      }
      void main() {
        vec2 p = waterPosition.xz;
        float shoreDistance = theatreLake ? 20. : parkLake ? min(min(p.x-waterBanks.x,waterBanks.y-p.x),min(p.y-lakeEnds.x,lakeEnds.y-p.y))
          : min(p.y-waterBanks.x,waterBanks.y-p.y);
        if(shoreDistance < 0.) discard;
        float a = p.x * .7 + p.y * .9 + time * 1.1;
        float b = p.x * 1.7 - p.y * .8 + time * 1.6;
        float c = p.x * 4.1 + p.y * 3.4 - time * 1.3;
        float bankCalm=mix(.16,1.,smoothstep(0.,10.,shoreDistance));
        vec3 normal = normalize(vec3(bankCalm*(-.055*cos(a)-.023*cos(b)-.008*cos(c)), 1.,
          bankCalm*(-.065*cos(a)+.015*cos(b)-.006*cos(c))));
        vec3 view = normalize(cameraPosition - waterPosition);
        float fresnel = .035 + .965 * pow(1. - max(dot(normal, view), 0.), 5.);
        float silt=waterNoise(p*.07);
        vec3 body = parkLake ? mix(vec3(.10,.17,.125)*(.93+silt*.14), vec3(.018,.073,.088), smoothstep(0.,28.,shoreDistance))
          : mix(vec3(.115,.13,.064),vec3(.078,.11,.049),smoothstep(0.,28.,shoreDistance))*(.93+silt*.14);
        vec3 reflected = mix(skyColour * .72, skyColour, clamp(reflect(-view, normal).y, 0., 1.));
        vec3 colour = mix(body * (.2 + daylight * .8), reflected, fresnel);
        float specular = pow(max(dot(reflect(-sunDirection, normal), view), 0.), 120.);
        colour += vec3(1.,.87,.64) * specular * daylight * .42;
        float wash=sin(shoreDistance*4.8-time*.42+waterNoise(p*.12)*2.);
        // Sparse ripple-edge flecks, never a continuous ocean-break stripe.
        float foam=(1.-smoothstep(.15,1.5,shoreDistance))*smoothstep(.78,.99,wash)
          *smoothstep(.57,.84,waterNoise(p*.5+vec2(time*.018,0.)));
        colour = mix(colour, vec3(.62,.64,.55) * (.3+daylight*.7), foam*.19);
        gl_FragColor = vec4(colour, 1.);
        #include <tonemapping_fragment>
        #include <colorspace_fragment>
        #include <fog_fragment>
      }`,
  });
  const mesh = new THREE.Mesh(geometry, material); mesh.name = "Irregular city lagoon";
  const group = new THREE.Group(); group.name = "City lagoon water"; group.add(mesh);
  let lakeGeometry: THREE.BufferGeometry | undefined, lakeMaterial: THREE.ShaderMaterial | undefined;
  if (parkPaths) {
    const lake=companionCityParkLake, columns=20;
    const sections: {z:number;left:number;right:number}[]=[];
    for (let i=1;i<lake.banks.length;i++) {
      const a=lake.banks[i-1],b=lake.banks[i],steps=Math.ceil((b.z-a.z)/4);
      for(let j=0;j<steps;j++) sections.push({z:a.z+(b.z-a.z)*j/steps,
        left:a.left+(b.left-a.left)*j/steps,right:a.right+(b.right-a.right)*j/steps});
    }
    sections.push(lake.banks[lake.banks.length-1]);
    const lakeVertexCount = sections.length * (columns + 1);
    const vertices = new Float32Array(lakeVertexCount * 3), shore = new Float32Array(lakeVertexCount * 2);
    const faces = lakeVertexCount > 65535 ? new Uint32Array((sections.length - 1) * columns * 6)
      : new Uint16Array((sections.length - 1) * columns * 6);
    let lakeVertex = 0, faceOffset = 0;
    for (const s of sections) for(let j=0;j<=columns;j++) {
      vertices[lakeVertex * 3] = s.left + (s.right - s.left) * j / columns;
      vertices[lakeVertex * 3 + 1] = lake.level; vertices[lakeVertex * 3 + 2] = s.z;
      shore[lakeVertex * 2] = s.left; shore[lakeVertex * 2 + 1] = s.right; lakeVertex++;
    }
    for(let i=1;i<sections.length;i++) for(let j=0;j<columns;j++) {
      const a=(i-1)*(columns+1)+j,b=i*(columns+1)+j;
      faces[faceOffset++] = a; faces[faceOffset++] = b; faces[faceOffset++] = a + 1;
      faces[faceOffset++] = a + 1; faces[faceOffset++] = b; faces[faceOffset++] = b + 1;
    }
    lakeGeometry=new THREE.BufferGeometry();
    lakeGeometry.setAttribute("position",new THREE.BufferAttribute(vertices,3));
    lakeGeometry.setAttribute("shoreBank",new THREE.BufferAttribute(shore,2));
    lakeGeometry.setIndex(new THREE.BufferAttribute(faces,1));lakeGeometry.computeBoundingSphere();
    lakeMaterial=new THREE.ShaderMaterial({vertexShader:material.vertexShader,fragmentShader:material.fragmentShader,fog:true,
      uniforms:{...uniforms,parkLake:{value:true}}});
    const lakeMesh=new THREE.Mesh(lakeGeometry,lakeMaterial);lakeMesh.name="Botanic connected lake";group.add(lakeMesh);
  }
  let disposed = false;
  const theatreGeometries: THREE.BufferGeometry[] = [], theatreMaterials: THREE.ShaderMaterial[] = [];
  if (theatreDistrict) for (const lake of companionCityTheatre.lakes) {
    const contour = lake.points.map(p => new THREE.Vector2(p[0], -p[1]));
    const lakeShape = new THREE.BufferGeometry();
    lakeShape.setAttribute("position", new THREE.Float32BufferAttribute(lake.points.flatMap(p => [p[0], lake.level, p[1]]), 3));
    lakeShape.setAttribute("shoreBank", new THREE.Float32BufferAttribute(lake.points.flatMap(() => [0, 0]), 2));
    lakeShape.setIndex(THREE.ShapeUtils.triangulateShape(contour, []).flat()); lakeShape.computeBoundingSphere();
    const lakeFinish = new THREE.ShaderMaterial({ vertexShader: material.vertexShader, fragmentShader: material.fragmentShader,
      fog: true, side: THREE.DoubleSide, uniforms: { ...uniforms, theatreLake: { value: true }, parkLake: { value: true } } });
    const basin = new THREE.Mesh(lakeShape, lakeFinish); basin.name = lake.id; group.add(basin);
    theatreGeometries.push(lakeShape); theatreMaterials.push(lakeFinish);
  }
  return { mesh, group,
    update(seconds: number, sun: THREE.Vector3, sky: THREE.Color, daylight: number) {
      if(disposed)return false;
      const time=Number.isFinite(seconds)?seconds:uniforms.time.value;
      const light=Number.isFinite(daylight)?THREE.MathUtils.clamp(daylight,0,1):uniforms.daylight.value;
      if(uniforms.time.value===time&&uniforms.daylight.value===light
        &&uniforms.sunDirection.value.equals(sun)&&uniforms.skyColour.value.equals(sky))return false;
      uniforms.time.value = time; uniforms.sunDirection.value.copy(sun);
      uniforms.skyColour.value.copy(sky); uniforms.daylight.value = light;
      return true;
    },
    dispose() {
      if(disposed)return;disposed=true;mesh.removeFromParent();group.removeFromParent();group.clear();
      geometry.dispose();material.dispose();
      lakeGeometry?.dispose();lakeMaterial?.dispose();
      theatreGeometries.forEach(shape => shape.dispose()); theatreMaterials.forEach(finish => finish.dispose());
    },
  };
}

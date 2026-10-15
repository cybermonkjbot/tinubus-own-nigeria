import * as THREE from "three";
import type { CompanionWaterZone } from "../../../../../packages/utils/src/companionWaterfront";

/** Local water patches share the existing scene clock, fog and reduced-motion owner. */
export function createWaterfrontWater(zone: CompanionWaterZone, width = 1200, depth = 1200) {
  const ocean = zone === "ocean", creek = zone === "creek";
  const geometry = new THREE.PlaneGeometry(width, depth, ocean ? 100 : 40, ocean ? 100 : 40);
  // Water shading derives normals and texture coordinates from world position.
  geometry.deleteAttribute("normal");
  geometry.deleteAttribute("uv");
  geometry.rotateX(-Math.PI / 2);
  const uniforms = { time: { value: 0 }, light: { value: 1 },
    ...THREE.UniformsUtils.clone(THREE.UniformsLib.fog) };
  const material = new THREE.ShaderMaterial({ uniforms, fog: true,
    vertexShader: `
      uniform float time; varying vec3 waterPoint;
      #include <fog_pars_vertex>
      void main() {
        vec3 p=position;
        float surf=smoothstep(28.,90.,-p.z);
        p.y+=${ocean ? "surf*(.42*sin(p.z*.095+time*1.3)+.18*sin(p.x*.034-p.z*.067+time*.8))" : creek ? ".009*sin(p.x*.4+p.z*.23+time*.15)" : ".045*sin(p.x*.11+p.z*.19+time*.55)"};
        waterPoint=(modelMatrix*vec4(p,1.)).xyz;
        vec4 mvPosition=modelViewMatrix*vec4(p,1.);gl_Position=projectionMatrix*mvPosition;
        #include <fog_vertex>
      }`,
    fragmentShader: `
      uniform float time; uniform float light; varying vec3 waterPoint;
      #include <fog_pars_fragment>
      void main() {
        vec2 p=waterPoint.xz;
        float wave=sin(p.y*.095+time*1.3),small=sin(p.x*.61+p.y*.74+time*.65);
        vec3 normal=normalize(vec3(-.03*cos(p.x*.61+time*.65),1.,-${ocean ? ".09" : ".025"}*cos(p.y*.095+time*1.3)));
        float fresnel=pow(1.-max(0.,dot(normal,normalize(cameraPosition-waterPoint))),4.);
        vec3 body=${creek ? "vec3(.019,.024,.016)" : ocean ? "mix(vec3(.07,.18,.22),vec3(.16,.26,.27),wave*.5+.5)" : "vec3(.09,.14,.064)"};
        vec3 colour=mix(body,vec3(.52,.66,.69),fresnel*.6)*(.17+.83*light);
        ${ocean ? "float surf=smoothstep(25.,42.,-p.y)*(1.-smoothstep(85.,170.,-p.y)); float foam=pow(max(0.,wave*.65+small*.15+.35),8.)*surf; colour=mix(colour,vec3(.8,.85,.81)*(.3+.7*light),clamp(foam,0.,.82));" : creek ? "float oil=sin(p.x*.15+p.y*.19+small*.08); colour+=vec3(.026,.009,.037)*pow(max(0.,oil),12.)*light;" : "colour+=vec3(.018,.018,.007)*small*light;"}
        gl_FragColor=vec4(colour,1.);
        #include <tonemapping_fragment>
        #include <colorspace_fragment>
        #include <fog_fragment>
      }`,
  });
  const mesh = new THREE.Mesh(geometry, material); mesh.position.y = -.22; mesh.name = `${zone} water`;
  return { mesh, update(seconds: number, daylight = 1) { uniforms.time.value = seconds; uniforms.light.value = daylight; },
    dispose() { mesh.removeFromParent(); geometry.dispose(); material.dispose(); } };
}

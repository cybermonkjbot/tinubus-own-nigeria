import type { MeshStandardMaterial } from "three";

export type CityStreetFinish = "asphalt" | "paving" | "path" | "sand" | "timber";

/** Metric UVs follow each ribbon. Subpixel grain averages out while resolved detail stays intact. */
export function applyCityStreetFinish(material: MeshStandardMaterial, finish: CityStreetFinish) {
  const joints = finish === "path" || finish === "paving";
  const timber = finish === "timber";
  const sand = finish === "sand";
  material.onBeforeCompile = shader => {
    shader.vertexShader = `varying vec2 streetMetric; varying vec3 streetFinishPoint;\n${shader.vertexShader}`
      .replace("#include <begin_vertex>", `#include <begin_vertex>
        streetMetric = uv;
        streetFinishPoint = (modelMatrix * vec4(transformed, 1.)).xyz;`);
    shader.fragmentShader = `varying vec2 streetMetric; varying vec3 streetFinishPoint;\n${shader.fragmentShader}`
      .replace("#include <color_fragment>", `#include <color_fragment>
        float streetRange = distance(cameraPosition, streetFinishPoint);
        vec2 grainPoint = streetFinishPoint.xz * ${sand ? "32." : "24."};
        float grainVisible = 1. - smoothstep(.35, 1., max(fwidth(grainPoint).x, fwidth(grainPoint).y));
        float streetGrain = mix(.5, fract(sin(dot(floor(grainPoint), vec2(127.1, 311.7))) * 43758.5453), grainVisible);
        float streetWash = sin(streetFinishPoint.x * .41) * sin(streetFinishPoint.z * .37);
        float streetNear = 1. - smoothstep(18., 90., streetRange);
        diffuseColor.rgb *= .986 + streetWash * .014 + (streetGrain - .5) * ${sand ? ".07" : ".045"} * streetNear;
        ${joints ? `
          vec2 course = vec2(streetMetric.x / 1.2, streetMetric.y / 2.4);
          course.x += mod(floor(course.y), 2.) * .5;
          vec2 jointDistance = min(fract(course), 1. - fract(course));
          vec2 jointAA = max(fwidth(course), vec2(.002));
          vec2 joint = 1. - smoothstep(vec2(.004), vec2(.004) + jointAA, jointDistance);
          float jointFade = 1. - smoothstep(55., 150., streetRange);
          diffuseColor.rgb *= 1. - max(joint.x, joint.y) * .16 * jointFade;` : ""}
        ${timber ? `
          float board = floor(streetMetric.y / .8);
          float boardTint = fract(sin(board * 19.17) * 43758.5453);
          float woodGrain = sin(streetMetric.y * 110. + sin(streetMetric.x * 2.3) * 3.);
          float woodVisible = 1. - smoothstep(1., 3.14, fwidth(streetMetric.y * 110.));
          diffuseColor.rgb *= .95 + boardTint * .07 + woodGrain * .025 * streetNear * woodVisible;` : ""}
        ${sand ? `
          float sandRipple = sin(streetMetric.y * 15. + sin(streetMetric.x * 1.7));
          float rippleVisible = 1. - smoothstep(1., 3.14, fwidth(streetMetric.y * 15.));
          diffuseColor.rgb *= 1. + sandRipple * .018 * streetNear * rippleVisible;` : ""}`);
    if (finish === "asphalt") {
      // A restrained crowned normal preserves the flat authoritative road and wheel contact.
      shader.fragmentShader = shader.fragmentShader.replace("#include <normal_fragment_maps>", `
        #include <normal_fragment_maps>
        vec3 crownedPoint = streetFinishPoint;
        crownedPoint.y += .25 * exp(-streetMetric.x * streetMetric.x / 18.);
        vec3 crownNormal = normalize(cross(dFdx(crownedPoint), dFdy(crownedPoint)));
        if (crownNormal.y < 0.) crownNormal = -crownNormal;
        vec3 crownViewNormal = normalize((viewMatrix * vec4(crownNormal, 0.)).xyz);
        normal = normalize(mix(normal, crownViewNormal, .75));`);
    }
  };
  material.customProgramCacheKey = () => `city-street-finish-${finish}-v3`;
}

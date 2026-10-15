import * as THREE from "three";
import { Sky } from "three/addons/objects/Sky.js";

/** Reversible outdoor lighting. Only the nearby shadow volume follows the viewer. */
export function createCityAtmosphere(scene: THREE.Scene, renderer: THREE.WebGLRenderer,
  camera: THREE.PerspectiveCamera, roomLights: readonly THREE.Light[]) {
  const previous = { fog: scene.fog,
    far: camera.far, lights: roomLights.map(light => light.visible) };
  const sky = new Sky(); sky.scale.setScalar(3500); sky.name = "City sky";
  sky.material.uniforms.turbidity.value = 3.4; sky.material.uniforms.rayleigh.value = 1.4;
  sky.material.uniforms.mieCoefficient.value = .004; sky.material.uniforms.mieDirectionalG.value = .78;
  const direction = new THREE.Vector3(-.48, .84, .24).normalize();
  sky.material.uniforms.sunPosition.value.copy(direction);
  scene.add(sky);
  const sun = new THREE.DirectionalLight("#fff0da", 2.7);
  const ambient = new THREE.HemisphereLight("#d1e5ef", "#837462", .75);
  sun.castShadow = false;
  const horizon = new THREE.Color("#bbcbd2"), night = new THREE.Color("#172738"), day = new THREE.Color("#bbcbd2");
  const fog = new THREE.Fog(horizon, 1000, 4200);
  scene.add(sun, sun.target, ambient); scene.fog = fog;
  roomLights.forEach(light => { light.visible = false; });
  renderer.shadowMap.enabled = false;
  renderer.shadowMap.autoUpdate = false; renderer.shadowMap.needsUpdate = false;
  camera.far = 9000; camera.updateProjectionMatrix();
  let lastHour = -1;
  return {
    direction, horizon, daylight: 1,
    update(hour: number, target: THREE.Vector3) {
      sky.position.copy(camera.position);
      const angle = (hour - 6) / 24 * Math.PI * 2;
      direction.set(-Math.cos(angle), Math.sin(angle), .24).normalize();
      this.daylight = THREE.MathUtils.smoothstep(direction.y, -.12, .22);
      if (Math.abs(hour - lastHour) > .005) {
        lastHour = hour;
        sky.material.uniforms.sunPosition.value.copy(direction);
        horizon.copy(night).lerp(day, this.daylight); fog.color.copy(horizon);
        if (this.daylight > 0.05) {
          sun.intensity = 2.7 * this.daylight;
          sun.color.setRGB(1, .65 + .29 * this.daylight, .4 + .43 * this.daylight);
        } else {
          sun.intensity = 0.35 * (1 - this.daylight);
          sun.color.set("#7ea0c6");
        }
        ambient.intensity = .2 + this.daylight * .55;
        renderer.shadowMap.needsUpdate = false;
      }
    },
    dispose() {
      scene.remove(sky, sun, sun.target, ambient); sky.geometry.dispose(); sky.material.dispose();
      scene.fog = previous.fog;
      camera.far = previous.far; camera.updateProjectionMatrix();
      roomLights.forEach((light, i) => { light.visible = previous.lights[i]; });
      renderer.shadowMap.enabled = false;
      renderer.shadowMap.autoUpdate = false; renderer.shadowMap.needsUpdate = false;
    },
  };
}

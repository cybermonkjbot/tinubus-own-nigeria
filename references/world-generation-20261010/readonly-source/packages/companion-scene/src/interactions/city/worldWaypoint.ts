import * as THREE from "three";
import { cityWaypointDistance, cityWaypointReached, type CityWaypoint } from "./cityWaypoint";
import "./world-waypoint.css";

/** Projects the selected pin with the actual camera and rendered player, including vehicle seats. */
export function createWorldWaypoint(scene: THREE.Scene, camera: THREE.PerspectiveCamera, host: HTMLElement) {
  const root = new THREE.Group(); root.name = "city-waypoint"; root.visible = false;
  const ringGeometry = new THREE.RingGeometry(1.5, 1.85, 48);
  const ringMaterial = new THREE.MeshBasicMaterial({ color: "#ab79ff", transparent: true, opacity: .85,
    side: THREE.DoubleSide, depthWrite: false, toneMapped: false });
  const ring = new THREE.Mesh(ringGeometry, ringMaterial);
  ring.rotation.x = -Math.PI / 2; ring.position.y = .06;
  const beamGeometry = new THREE.CylinderGeometry(.22, 1.4, 10, 24, 1, true);
  const beamMaterial = new THREE.MeshBasicMaterial({ color: "#ab79ff", transparent: true, opacity: .16,
    side: THREE.DoubleSide, depthWrite: false, toneMapped: false });
  const beam = new THREE.Mesh(beamGeometry, beamMaterial); beam.position.y = 5;
  // Guidance must not intercept floor/object picking or cast a gameplay shadow.
  ring.raycast = () => {}; beam.raycast = () => {};
  root.add(ring, beam); scene.add(root);

  const safe = document.createElement("div"); safe.className = "world-waypoint-safe";
  const marker = document.createElement("div"); marker.className = "world-waypoint"; marker.hidden = true;
  marker.setAttribute("role", "img");
  const icon = document.createElement("span"); icon.className = "world-waypoint-icon"; icon.setAttribute("aria-hidden", "true");
  const caption = document.createElement("span"); caption.className = "world-waypoint-caption";
  const name = document.createElement("span"); name.className = "world-waypoint-name";
  const distanceLabel = document.createElement("span"); distanceLabel.className = "world-waypoint-distance";
  caption.append(name, distanceLabel); marker.append(icon, caption); safe.append(marker); host.append(safe);
  const cameraPoint = new THREE.Vector3(), projection = new THREE.Vector3();
  let left = 0, top = 0, width = 0, height = 0, lastLabel = "";
  const resize = new ResizeObserver(() => {
    left = safe.offsetLeft; top = safe.offsetTop; width = safe.clientWidth; height = safe.clientHeight;
  });
  resize.observe(safe);

  return {
    update(waypoint: CityWaypoint | null, player: THREE.Vector3 | undefined, floorY: number, viewportWidth: number, viewportHeight: number) {
      if (!waypoint || !player || width <= 0 || height <= 0) {
        root.visible = false; marker.hidden = true; return;
      }
      const distance = Math.hypot(waypoint.x - player.x, waypoint.z - player.z);
      const reached = cityWaypointReached(distance);
      root.position.set(waypoint.x, floorY, waypoint.z); root.visible = true;
      beam.visible = !reached;
      cameraPoint.set(waypoint.x, floorY + 3, waypoint.z).applyMatrix4(camera.matrixWorldInverse);
      projection.set(waypoint.x, floorY + 3, waypoint.z).project(camera);
      const projectedX = (projection.x + 1) * viewportWidth / 2 - left;
      const projectedY = (1 - projection.y) * viewportHeight / 2 - top;
      const onScreen = cameraPoint.z < -camera.near && projection.z >= -1 && projection.z <= 1
        && projectedX >= 20 && projectedX <= width - 20 && projectedY >= 20 && projectedY <= height - 20;
      let x = projectedX, y = projectedY, angle = 0;
      if (!onScreen) {
        // Camera-space bearing avoids mirroring the arrow when a target is behind the camera.
        const dx = cameraPoint.x, dy = cameraPoint.z;
        const length = Math.hypot(dx, dy);
        const bearingX = length > .001 ? dx / length : 0;
        const bearingY = length > .001 ? dy / length : 1;
        const halfWidth = Math.max(1, width / 2 - 20), halfHeight = Math.max(1, height / 2 - 20);
        const edge = 1 / Math.max(Math.abs(bearingX) / halfWidth, Math.abs(bearingY) / halfHeight);
        x = width / 2 + bearingX * edge; y = height / 2 + bearingY * edge;
        angle = Math.atan2(bearingY, bearingX) * 180 / Math.PI + 90;
      }
      marker.hidden = false;
      marker.dataset.edge = String(!onScreen); marker.dataset.reached = String(reached);
      marker.dataset.align = x < 100 ? "left" : x > width - 100 ? "right" : "center";
      marker.style.transform = `translate(${x.toFixed(1)}px, ${y.toFixed(1)}px)`;
      icon.style.setProperty("--waypoint-bearing", `${angle.toFixed(1)}deg`);
      const detail = reached ? "At waypoint" : cityWaypointDistance(distance);
      if (name.textContent !== waypoint.label) name.textContent = waypoint.label;
      if (distanceLabel.textContent !== detail) distanceLabel.textContent = detail;
      const label = `${waypoint.label}. ${detail}${!onScreen ? ". Offscreen" : ""}.`;
      if (label !== lastLabel) { marker.setAttribute("aria-label", label); lastLabel = label; }
    },
    dispose() {
      resize.disconnect(); safe.remove(); root.removeFromParent();
      ringGeometry.dispose(); ringMaterial.dispose(); beamGeometry.dispose(); beamMaterial.dispose();
    },
  };
}

import * as THREE from 'three';
import type { PersonalRoom } from '../../../../../packages/utils/src/companion';
import { roomLayoutFor } from '../../../../../packages/utils/src/companionRoomLayouts';
import { roomNavigationFor } from '../../../../../packages/utils/src/companionNavigation';
import type { CompanionEnvironment } from './environments';
import { applyCustomRoomColors, customRoomMaterialColor } from './roomMaterials';
import { terracePalettes } from './terraceGeometry';

const paletteSlots = {
  rose: { wall: '#d9c6ba', upholstery: '#b98078', rug: '#a65366' },
  sage: { wall: '#becbb7', upholstery: '#879b80', rug: '#667b67' },
  midnight: { wall: '#636681', upholstery: '#4a526b', rug: '#3a405c' },
} as const;

export function configureAuthoredMaterials(group: THREE.Group, room: PersonalRoom) {
  if (room.layoutId === 'pat-studio-v01' && !group.userData.computerChairAligned) {
    // Swivel the complete authored office chair toward the keyboard. Geometry and materials stay shared.
    const pivot = new THREE.Vector3(.2665, 0, -.3615);
    const swivel = new THREE.Matrix4().makeTranslation(pivot.x, 0, pivot.z)
      .multiply(new THREE.Matrix4().makeRotationY(-2.0556))
      .multiply(new THREE.Matrix4().makeTranslation(-pivot.x, 0, -pivot.z));
    group.traverse(node => {
      if (typeof node.userData.source_object === 'string' && node.userData.source_object.startsWith('Aeron Classic |')) node.applyMatrix4(swivel);
    });
    group.userData.computerChairAligned = true;
  }
  const visited = new Set<THREE.Material>();
  group.traverse(node => {
    if (!(node instanceof THREE.Mesh)) return;
    // Loose rug fibres remain visible; weight rests on the authored backing.
    // A single yarn cap must not lift a shoe above the surrounding rug.
    if (room.layoutId === 'pat-studio-v01' && (node.userData.source_object === 'Pale rug | short loose surface fibers'
      || /^Studio_portable_rug_groom_[1-5]$/.test(node.name))) node.userData.groundSupport = false;
    node.castShadow = false; node.receiveShadow = false;
    for (const material of Array.isArray(node.material) ? node.material : [node.material]) {
      if (!(material instanceof THREE.MeshStandardMaterial) || visited.has(material)) continue;
      visited.add(material);
      if (room.environmentId === 'terrace') {
        const slot = material.userData.paletteSlot as keyof typeof terracePalettes.rose | undefined;
        if (slot && Object.hasOwn(terracePalettes.rose, slot) && room.palette !== 'rose' && !customRoomMaterialColor(material, room)) {
          const target = new THREE.Color(terracePalettes[room.palette][slot]);
          const base = new THREE.Color(terracePalettes.rose[slot]);
          // Keep each paver's authored variation while changing the saved palette.
          material.color.add(target.sub(base));
        }
        const levels = material.userData.baddie_practical_emission_levels;
        if (Array.isArray(levels) && levels.length === 3 && levels.every(value => typeof value === 'number' && Number.isFinite(value) && value >= 0)) {
          material.emissiveIntensity = levels[room.light === 'evening' ? 2 : room.light === 'golden' ? 1 : 0];
        }
        continue;
      }
      const slot = material.userData.baddie_palette_slot as keyof typeof paletteSlots.rose | undefined;
      if (slot && Object.hasOwn(paletteSlots.rose, slot) && room.palette !== 'rose' && !customRoomMaterialColor(material, room)) {
        const target = new THREE.Color(paletteSlots[room.palette][slot]);
        const recorded = material.userData.baddie_palette_base_linear;
        const base = Array.isArray(recorded) && recorded.length === 3 && recorded.every(v => typeof v === 'number' && Number.isFinite(v) && v > 0)
          ? new THREE.Color().setRGB(recorded[0], recorded[1], recorded[2]) : new THREE.Color(paletteSlots.rose[slot]);
        // Instance-owned color factors retain the baked material's local detail.
        material.color.multiply(new THREE.Color().setRGB(target.r / base.r, target.g / base.g, target.b / base.b));
      }
      const emission = material.userData.baddie_practical_emission;
      if (typeof emission === 'number' && Number.isFinite(emission)) material.emissiveIntensity = emission * (room.light === 'evening' ? 1.3 : 1);
    }
  });
  applyCustomRoomColors(group, room);
}

/** Source camera stays inside the enclosure; legacy exterior orbits do not fit. */
export function authoredEnvironment(room: PersonalRoom): CompanionEnvironment | undefined {
  if (!room.layoutId || room.environmentId !== 'studio') return undefined;
  const layout = roomLayoutFor(room);
  if (room.layoutId === 'modern-kitchen-v01' || room.layoutId === 'bathroom-v01') return {
    id: layout.id, name: room.layoutId === 'modern-kitchen-v01' ? 'Kitchen' : 'Bathroom', background: '#e8e3d9', layout,
    characterScale: .38, spawnYaw: Math.PI, fov: 78, cameraOrbitLimit: .8,
    wide: { eye: [.9, 2.3, 2.5], target: [0, .65, .45] },
    portraitWide: room.layoutId === 'modern-kitchen-v01'
      ? { eye: [0, 2.1, 2.42], target: [0, .7, .95] }
      : { eye: [.6, 2.4, 2.42], target: [-.5, .55, .8] },
    close: { eye: [1.7, 1.65, 2.3], target: [0, .7, 1.2] },
    spawn: [0, 0, 1.6], walkingPoints: [], navigation: roomNavigationFor(room),
    authored: { key: room.layoutId === 'modern-kitchen-v01' ? 'bestieKitchen' : 'bestieBathroom' },
    create: () => new THREE.Group(),
  };
  if (room.layoutId === 'pink-bedroom-v02') return {
    id: layout.id, name: 'Pink bedroom', background: '#e6e3dc', layout,
    characterScale: .38, spawnYaw: Math.PI, fov: 64, cameraOrbitLimit: .12,
    wide: { eye: [-1.92, 1.86, -2.22], target: [-.15, .95, .45] },
    portraitWide: { eye: [-1.92, 1.86, -2.22], target: [0, .45, -.75] },
    close: { eye: [-.7, 1.6, -2.15], target: [0, .5, -.75] },
    spawn: [0, 0, -.75], walkingPoints: [], navigation: roomNavigationFor(room),
    authored: { key: 'bestiePinkBedroomV2' },
    create: () => new THREE.Group(),
  };
  const bedroom = room.layoutId === 'pink-bedroom-v01';
  return {
    id: layout.id, name: bedroom ? 'Pink bedroom' : 'Studio', background: '#d9c6ba', layout,
    // The desk's walking footprint ends below the displays and softbox.
    // Camera-only measured housings leave the floor routes unchanged.
    cameraObstacles: bedroom ? layout.obstacles : [...layout.obstacles,
      { id: 'display-left', x: -.43322, z: -.932245, y: .85334, width: .35654, depth: .56155, height: .37997 },
      { id: 'display-center', x: .604985, z: -1.00739, y: .8147, width: .50393, depth: .44704, height: .36996 },
      { id: 'display-right', x: 1.31612, z: -.67432, y: .81906, width: .44654, depth: .54316, height: .37359 },
      { id: 'softbox', x: .61781, z: -1.060515, y: .63856, width: 1.13406, depth: 1.27697, height: 1.58299 },
    ],
    characterScale: .38, spawnYaw: Math.PI, fov: 64, cameraOrbitLimit: .12,
    wide: bedroom ? { eye: [-.8, 1.65, -2.02], target: [0, .8, .25] }
      : { eye: [-.839, 1.588, -1.249], target: [-.427, 1.284, 1.707] },
    portraitWide: bedroom ? undefined
      : { eye: [-.8, 1.8, -1.6], target: [-.8, .45, 1.2] },
    close: bedroom ? { eye: [-.75, 1.35, -1.9], target: [-.4, .75, -1.1] }
      : { eye: [-.8, 1.45, -.65], target: [-.8, .5, 1.2] },
    spawn: bedroom ? [-.4, 0, -1.1] : [-.8, 0, 1.2], walkingPoints: [],
    navigation: roomNavigationFor(room),
    authored: { key: bedroom ? 'bestiePinkBedroom' : 'bestiePatStudio' },
    create: () => new THREE.Group(),
  };
}

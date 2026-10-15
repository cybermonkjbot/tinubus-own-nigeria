import type { CompanionPlaceLayout } from "./companionPlaces";
import type { CompanionSoundZone } from "./companionProximityAudio";

export const COMPANION_PLACE_SOUND_ZONE_LIMIT = 16;
export type CompanionPlaceSoundBoundary = {
  /** "place" uses resolved navigation bounds; a zone ID uses its stored rectangle. */
  zoneId: string; boundaryGain: number;
};

/** Optional playback geometry only. No defaults imply walls, private rooms,
 * membership or microphone access. The audio owner supplies approved gains. */
export function companionPlaceSoundZones(layout?: CompanionPlaceLayout | null,
  selected: readonly CompanionPlaceSoundBoundary[] = []): readonly CompanionSoundZone[] {
  if (!layout || selected.length === 0) return Object.freeze([]);
  if (selected.length > COMPANION_PLACE_SOUND_ZONE_LIMIT) throw new Error("Choose at most 16 venue sound boundaries.");
  const ids = new Set<string>();
  const zones = selected.map(selection => {
    if (!selection.zoneId || ids.has(selection.zoneId) || !Number.isFinite(selection.boundaryGain) || selection.boundaryGain < 0 || selection.boundaryGain > 1) {
      throw new Error("Sound boundaries need unique resolved IDs and gains between zero and one.");
    }
    ids.add(selection.zoneId);
    const bounds = selection.zoneId === "place" ? layout.navigation.bounds : layout.zones.find(zone => zone.id === selection.zoneId)?.bounds;
    if (!bounds || ![bounds.minX, bounds.maxX, bounds.minZ, bounds.maxZ].every(Number.isFinite) || bounds.minX >= bounds.maxX || bounds.minZ >= bounds.maxZ) {
      throw new Error(`No resolved sound rectangle for ${selection.zoneId}.`);
    }
    const halfWidth = (bounds.maxX - bounds.minX) / 2, halfDepth = (bounds.maxZ - bounds.minZ) / 2;
    const x = bounds.minX + halfWidth, z = bounds.minZ + halfDepth;
    if (![halfWidth, halfDepth, x, z].every(Number.isFinite)) throw new Error(`Sound rectangle exceeds finite metres for ${selection.zoneId}.`);
    return Object.freeze({ id: `${layout.placeId}:${selection.zoneId}`,
      center: Object.freeze({ x, z }), halfWidth, halfDepth, yaw: 0,
      boundaryGain: selection.boundaryGain });
  });
  return Object.freeze(zones);
}
export type LagosAmbientSoundKind = "generator-hum" | "traffic-horn" | "street-call" | "suya-sizzle";

export type CompanionLagosAmbientEmitter = {
  id: string;
  kind: LagosAmbientSoundKind;
  x: number;
  z: number;
  baseGain: number;
  maxDistanceMetres: number;
};

export type CompanionLagosGridPowerState = {
  gridPowerActive: boolean;
  generatorRunning: boolean;
  /** Normalized generator RPM/noise level 0..1 */
  generatorOutput: number;
  lastGridTransitionMs: number;
};

/** Calculate effective emitter gain based on 3D listener distance and power status */
export function sampleLagosAmbientGain(
  emitter: CompanionLagosAmbientEmitter,
  listener: { x: number; z: number },
  powerState?: CompanionLagosGridPowerState
): number {
  if (emitter.kind === "generator-hum" && powerState && !powerState.generatorRunning) {
    return 0;
  }
  const distance = Math.hypot(emitter.x - listener.x, emitter.z - listener.z);
  if (distance >= emitter.maxDistanceMetres) return 0;
  const falloff = 1 - distance / emitter.maxDistanceMetres;
  const powerFactor = emitter.kind === "generator-hum" && powerState ? powerState.generatorOutput : 1;
  return Math.max(0, Math.min(1, emitter.baseGain * falloff * falloff * powerFactor));
}

/** Default street ambient emitters for Lagos Town Square */
export const townSquareAmbientEmitters: readonly CompanionLagosAmbientEmitter[] = Object.freeze([
  // Diesel generator behind West compound
  { id: "west-generator", kind: "generator-hum", x: -22, z: 28, baseGain: 0.65, maxDistanceMetres: 24 },
  // Generator behind East shops
  { id: "east-generator", kind: "generator-hum", x: 24, z: -10, baseGain: 0.60, maxDistanceMetres: 22 },
  // Suya kitchen sizzle and charcoal smoke
  { id: "suya-stall-audio", kind: "suya-sizzle", x: 21, z: -4, baseGain: 0.75, maxDistanceMetres: 12 },
  // Danfo bus stop horn chokepoint
  { id: "bus-stop-horns", kind: "traffic-horn", x: -11, z: 10, baseGain: 0.85, maxDistanceMetres: 35 },
]);

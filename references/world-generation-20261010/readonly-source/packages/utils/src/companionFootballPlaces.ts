import type { CompanionPlaceLayout } from "./companionPlaces";
import type { FootballPitch } from "./companionFootball";
import { validFootballPitch } from "./companionFootballEngine";

/** Resolve from the venue's goal fixtures, not a competing football layout. */
export function footballPitchFromPlace(layout: CompanionPlaceLayout, instanceId: string, revision: number): FootballPitch {
  if (layout.kind !== "soccer-pitch") throw new Error("This place is not a football pitch.");
  const zone = layout.zones.find(z => z.activity === "football");
  const goals = layout.fixtures.filter(f => f.kind === "goal").sort((a, b) => a.point[2] - b.point[2]);
  if (!zone || goals.length !== 2 || goals.some(g => Math.abs(Math.sin(g.yaw)) > .001)) throw new Error("The pitch needs two aligned goals.");
  const [north, south] = goals;
  if (Math.abs(north.point[0] - south.point[0]) > .05) throw new Error("Goal centres must agree.");
  const pitch: FootballPitch = { id: layout.id, instanceId, revision, floorY: layout.navigation.floorY,
    centre: { x: north.point[0], z: (north.point[2] + south.point[2]) / 2 }, halfLength: (south.point[2] - north.point[2]) / 2,
    halfWidth: Math.min(north.point[0] - zone.bounds.minX, zone.bounds.maxX - north.point[0]),
    goalHalfWidth: Math.min(north.size[0], south.size[0]) / 2 - .065, goalHeight: Math.min(north.size[1], south.size[1]) - .065 };
  if (!validFootballPitch(pitch)) throw new Error("The venue's pitch coordinates are invalid.");
  return pitch;
}

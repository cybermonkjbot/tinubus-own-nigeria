import type { PersonalRoom } from './companion';
import { companionHomePoint } from './companionHomeDesign';
import { roomNavigationFor, navigableStandingPoints, planRoomRoute, type ScenePoint } from './companionNavigation';

/** Door coordinates belong to the delivered room, not its first spawn. */
export function companionHomeDoor(room: PersonalRoom) {
  const navigation = roomNavigationFor(room);
  const standing = navigableStandingPoints(room);
  const spawn = standing[0];
  if (!spawn) throw new Error('Your Home has no clear standing space.');
  const start: ScenePoint = [spawn.x, navigation.floorY, spawn.z];
  const bedroom = room.layoutId === 'pink-bedroom-v02';
  const studio = room.layoutId === 'pat-studio-v01' || room.layoutId === 'pink-bedroom-v01';
  if (!bedroom && !studio) return { point: start, doorway: start, authored: false };
  const doorway = companionHomePoint(room, bedroom ? [-1.74, navigation.floorY, -2.6] : [-1.6891, navigation.floorY, .6931]);
  const preferred = companionHomePoint(room, bedroom ? [-1.74, navigation.floorY, -2.25] : [-1.34, navigation.floorY, .6931]);
  const candidates: ScenePoint[] = [preferred, ...(bedroom ? [companionHomePoint(room, [-1.15, navigation.floorY, -1.7])] : []),
    ...standing.map(p => [p.x, navigation.floorY, p.z] as const)]
    .filter(point => Math.hypot(point[0] - doorway[0], point[2] - doorway[2]) <= 1.25)
    .sort((a, b) => Math.hypot(a[0] - doorway[0], a[2] - doorway[2]) - Math.hypot(b[0] - doorway[0], b[2] - doorway[2]));
  const point = candidates.find(point => planRoomRoute(navigation, start, point));
  if (!point) throw new Error('Clear the space in front of your Home door.');
  return { point, doorway, authored: true };
}

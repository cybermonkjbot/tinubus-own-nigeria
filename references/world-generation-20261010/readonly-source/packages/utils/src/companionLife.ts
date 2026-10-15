import type { PersonalRoom } from "./companion";
import { companionHomePoint, companionHomeYaw } from "./companionHomeDesign";
import { validCompanionTaskQueue, type CompanionTaskQueue, type CompanionTaskCommand } from "./companionTasks";
import { newCompanionSkills, validCompanionSkills, type CompanionSkills } from "./companionSkills";
import { isCompanionActivityKind, type CompanionActivityKind } from "./companionActivityKinds";

export type { CompanionActivityKind } from "./companionActivityKinds";
export const COMPANION_WORK_EXPERIENCE = 8;
export type CompanionNeeds = { hunger: number; energy: number; cleanliness: number; enjoyment: number };
export const companionNeedDriftPerHour: Readonly<CompanionNeeds> = Object.freeze({ hunger: 4, energy: -3, cleanliness: -2, enjoyment: -3 });
export type CompanionActivity = {
  id: string; kind: CompanionActivityKind; roomId: string; stationId: string;
  startedAt: number; endsAt: number;
  /** Absent on legacy activities, which already had an active server clock. */
  phase?: "approaching" | "active";
  requestedAt?: number;
  roomKey?: string;
};
export type CompanionCareReceipt = { activityId: string; status: "arrived" | "blocked" | "interrupted" };
export const COMPANION_CARE_APPROACH_MS = 60_000;
export const COMPANION_CARE_ENTRY_MS = 900;
export const companionCareRoomKey = (room: PersonalRoom) => JSON.stringify([room.id, room.environmentId, room.layoutId, room.items,
  ...(room.homeDesign ? [room.homeDesign] : [])]);
export type CompanionLife = {
  characterId: string; needs: CompanionNeeds; updatedAt: number; revision: number;
  activity: CompanionActivity | null; meals: number; reminders: boolean;
  taskQueue?: CompanionTaskQueue;
  skills?: CompanionSkills;
};
export type CompanionLifeSnapshot = { life: CompanionLife | null; busy: boolean; error: string | null; retryable?: boolean };
export interface CompanionLifeStore {
  getSnapshot(): CompanionLifeSnapshot;
  subscribe(listener: () => void): () => void;
  select(characterId: string): void;
  act(roomId: string, stationId: string, action: "start" | "cancel" | "reminders-on" | "reminders-off", cooking?: { recipeKey: string; batches: 1 | 2 }): Promise<void>;
  report?(receipt: CompanionCareReceipt): Promise<void>;
  retry?(): Promise<void>;
  task?(command: CompanionTaskCommand): Promise<void>;
}
export type CompanionStation = {
  id: string; label: string; kind: CompanionActivityKind; seconds: number;
  /** Approach on clear floor, then the authored interaction pose. Runtime metres. */
  approach: readonly [number, number, number]; position: readonly [number, number, number]; yaw: number;
  workSurface?: readonly [number, number, number];
  /** Keycap height change per metre along room x/z, for tilted keyboards. */
  workSurfaceSlope?: readonly [number, number];
  /** Head centre on the usable bedding, in room x/z, excluding upright cushions. */
  sleepHead?: readonly [number, number];
  /** Furniture shared by several actions, without adding collision volumes. */
  targetIds?: readonly string[];
  screen?: { position: readonly [number, number, number]; yaw: number; tilt?: number; width: number; height: number };
};
export const companionStationTargets = (station: CompanionStation): readonly string[] =>
  station.targetIds ?? (station.id === "computer-work" ? ["workstation", "desk-chair"] : [station.id]);
const HOUR = 3_600_000;
const clamp = (n: number) => Math.min(100, Math.max(0, n));
export function validCompanionLife(value: unknown): value is CompanionLife {
  const record = (value: unknown): value is Record<string, unknown> => !!value && typeof value === "object" && !Array.isArray(value);
  const text = (value: unknown) => typeof value === "string" && value.length > 0 && value.length <= 100;
  const finite = (value: unknown): value is number => typeof value === "number" && Number.isFinite(value);
  if (!record(value) || !text(value.characterId) || !record(value.needs) ||
    !finite(value.updatedAt) || value.updatedAt < 0 || !finite(value.revision) || !Number.isSafeInteger(value.revision) || value.revision < 0 ||
    !finite(value.meals) || !Number.isInteger(value.meals) || value.meals < 0 || value.meals > 3 || typeof value.reminders !== "boolean"
    || value.taskQueue !== undefined && !validCompanionTaskQueue(value.taskQueue)
    || value.skills !== undefined && !validCompanionSkills(value.skills)) return false;
  for (const key of ["hunger", "energy", "cleanliness", "enjoyment"]) {
    const need = value.needs[key]; if (!finite(need) || need < 0 || need > 100) return false;
  }
  const activity = value.activity;
  return activity === null || record(activity) && text(activity.id) && text(activity.roomId) && text(activity.stationId)
    && isCompanionActivityKind(activity.kind)
    && finite(activity.startedAt) && finite(activity.endsAt) && activity.startedAt >= 0
    && (activity.phase === undefined || activity.phase === "approaching" || activity.phase === "active")
    && (activity.requestedAt === undefined || finite(activity.requestedAt) && activity.requestedAt >= 0 && activity.requestedAt <= activity.startedAt)
    && (activity.roomKey === undefined || typeof activity.roomKey === "string" && activity.roomKey.length <= 5000)
    && activity.endsAt > activity.startedAt && activity.endsAt - activity.startedAt <= 120_000;
}
export const companionNeedLabels: Record<keyof CompanionNeeds, string> = {
  hunger: "Hunger", energy: "Energy", cleanliness: "Cleanliness", enjoyment: "Enjoyment",
};
export function newCompanionLife(characterId: string, now: number): CompanionLife {
  return { characterId, needs: { hunger: 20, energy: 85, cleanliness: 90, enjoyment: 80 },
    updatedAt: now, revision: 0, activity: null, meals: 0, reminders: false, skills: newCompanionSkills() };
}
const gains: Record<CompanionActivityKind, Partial<CompanionNeeds>> = {
  sleep: { energy: 75 }, sit: { energy: 12, enjoyment: 6 }, play: { enjoyment: 40, energy: -4 },
  cook: { enjoyment: 12, energy: -3 }, eat: { hunger: -60 }, bathe: { cleanliness: 70, enjoyment: 8 },
  work: { enjoyment: 4, energy: -6 },
  "watch-tv": { enjoyment: 30, energy: 5 }, read: { enjoyment: 22, energy: 8 },
  stretch: { enjoyment: 14, energy: -3 }, wash: { cleanliness: 20, energy: -2 },
};
/** Deterministic projection. Callers supply time; only the server commits rewards. */
export function projectCompanionLife(saved: CompanionLife, now: number): CompanionLife {
  if (!Number.isFinite(now) || now <= saved.updatedAt) return saved;
  const hours = Math.min(72, (now - saved.updatedAt) / HOUR);
  const needs = { ...saved.needs };
  const drift = companionNeedDriftPerHour;
  const activity = saved.activity;
  let meals = saved.meals;
  let activityHours = 0;
  if (activity && activity.phase !== "approaching") {
    const duration = activity.endsAt - activity.startedAt;
    const overlap = Math.max(0, Math.min(now, activity.endsAt) - Math.max(saved.updatedAt, activity.startedAt));
    const part = duration > 0 ? Math.min(1, overlap / duration) : 0;
    activityHours = Math.min(hours, Math.max(0, Math.min(now, activity.endsAt) - saved.updatedAt) / HOUR);
    for (const key of Object.keys(needs) as (keyof CompanionNeeds)[]) {
      needs[key] = clamp(needs[key] + drift[key] * activityHours + (gains[activity.kind][key] ?? 0) * part);
    }
    // Food completion belongs to inventory receipts. Preserve legacy meal counts without converting them.
  }
  // Apply time after completion last: a bath taken days ago cannot restore
  // cleanliness at reconnect time merely because its callback was delayed.
  for (const key of Object.keys(needs) as (keyof CompanionNeeds)[]) needs[key] = clamp(needs[key] + drift[key] * (hours - activityHours));
  return { ...saved, needs, meals, updatedAt: now, activity: activity && now < activity.endsAt ? activity : null };
}
export function companionStations(room: Pick<PersonalRoom, "id" | "layoutId" | "environmentId" | "homeDesign">): readonly CompanionStation[] {
  const stations = authoredCompanionStations(room);
  if (!room.homeDesign) return stations;
  return stations.map(station => {
    const transformed: CompanionStation = { ...station, approach: companionHomePoint(room, station.approach),
      position: companionHomePoint(room, station.position), yaw: station.yaw + companionHomeYaw(room) };
    if (station.workSurface) transformed.workSurface = companionHomePoint(room, station.workSurface);
    if (station.screen) transformed.screen = { ...station.screen, position: companionHomePoint(room, station.screen.position),
      yaw: station.screen.yaw + companionHomeYaw(room) };
    if (station.workSurfaceSlope) {
      const [x, , z] = companionHomePoint(room, [station.workSurfaceSlope[0], 0, station.workSurfaceSlope[1]]);
      transformed.workSurfaceSlope = [x, z];
    }
    if (station.sleepHead) {
      const [x, , z] = companionHomePoint(room, [station.sleepHead[0], 0, station.sleepHead[1]]);
      transformed.sleepHead = [x, z];
    }
    return transformed;
  });
}

function authoredCompanionStations(room: Pick<PersonalRoom, "id" | "layoutId" | "environmentId">): readonly CompanionStation[] {
  const station = (id: string, label: string, kind: CompanionActivityKind, seconds: number,
    approach: CompanionStation["approach"], position: CompanionStation["position"], yaw = 0): CompanionStation =>
    ({ id, label, kind, seconds, approach, position, yaw });
  if (room.layoutId === "pink-bedroom-v02") return [
    // V15 duvet top is .63-.65m; the centre cushion starts at z=1.653m.
    { ...station("bed", "Sleep", "sleep", 120, [-.7, 0, -.25], [-.7, .645, .45]), sleepHead: [-.7, 1.36] },
    station("window-chair", "Sit", "sit", 25, [-1.1, 0, -.75], [-1.71, .45, -.75], Math.PI / 2),
    { ...station("bedroom-tv", "Watch TV", "watch-tv", 60, [-.7, 0, -.35], [-.7, .645, .15], Math.PI),
      targetIds: ["media-display", "media-console"], screen: { position: [-.5, 1.6, -2.496], yaw: 0, width: 1.16, height: .64 } },
    { ...station("window-reading", "Read by the window", "read", 45, [-1.1, 0, -.75], [-1.71, .45, -.75], Math.PI / 2), targetIds: ["window-chair"] },
    station("bedroom-stretch", "Stretch", "stretch", 30, [.65, 0, -.6], [.65, 0, -.6], Math.PI),
  ];
  if (room.layoutId === "pink-bedroom-v01") return [station("bed", "Sleep", "sleep", 120, [-.4, 0, -.5], [-.165, .64, .3])];
  if (room.layoutId === "modern-kitchen-v01") return [
    station("island", "Cook", "cook", 35, [0, 0, 1.1], [0, 0, 1.1], Math.PI),
    station("island-seat", "Eat", "eat", 20, [1.4, 0, .9], [1.4, .65, .15], -Math.PI / 2),
    { ...station("kitchen-washing", "Wash dishes", "wash", 30, [-1.28, 0, -1.45], [-1.28, 0, -1.48], Math.PI),
      targetIds: ["counter"], workSurface: [-1.28, 1.04, -2.05] },
  ];
  if (room.layoutId === "bathroom-v01") return [
    station("bath", "Take a bath", "bathe", 50, [0, 0, .8], [-1.05, .26, .15]),
    { ...station("vanity-washing", "Wash hands", "wash", 15, [.65, 0, -1.45], [.65, 0, -1.48], Math.PI),
      targetIds: ["vanity"], workSurface: [.65, .97, -2.05] },
  ];
  if (room.layoutId === "pat-studio-v01") return [
    // The INZOY cushion faces diagonally into the room. Place the pelvis
    // .19m ahead of its center so v27's knees clear the upholstered front.
    // The .418m cushion top and existing walk-up point remain separate.
    station("chair", "Sit", "sit", 25, [.1, 0, 1.15], [.8377, .418, 1.4775], Math.PI + .888006),
    station("workstation", "Play", "play", 35, [-.75, 0, .25], [-.75, 0, .25], Math.PI / 2),
    // Aeron seat top, with the chair swivel aligned to the keyboard.
    // Measured keycap plane. The keyboard slopes down toward the chair.
    { ...station("computer-work", "Work at computer", "work", 45, [-.75, 0, .25], [.215, .371, -.49], Math.PI + .42),
      workSurface: [.06, .7242, -.927], workSurfaceSlope: [-.004, -.059] },
    { ...station("studio-tv", "Watch TV at the computer", "watch-tv", 60, [-.75, 0, .25], [.215, .371, -.49], 2.498),
      targetIds: ["workstation", "desk-chair"], screen: { position: [.581, 1.003, -.979], yaw: -.707, tilt: .054, width: .592, height: .331 } },
    { ...station("chair-reading", "Read a book", "read", 45, [.1, 0, 1.15], [.8377, .418, 1.4775], Math.PI + .888006), targetIds: ["chair"] },
  ];
  if (room.environmentId === "terrace") return [station("terrace-stretch", "Stretch outdoors", "stretch", 30, [0, 0, 1.05], [0, 0, 1.05])];
  // Legacy layouts have no approved furniture interaction anchors yet.
  return [];
}

export type LagosStreetFoodItem = {
  id: string;
  label: string;
  hungerRestored: number;
  energyRestored: number;
  enjoymentRestored: number;
  carriedUnit: string;
};

export const lagosStreetFoodMenu: Record<string, LagosStreetFoodItem> = {
  "suya-beef-skewer": { id: "suya-beef-skewer", label: "Mai Suya Beef Skewer", hungerRestored: 35, energyRestored: 15, enjoymentRestored: 20, carriedUnit: "pack" },
  "akara-agege-bread": { id: "akara-agege-bread", label: "Hot Akara & Agege Bread", hungerRestored: 50, energyRestored: 25, enjoymentRestored: 15, carriedUnit: "wrap" },
  "gala-sausage": { id: "gala-sausage", label: "Gala Sausage Roll", hungerRestored: 20, energyRestored: 10, enjoymentRestored: 5, carriedUnit: "piece" },
  "lacasera-drink": { id: "lacasera-drink", label: "Chilled Lacasera Apple Drink", hungerRestored: 10, energyRestored: 20, enjoymentRestored: 10, carriedUnit: "bottle" },
  "plantain-chips": { id: "plantain-chips", label: "Crispy Plantain Chips (Ipekere)", hungerRestored: 15, energyRestored: 10, enjoymentRestored: 12, carriedUnit: "pack" },
  "chilled-pure-water": { id: "chilled-pure-water", label: "Ice Cold Sachet Water", hungerRestored: 5, energyRestored: 15, enjoymentRestored: 8, carriedUnit: "sachet" },
};

export function applyLagosStreetFood(needs: CompanionNeeds, foodKey: string): CompanionNeeds {
  const item = lagosStreetFoodMenu[foodKey];
  if (!item) return needs;
  return {
    hunger: clamp(needs.hunger - item.hungerRestored),
    energy: clamp(needs.energy + item.energyRestored),
    cleanliness: needs.cleanliness,
    enjoyment: clamp(needs.enjoyment + item.enjoymentRestored),
  };
}

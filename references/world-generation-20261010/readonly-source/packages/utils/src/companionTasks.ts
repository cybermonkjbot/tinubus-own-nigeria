import type { PersonalRoom } from "./companion";
import type { CompanionActivity, CompanionActivityKind, CompanionLife } from "./companionLife";
import { companionStations } from "./companionLife";
import { companionActivityLabels, isCompanionActivityKind } from "./companionActivityKinds";

export const COMPANION_TASK_LIMIT = 8;
export type CompanionTask = {
  id: string; roomId: string; stationId: string; kind: CompanionActivityKind;
  status: "waiting" | "active" | "blocked"; activityId: string | null; reason: string | null;
  cooking?: { recipeKey: string; batches: 1 | 2 };
};
export type CompanionTaskQueue = { tasks: CompanionTask[]; paused: boolean; reason: string | null };
export type CompanionTaskCommand =
  | { type: "enqueue"; roomId: string; stationId: string; cooking?: { recipeKey: string; batches: 1 | 2 } }
  | { type: "pause" | "resume" | "stop" }
  | { type: "remove" | "retry"; taskId: string }
  | { type: "move"; taskId: string; direction: "up" | "down" }
  | { type: "advance"; roomId: string };

export function companionTaskQueue(life: CompanionLife | null | undefined): CompanionTaskQueue {
  return life?.taskQueue ?? { tasks: [], paused: false, reason: null };
}
export function companionTaskLabel(task: CompanionTask, rooms: readonly PersonalRoom[]): string {
  const room = rooms.find(room => room.id === task.roomId);
  return room && companionStations(room).find(station => station.id === task.stationId && station.kind === task.kind)?.label
    || companionActivityLabels[task.kind];
}
export function validCompanionTaskQueue(value: unknown): value is CompanionTaskQueue {
  const record = (item: unknown): item is Record<string, unknown> => !!item && typeof item === "object" && !Array.isArray(item);
  const text = (item: unknown, max = 100): item is string => typeof item === "string" && item.length > 0 && item.length <= max;
  if (!record(value) || !Array.isArray(value.tasks) || value.tasks.length > COMPANION_TASK_LIMIT || typeof value.paused !== "boolean"
    || !(value.reason === null || text(value.reason, 240))) return false;
  const ids = new Set<string>();
  for (const [index, task] of value.tasks.entries()) {
    if (!record(task) || !text(task.id) || ids.has(task.id) || !text(task.roomId, 80) || !text(task.stationId, 80)
      || !isCompanionActivityKind(task.kind)
      || !["waiting", "active", "blocked"].includes(String(task.status))
      || !(task.reason === null || text(task.reason, 240))
      || !(task.activityId === null || text(task.activityId))
      || task.status === "active" && (index !== 0 || !task.activityId)
      || task.status !== "active" && task.activityId !== null) return false;
    if (task.cooking !== undefined && (task.kind !== "cook" || !record(task.cooking)
      || !text(task.cooking.recipeKey, 80) || task.cooking.batches !== 1 && task.cooking.batches !== 2)) return false;
    ids.add(task.id);
  }
  return true;
}

/** Only authoritative care transitions call this; projection never settles queued tasks. */
export function settleCompanionTask(life: CompanionLife, activity: CompanionActivity,
  outcome: "completed" | "blocked" | "interrupted", reason: string | null = null): CompanionLife {
  const queue = companionTaskQueue(life), first = queue.tasks[0];
  if (first?.status !== "active" || first.activityId !== activity.id) return life;
  if (outcome === "completed") return { ...life, taskQueue: { ...queue, tasks: queue.tasks.slice(1) } };
  return { ...life, taskQueue: { tasks: [{ ...first, status: outcome === "blocked" ? "blocked" : "waiting", activityId: null, reason }, ...queue.tasks.slice(1)],
    paused: true, reason: reason ?? "Task interrupted. Resume when ready." } };
}

import { v } from "convex/values";

export const worldPoint = v.object({ x: v.number(), z: v.number(), yaw: v.number() });
export const worldMovement = v.object({
  version: v.literal(1),
  sequence: v.number(),
  roomRevision: v.number(),
  startedAt: v.number(),
  from: worldPoint,
  initialSpeed: v.optional(v.number()),
  pace: v.optional(v.union(v.literal("walk"), v.literal("run"), v.literal("fast"))),
  path: v.array(v.object({ x: v.number(), z: v.number() })),
  yaw: v.number(),
});
export const worldGeoPoint = v.object({ latitude: v.number(), longitude: v.number() });

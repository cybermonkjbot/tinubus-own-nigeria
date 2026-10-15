import * as THREE from "three";
import type { CompanionRoadReaction } from "../../../../packages/utils/src/companionRoadMechanics";

export type TrafficDecision = { speed: number; lateral: number; lateralAcceleration: number; overtaking: boolean };
type Vehicle = { id: string; x: number; z: number; yaw: number; speed: number; halfWidth: number; halfLength: number };
export type TrafficFrame = {
  vehicle: Vehicle;
  targetSpeed: number;
  cruiseSpeed: number;
  lead?: Vehicle;
  leadGap: number;
  canPass: boolean;
  lateralGrip: number;
  /** Swept full-chassis clearance, including opposing vehicles and pedestrians. */
  clear: (lateral: number, forward: number) => boolean;
  horn: () => void;
  /** Accepted finite road reaction. The caller retains pose/crash/lane ownership. */
  road?: { now: number; reaction: CompanionRoadReaction | null; externalHold: boolean;
    owner: "ambient" | "player" | "responder" };
};
export type CrossingVehicle = Vehicle & { radius: number };

/** Local spline decisions only. Shared movement receipts retain their existing owner. */
export class TrafficAI {
  private frame?: TrafficFrame;
  private disposed = false;
  private passId = "";
  private passTime = 0;
  private cooldown = 0;
  private desiredLateral = 0;
  private lateralVelocity = 0;
  readonly decision: TrafficDecision = { speed: 0, lateral: 0, lateralAcceleration: 0, overtaking: false };
  init() { if (!this.disposed) this.reset(); }
  setFrame(frame: TrafficFrame) { if (!this.disposed) this.frame = frame; }
  update(delta: number) {
    const frame = this.frame;
    if (this.disposed || !frame || !Number.isFinite(delta) || delta <= 0 || delta > .25) return this.decision;
    const dt = Math.min(delta, .1), { vehicle, lead } = frame;
    if (frame.road && (frame.road.externalHold || frame.road.owner !== "ambient")) {
      this.decision.speed = 0; return this.decision;
    }
    const reaction = frame.road?.reaction;
    const yielding = !!reaction && Number.isSafeInteger(frame.road?.now) && reaction.vehicleId === vehicle.id
      && frame.road!.now >= reaction.startsAt && frame.road!.now < reaction.endsAt
      && reaction.endsAt - reaction.startsAt <= 2500 && reaction.endsAt > reaction.startsAt
      && [reaction.lateral, reaction.speedLimit].every(Number.isFinite)
      && Math.abs(reaction.lateral) <= 4 && reaction.speedLimit >= 0 && reaction.speedLimit <= 35;
    this.cooldown = Math.max(0, this.cooldown - dt);
    let speed = frame.targetSpeed;
    if (lead && frame.leadGap > 1.8 && speed < 5 / 3.6 && frame.canPass && frame.clear(this.decision.lateral, 1.8))
      speed = Math.min(5 / 3.6, frame.cruiseSpeed);
    if (!yielding && !this.passId && this.cooldown === 0 && frame.canPass && lead && frame.leadGap > 1.8 && frame.leadGap < 15
      && lead.speed < frame.cruiseSpeed * .3) {
      const offset = vehicle.halfWidth + lead.halfWidth + .65;
      for (const side of [-1, 1]) if (frame.clear(offset * side, 8)) {
        this.passId = lead.id; this.passTime = 0; this.desiredLateral = offset * side;
        frame.horn(); break;
      }
    }
    if (yielding) {
      // One reaction owns this lane request. Existing route checks guard both outward and return sweeps.
      this.passId = ""; this.passTime = 0; this.cooldown = Math.max(this.cooldown, 3.5);
      this.desiredLateral = reaction.kind === "horn" ? reaction.lateral : this.decision.lateral;
      speed = Math.min(speed, reaction.speedLimit);
      if (!frame.clear(this.desiredLateral, Math.max(.3, vehicle.speed * dt))) speed = 0;
    } else if (this.passId) {
      this.passTime += dt;
      const passed = !lead || lead.id !== this.passId || frame.leadGap < -2 * (vehicle.halfLength + lead.halfLength) - .8;
      if (passed || this.passTime > 8 || !frame.canPass || !frame.clear(this.desiredLateral, 8)) this.desiredLateral = 0;
      if (this.desiredLateral !== 0) speed = frame.cruiseSpeed;
      if (this.desiredLateral === 0 && Math.abs(this.decision.lateral) < .02) {
        this.passId = ""; this.cooldown = 8; this.passTime = 0;
      }
    } else if (frame.road) {
      this.desiredLateral = 0;
    }
    const next = THREE.MathUtils.damp(this.decision.lateral, this.desiredLateral, 3.5 * Math.min(1, frame.lateralGrip / .85), dt);
    // Losing a merge gap cannot move a vehicle through the adjacent chassis.
    const previousLateral = this.decision.lateral;
    if (frame.clear(next, Math.max(.3, vehicle.speed * dt))) this.decision.lateral = next;
    else speed = 0;
    const lateralVelocity = (this.decision.lateral - previousLateral) / dt;
    this.decision.lateralAcceleration = THREE.MathUtils.clamp((lateralVelocity - this.lateralVelocity) / dt, -5, 5);
    this.lateralVelocity = lateralVelocity;
    this.decision.speed = Math.max(0, speed); this.decision.overtaking = !!this.passId;
    return this.decision;
  }
  static canSprintAcross(start: THREE.Vector3, end: THREE.Vector3, median: THREE.Vector3,
    vehicles: readonly CrossingVehicle[], extraClear: () => boolean = () => true) {
    if (![...start.toArray(), ...end.toArray(), ...median.toArray()].every(Number.isFinite)
      || vehicles.length > 16 || start.distanceTo(median) > 5 || start.distanceTo(end) / 4.5 > 4 || !extraClear()) return false;
    const dx = end.x - start.x, dz = end.z - start.z, length = dx * dx + dz * dz;
    for (const vehicle of vehicles) {
      if (![vehicle.x, vehicle.z, vehicle.yaw, vehicle.speed, vehicle.radius].every(Number.isFinite)) return false;
      // Swept vehicle occupancy over the complete four-second gap, both directions.
      for (let step = 0; step <= 16; step++) {
        const time = step / 4, x = vehicle.x + Math.sin(vehicle.yaw) * vehicle.speed * time;
        const z = vehicle.z + Math.cos(vehicle.yaw) * vehicle.speed * time;
        const t = length ? THREE.MathUtils.clamp(((x - start.x) * dx + (z - start.z) * dz) / length, 0, 1) : 0;
        if (Math.hypot(x - start.x - dx * t, z - start.z - dz * t) < vehicle.radius + .5 + vehicle.speed * .125) return false;
      }
    }
    return true;
  }
  reset(lateral = 0) { this.frame = undefined; this.passId = ""; this.passTime = this.cooldown = this.lateralVelocity = 0;
    this.desiredLateral = 0;
    Object.assign(this.decision, { speed: 0, lateral: Number.isFinite(lateral) ? lateral : 0, lateralAcceleration: 0, overtaking: false }); }
  dispose() { this.reset(); this.disposed = true; }
  get passingVehicleId() { return this.passId; }
}

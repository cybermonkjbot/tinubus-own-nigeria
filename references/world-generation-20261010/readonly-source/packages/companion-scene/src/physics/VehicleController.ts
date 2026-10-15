import * as THREE from "three";
import type { LagosSurface } from "../world/LagosScatter";
import { validCompanionRoadFrame, sameCompanionRoadScope,
  type CompanionRoadAcceptedFrame } from "../../../../packages/utils/src/companionRoadMechanics";
import type { CompanionRoadWheelImpact, CompanionVehicleStrainProjection } from "../../../../packages/utils/src/companionVehicleStrain";
import { castVehicleRoadPlane, createVehicleDynamics, createVehicleDynamicsConfig, holdVehicleDynamics, stepVehicleDynamics,
  vehicleDynamicsPose, vehicleSteeringLimit, vehicleSurfaceGrip,
  type CompanionVehicleDynamics, type VehicleDynamicsConfig, type VehicleGroundCast } from "../../../../packages/utils/src/companionVehicleDynamics";

export type VehicleMotion = { speed: number; targetSpeed: number; curvature: number; brake: number; acceleration: number; lateralAcceleration?: number };
export type VehicleRoadResponse = { speed: number; roll: number; pitch: number; lift: number; brakingPull: number; slip: number; lateralGrip: number };
type Options = {
  danfo: boolean;
  wheelbase: number;
  halfWidth: number;
  wheelRadius?: number;
  wheelLayout?: readonly { x: number; z: number; front: boolean; radius: number }[];
  groundCast?: VehicleGroundCast;
  /** Adapter returns the wheel raycast's collider tag, or the same sampled road patch. */
  ground: (wheel: THREE.Vector3) => LagosSurface;
  /** Stable collider/patch identity. Required for accepted persistent pothole consequences. */
  groundContact?: (wheel: THREE.Vector3) => string | null;
  /** Presentation cannot settle this receipt. The driving/world owner rechecks contact and commits it once. */
  acceptedImpact?: (impact: CompanionRoadWheelImpact) => void;
  impact?: (strength: number, speed: number) => void;
  shake?: (strength: number) => void;
};
/** Ambient driving adapter. The traffic owner still sweeps and accepts every proposed chassis pose. */
export class VehicleController {
  readonly centerOfMass: THREE.Vector3;
  readonly config: VehicleDynamicsConfig;
  dynamics: CompanionVehicleDynamics | null = null;
  private initialized = false;
  private disposed = false;
  private contact = false;
  private cooldown = 0;
  private fade = 0;
  private position = new THREE.Vector3();
  private wheel = new THREE.Vector3();
  private yaw = 0;
  private attitude = new THREE.Euler(0, 0, 0, "YXZ");
  private rotation = new THREE.Quaternion();
  private longitudinalGrip = .85;
  private motion: VehicleMotion = { speed: 0, targetSpeed: 0, curvature: 0, brake: 2.8, acceleration: .9 };
  private acceptedFrame?: CompanionRoadAcceptedFrame;
  private acceptedSequence = -1;
  private acceptedContact: string | null = null;
  private strain?: CompanionVehicleStrainProjection;
  readonly response: VehicleRoadResponse = { speed: 0, roll: 0, pitch: 0, lift: 0, brakingPull: 0, slip: 0, lateralGrip: .85 };
  constructor(private readonly options: Options) {
    this.config = createVehicleDynamicsConfig({ kind: options.danfo ? "danfo" : "car", wheelbase: options.wheelbase,
      halfWidth: options.halfWidth, wheelRadius: options.wheelRadius, wheelLayout: options.wheelLayout });
    this.centerOfMass = new THREE.Vector3(this.config.centerOfMass.x, this.config.centerOfMass.y, this.config.centerOfMass.z);
  }
  init() {
    if (this.disposed || this.initialized) return;
    this.initialized = true;
  }
  setMotion(point: THREE.Vector3, yaw: number, motion: VehicleMotion) {
    if (![point.x, point.y, point.z, yaw, ...Object.values(motion)].every(Number.isFinite)) return;
    this.position.copy(point); this.yaw = yaw; this.motion = motion;
    const current = this.dynamics && vehicleDynamicsPose(this.dynamics, this.config);
    if (!current || Math.hypot(current.x - point.x, current.z - point.z) > .001
      || Math.abs(Math.atan2(Math.sin(current.yaw - yaw), Math.cos(current.yaw - yaw))) > .001)
      this.dynamics = createVehicleDynamics(this.config, { x: point.x, y: point.y, z: point.z, yaw }, motion.speed);
  }
  /** Contact evidence for externally accepted motion. Does not run a second driving simulation. */
  setAcceptedFrame(frame: CompanionRoadAcceptedFrame, strain?: CompanionVehicleStrainProjection) {
    if (this.disposed || !validCompanionRoadFrame(frame) || this.acceptedFrame && sameCompanionRoadScope(this.acceptedFrame.scope, frame.scope)
      && (frame.sequence < this.acceptedFrame.sequence || frame.at < this.acceptedFrame.at)) return false;
    if (strain && (![strain.temperature, strain.brakeFade, strain.alignment, strain.handling.accelerationScale,
      strain.handling.brakingScale, strain.handling.speedLimit, strain.handling.steeringBias].every(Number.isFinite)
      || strain.temperature < 0 || strain.temperature > 100 || strain.brakeFade < 0 || strain.brakeFade > .18
      || Math.abs(strain.alignment) > .035 || strain.handling.accelerationScale < 0 || strain.handling.accelerationScale > 1
      || strain.handling.brakingScale < .5 || strain.handling.brakingScale > 1
      || strain.handling.speedLimit < 0 || strain.handling.speedLimit > 35 || Math.abs(strain.handling.steeringBias) > .12)) return false;
    if (this.acceptedFrame && !sameCompanionRoadScope(this.acceptedFrame.scope, frame.scope)) {
      this.acceptedSequence = -1; this.acceptedContact = null;
    }
    this.acceptedFrame = Object.freeze({ ...frame, scope: Object.freeze({ ...frame.scope }), pose: Object.freeze({ ...frame.pose }) });
    this.strain = strain;
    this.position.set(frame.pose.x, frame.pose.y, frame.pose.z); this.yaw = frame.pose.yaw;
    return true;
  }
  update(delta: number) {
    if (this.disposed || !Number.isFinite(delta) || delta <= 0 || delta > .25) return this.response;
    if (this.acceptedFrame && (!this.acceptedFrame.active || this.acceptedFrame.externalHold)) return this.response;
    // Accepted wheel evidence samples the accepted pose, never a locally predicted setMotion pose.
    if (this.acceptedFrame) {
      this.position.set(this.acceptedFrame.pose.x, this.acceptedFrame.pose.y, this.acceptedFrame.pose.z);
      this.yaw = this.acceptedFrame.pose.yaw;
    }
    this.init(); const dt = Math.min(delta, .1), { targetSpeed, curvature, brake, acceleration } = this.motion;
    const speed = this.acceptedFrame ? Math.abs(this.acceptedFrame.speed) : this.motion.speed;
    const { longitudinal, lateral, pothole, contactId, contactSide } = this.sampleGround();
    const accepted = this.acceptedFrame;
    const freshContact = !!accepted && accepted.sequence > this.acceptedSequence;
    if (freshContact) {
      if (pothole && contactId && contactId !== this.acceptedContact && Math.abs(accepted.speed) >= 2) {
        this.options.acceptedImpact?.(Object.freeze({ id: `${accepted.scope.drivingId}:wheel:${accepted.sequence}`,
          scope: Object.freeze({ ...accepted.scope }), sequence: accepted.sequence, at: accepted.at,
          contactId, speed: Math.abs(accepted.speed), strength: Math.min(1, Math.abs(accepted.speed) / 14), side: contactSide }));
      }
      this.acceptedContact = contactId; this.acceptedSequence = accepted.sequence;
    }
    this.longitudinalGrip = longitudinal;
    this.cooldown = Math.max(0, this.cooldown - dt);
    if (pothole && !this.contact && this.cooldown === 0 && speed > .2 && (!accepted || freshContact)) {
      const strength = Math.min(1, speed / 14); this.cooldown = .35;
      this.options.impact?.(strength, speed);
      if (speed * 3.6 > 35) this.options.shake?.(Math.min(.18, (speed * 3.6 - 35) / 250));
    }
    this.contact = pothole;
    const braking = targetSpeed < speed;
    this.fade = this.strain?.brakeFade ?? THREE.MathUtils.clamp(this.fade
      + (this.options.danfo && braking && speed > 8 ? .16 : -.08) * dt, 0, .18);
    this.dynamics ??= createVehicleDynamics(this.config, { x: this.position.x, y: this.position.y, z: this.position.z, yaw: this.yaw }, speed);
    if (!accepted) {
      try { this.dynamics = stepVehicleDynamics(this.dynamics, this.config, {
        throttle: THREE.MathUtils.clamp((targetSpeed - speed) * .8, 0, 1) * Math.min(1, acceleration / 3),
        brake: THREE.MathUtils.clamp((speed - targetSpeed) / Math.max(.2, brake * .25), 0, 1),
        steering: Math.atan(this.options.wheelbase * curvature) / vehicleSteeringLimit(this.config, speed) + (this.strain?.handling.steeringBias ?? 0),
        accelerationScale: this.strain?.handling.accelerationScale ?? 1,
        brakingScale: this.strain?.handling.brakingScale ?? 1 - this.fade,
        speedLimit: Math.min(targetSpeed, this.strain?.handling.speedLimit ?? 35),
      }, dt, this.options.groundCast ?? ((origin, direction, length) => {
        this.wheel.set(origin.x, this.position.y, origin.z);
        return castVehicleRoadPlane(origin, direction, length, this.position.y, this.options.ground(this.wheel));
      })); } catch { this.hold(); return this.response; }
      const pose = vehicleDynamicsPose(this.dynamics, this.config), q = this.dynamics.orientation;
      this.attitude.setFromQuaternion(this.rotation.set(q.x, q.y, q.z, q.w), "YXZ");
      this.response.speed = Math.hypot(this.dynamics.velocity.x, this.dynamics.velocity.z);
      this.response.roll = this.attitude.z; this.response.pitch = this.attitude.x;
      this.response.lift = pose.y - this.position.y;
    } else {
      // Accepted motion belongs to another owner. Never integrate it again from a frame receipt.
      this.response.speed = speed;
    }
    this.response.brakingPull = 0;
    this.response.lateralGrip = lateral;
    this.response.slip = Math.max(...this.dynamics.wheels.map(wheel => Math.abs(wheel.slipRatio)));
    return this.response;
  }
  private sampleGround() {
    let longitudinal = .85, lateral = .85, pothole = false;
    let contactId: string | null = null, contactSide: -1 | 0 | 1 = 0;
    for (const side of [-1, 1]) for (const axle of [-1, 1]) {
      const x = side * this.options.halfWidth * .75, z = axle * this.options.wheelbase * .5;
      this.wheel.set(this.position.x + x * Math.cos(this.yaw) + z * Math.sin(this.yaw), this.position.y,
        this.position.z - x * Math.sin(this.yaw) + z * Math.cos(this.yaw));
      const tag = this.options.ground(this.wheel), surface = vehicleSurfaceGrip[tag] ?? vehicleSurfaceGrip.clean_asphalt;
      longitudinal = Math.min(longitudinal, surface.longitudinal); lateral = Math.min(lateral, surface.lateral);
      pothole ||= tag === "pothole";
      if (tag === "pothole") {
        const id = this.options.groundContact?.(this.wheel) ?? null;
        if (id && (!contactId || id < contactId)) { contactId = id; contactSide = side < 0 ? -1 : 1; }
        else if (id === contactId && (side < 0 ? -1 : 1) !== contactSide) contactSide = 0;
      }
    }
    return { longitudinal, lateral, pothole, contactId, contactSide };
  }
  brakingDeceleration(brake: number) {
    // Sample before planning the stopping horizon, including the first wheel entering mud.
    this.longitudinalGrip = this.sampleGround().longitudinal;
    return Math.max(.2, brake * this.longitudinalGrip / .85 * (1 - (this.strain?.brakeFade ?? this.fade)));
  }
  reset() {
    this.contact = false; this.cooldown = this.fade = 0; this.longitudinalGrip = .85; this.dynamics = null;
    this.acceptedFrame = undefined; this.acceptedSequence = -1; this.acceptedContact = null; this.strain = undefined;
    Object.assign(this.response, { speed: 0, roll: 0, pitch: 0, lift: 0, brakingPull: 0, slip: 0, lateralGrip: .85 });
  }
  /** Discard an unaccepted step at an occupancy boundary, retaining the last accepted wheel pose. */
  hold(previous: CompanionVehicleDynamics | null = this.dynamics) {
    this.dynamics = previous && holdVehicleDynamics(previous); this.response.speed = 0;
    if (this.dynamics) {
      const pose = vehicleDynamicsPose(this.dynamics, this.config), q = this.dynamics.orientation;
      this.attitude.setFromQuaternion(this.rotation.set(q.x, q.y, q.z, q.w), "YXZ");
      this.response.roll = this.attitude.z; this.response.pitch = this.attitude.x; this.response.lift = pose.y - this.position.y;
    }
  }
  dispose() { this.reset(); this.disposed = true; }
}

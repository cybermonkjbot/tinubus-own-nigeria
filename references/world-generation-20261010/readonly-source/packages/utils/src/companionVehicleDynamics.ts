/** Metres, kilograms, seconds. Local axes are +X right, +Y up and +Z forward.
 * No renderer, clock, transport or collision ownership lives in this solver. */
export type VehicleVector = { x: number; y: number; z: number };
export type VehicleQuaternion = VehicleVector & { w: number };
export type VehicleSurface = "clean_asphalt" | "laterite_mud" | "pothole" | "gutter_wood";
export type VehicleWheelState = {
  angularVelocity: number; angle: number; steering: number; compression: number;
  normalLoad: number; slipRatio: number; slipAngle: number; longitudinalForce: number; lateralForce: number;
  grounded: boolean;
};
export type CompanionVehicleDynamics = {
  version: 1; position: VehicleVector; orientation: VehicleQuaternion;
  velocity: VehicleVector; angularVelocity: VehicleVector; steering: number;
  engineOmega: number; gear: number; shiftRemaining: number; wheels: VehicleWheelState[];
};
type Wheel = { x: number; z: number; front: boolean; radius: number; mass: number; spring: number; damper: number };
export type VehicleDynamicsConfig = Readonly<{
  mass: number; centerOfMass: Readonly<VehicleVector>; inertia: Readonly<VehicleVector>;
  wheelbase: number; track: number; restLength: number; sag: number; antiRoll: number;
  parkSteering: number; highwaySteering: number; steeringSpeed: number;
  engineTorque: number; engineInertia: number; idleOmega: number; redlineOmega: number;
  finalDrive: number; gears: readonly number[]; brakeTorque: number; wheels: readonly Wheel[];
}>;
export type VehicleGroundHit = {
  distance: number; normal: VehicleVector; velocity: VehicleVector; surface: VehicleSurface;
};
/** The owner supplies a ray query against its admitted road, excluding the vehicle itself. */
export type VehicleGroundCast = (origin: VehicleVector, direction: VehicleVector, length: number) => VehicleGroundHit | null;
export type VehicleDriverInput = Readonly<{
  throttle: number; brake: number; steering: number; accelerationScale?: number; brakingScale?: number; speedLimit?: number;
}>;
const G = 9.81, TAU = Math.PI * 2;
const clamp = (n: number, a: number, b: number) => Math.max(a, Math.min(b, n));
const finite3 = (x: number, y: number, z: number) => Number.isFinite(x) && Number.isFinite(y) && Number.isFinite(z);
const finiteVector = (v: VehicleVector) => !!v && finite3(v.x, v.y, v.z);
const zero = (): VehicleVector => ({ x: 0, y: 0, z: 0 });
const add = (a: VehicleVector, b: VehicleVector): VehicleVector => ({ x: a.x + b.x, y: a.y + b.y, z: a.z + b.z });
const sub = (a: VehicleVector, b: VehicleVector): VehicleVector => ({ x: a.x - b.x, y: a.y - b.y, z: a.z - b.z });
const scale = (a: VehicleVector, s: number): VehicleVector => ({ x: a.x * s, y: a.y * s, z: a.z * s });
const dot = (a: VehicleVector, b: VehicleVector) => a.x * b.x + a.y * b.y + a.z * b.z;
const cross = (a: VehicleVector, b: VehicleVector): VehicleVector => ({ x: a.y * b.z - a.z * b.y,
  y: a.z * b.x - a.x * b.z, z: a.x * b.y - a.y * b.x });
const unit = (a: VehicleVector) => scale(a, 1 / Math.max(1e-9, Math.hypot(a.x, a.y, a.z)));
const rotated = (v: VehicleVector, x: number, y: number, z: number, w: number): VehicleVector => {
  // Keep the cross/scale/add order while avoiding the five temporary vectors.
  const tx = (y * v.z - z * v.y) * 2;
  const ty = (z * v.x - x * v.z) * 2;
  const tz = (x * v.y - y * v.x) * 2;
  return { x: v.x + (tx * w + (y * tz - z * ty)),
    y: v.y + (ty * w + (z * tx - x * tz)),
    z: v.z + (tz * w + (x * ty - y * tx)) };
};
const rotate = (v: VehicleVector, q: VehicleQuaternion) => rotated(v, q.x, q.y, q.z, q.w);
const local = (v: VehicleVector, q: VehicleQuaternion) => rotated(v, -q.x, -q.y, -q.z, q.w);
export const vehicleSurfaceGrip: Readonly<Record<VehicleSurface, { longitudinal: number; lateral: number; rolling: number }>> = Object.freeze({
  clean_asphalt: { longitudinal: .85, lateral: .85, rolling: .012 },
  laterite_mud: { longitudinal: .4, lateral: .4, rolling: .06 },
  pothole: { longitudinal: .85, lateral: .85, rolling: .018 },
  gutter_wood: { longitudinal: .7, lateral: .35, rolling: .02 },
});

export function createVehicleDynamicsConfig(options: {
  kind: "car" | "danfo" | "keke"; wheelbase: number; halfWidth: number; wheelRadius?: number;
  wheelLayout?: readonly { x: number; z: number; front: boolean; radius: number }[];
}): VehicleDynamicsConfig {
  const { kind, wheelbase, halfWidth } = options;
  const keke = kind === "keke", danfo = kind === "danfo";
  const mass = keke ? 420 : danfo ? 2200 : 1300;
  const centerOfMass = { x: 0, y: keke ? .44 : danfo ? .72 : .48, z: keke ? -.15 : danfo ? -.25 : -.08 };
  const track = options.wheelLayout ? Math.max(...options.wheelLayout.map(wheel => wheel.x))
    - Math.min(...options.wheelLayout.map(wheel => wheel.x)) : halfWidth * 1.5;
  const restLength = keke ? .24 : .32, sag = keke ? .06 : .085;
  const radius = options.wheelRadius ?? (keke ? .25 : danfo ? .4 : .32);
  if (![wheelbase, halfWidth, radius].every(Number.isFinite) || wheelbase < 1 || wheelbase > 8
    || halfWidth < .3 || halfWidth > 3 || radius < .1 || radius > 1) throw new Error("Use reviewed vehicle dimensions.");
  const corners = options.wheelLayout ?? (keke ? [{ x: 0, z: wheelbase / 2, front: true },
    { x: -track / 2, z: -wheelbase / 2, front: false }, { x: track / 2, z: -wheelbase / 2, front: false }]
    : [true, false].flatMap(front => [-1, 1].map(side => ({ x: side * track / 2, z: (front ? 1 : -1) * wheelbase / 2, front }))));
  if (corners.length !== (keke ? 3 : 4) || !Number.isFinite(track) || track < .3
    || corners.some(wheel => ![wheel.x, wheel.z].every(Number.isFinite))
    || corners.filter(wheel => wheel.front).length !== (keke ? 1 : 2)) throw new Error("Use the vehicle's complete axle layout.");
  const axleCenter = (Math.max(...corners.map(wheel => wheel.z)) + Math.min(...corners.map(wheel => wheel.z))) / 2;
  const wheels = corners.map(wheel => {
    const axleShare = .5 + (wheel.front ? 1 : -1) * (centerOfMass.z - axleCenter) / wheelbase;
    const cornerMass = mass * axleShare / (keke && wheel.front ? 1 : 2), spring = cornerMass * G / sag;
    const wheelRadius = "radius" in wheel ? wheel.radius : radius;
    if (typeof wheelRadius !== "number" || !Number.isFinite(wheelRadius) || wheelRadius < .1 || wheelRadius > 1 || cornerMass <= 0)
      throw new Error("Use a supported wheel radius and center of mass.");
    return { ...wheel, radius: wheelRadius, mass: keke ? 7 : 18, spring, damper: 2 * .75 * Math.sqrt(spring * cornerMass) };
  });
  return Object.freeze({ mass, centerOfMass: Object.freeze(centerOfMass),
    inertia: Object.freeze({ x: mass * (wheelbase ** 2 + .8 ** 2) / 12,
      y: mass * (wheelbase ** 2 + track ** 2) / 12, z: mass * (track ** 2 + .8 ** 2) / 12 }),
    wheelbase, track, restLength, sag, antiRoll: mass * (keke ? 4 : 7),
    parkSteering: keke ? .48 : 1.08, highwaySteering: keke ? .22 : .16, steeringSpeed: keke ? 10 : 25,
    engineTorque: keke ? 26 : danfo ? 230 : 180, engineInertia: keke ? .18 : .65,
    idleOmega: 95, redlineOmega: 620, finalDrive: keke ? 4.2 : 3.7,
    gears: Object.freeze([3.2, 2.1, 1.45, 1, .8]), brakeTorque: mass * 5.5 * radius / wheels.length,
    wheels: Object.freeze(wheels.map(wheel => Object.freeze(wheel))) });
}

export function createVehicleDynamics(config: VehicleDynamicsConfig, pose: { x: number; y: number; z: number; yaw: number }, speed = 0): CompanionVehicleDynamics {
  const orientation = { x: 0, y: Math.sin(pose.yaw / 2), z: 0, w: Math.cos(pose.yaw / 2) };
  return { version: 1, position: add(pose, rotate(config.centerOfMass, orientation)), orientation,
    velocity: { x: Math.sin(pose.yaw) * speed, y: 0, z: Math.cos(pose.yaw) * speed }, angularVelocity: zero(),
    steering: 0, engineOmega: config.idleOmega, gear: 0, shiftRemaining: 0,
    wheels: config.wheels.map(wheel => ({ angularVelocity: speed / wheel.radius, angle: 0, steering: 0,
      compression: config.sag, normalLoad: wheel.spring * config.sag, slipRatio: 0, slipAngle: 0,
      longitudinalForce: 0, lateralForce: 0, grounded: true })) };
}
export function copyVehicleDynamics(state: CompanionVehicleDynamics): CompanionVehicleDynamics {
  return { ...state, position: { ...state.position }, orientation: { ...state.orientation }, velocity: { ...state.velocity },
    angularVelocity: { ...state.angularVelocity }, wheels: state.wheels.map(wheel => ({ ...wheel })) };
}
export function validVehicleDynamics(state: CompanionVehicleDynamics, config: VehicleDynamicsConfig): boolean {
  return !!state && state.version === 1 && finiteVector(state.position) && finiteVector(state.velocity)
    && finiteVector(state.angularVelocity) && finiteVector(state.orientation) && Number.isFinite(state.orientation.w)
    && Math.abs(Math.hypot(state.orientation.x, state.orientation.y, state.orientation.z, state.orientation.w) - 1) < .001
    && Math.hypot(state.velocity.x, state.velocity.y, state.velocity.z) < 80
    && Math.hypot(state.angularVelocity.x, state.angularVelocity.y, state.angularVelocity.z) < 30
    && finite3(state.steering, state.engineOmega, state.shiftRemaining)
    && Math.abs(state.steering) <= config.parkSteering && state.engineOmega >= 0 && state.engineOmega <= config.redlineOmega * 1.1
    && Number.isInteger(state.gear) && state.gear >= 0 && state.gear < config.gears.length
    && state.shiftRemaining >= 0 && state.shiftRemaining <= .25 && Array.isArray(state.wheels)
    && state.wheels.length === config.wheels.length && state.wheels.every(wheel => wheel
      && typeof wheel.grounded === "boolean" && finite3(wheel.angularVelocity, wheel.angle, wheel.steering)
      && finite3(wheel.compression, wheel.normalLoad, wheel.slipRatio)
      && finite3(wheel.slipAngle, wheel.longitudinalForce, wheel.lateralForce)
      && Math.abs(wheel.angularVelocity) <= 1200 && wheel.compression >= 0 && wheel.compression <= config.restLength
      && wheel.normalLoad >= 0 && wheel.normalLoad <= config.mass * G * 6);
}
/** Only lifecycle/occupancy owners may impose a hold. This is not the service brake. */
export function holdVehicleDynamics(state: CompanionVehicleDynamics): CompanionVehicleDynamics {
  const next = copyVehicleDynamics(state);
  next.velocity = zero(); next.angularVelocity = zero();
  next.wheels.forEach(wheel => { wheel.angularVelocity = wheel.slipRatio = wheel.slipAngle = wheel.longitudinalForce = wheel.lateralForce = 0; });
  return next;
}
export function vehicleDynamicsPose(state: CompanionVehicleDynamics, config: VehicleDynamicsConfig) {
  const origin = sub(state.position, rotate(config.centerOfMass, state.orientation));
  const forward = rotate({ x: 0, y: 0, z: 1 }, state.orientation);
  return { ...origin, yaw: Math.atan2(forward.x, forward.z) };
}
export const vehicleSteeringLimit = (config: VehicleDynamicsConfig, speed: number) =>
  config.parkSteering + (config.highwaySteering - config.parkSteering) * clamp(Math.abs(speed) / config.steeringSpeed, 0, 1);
export function vehicleAckermannAngle(config: VehicleDynamicsConfig, steering: number, x: number) {
  if (Math.abs(steering) < 1e-6) return 0;
  const radius = config.wheelbase / Math.tan(steering);
  return Math.atan(config.wheelbase / (radius - x));
}
/** Magic Formula with E=0. Peak at the configured slip, finite sliding grip beyond it. */
export function vehicleTireCurve(slip: number, peak: number) {
  const c = 1.6, b = Math.tan(Math.PI / (2 * c));
  return Math.sin(c * Math.atan(b * slip / peak));
}
/** Analytic ray-plane intersection for the existing flat navigation floor. No invented bumps. */
export function castVehicleRoadPlane(origin: VehicleVector, direction: VehicleVector, length: number,
  floor: number, surface: VehicleSurface): VehicleGroundHit | null {
  if (direction.y >= -1e-5) return null;
  const distance = (floor - origin.y) / direction.y;
  return distance >= 0 && distance <= length ? { distance, normal: { x: 0, y: 1, z: 0 }, velocity: zero(), surface } : null;
}

/** Advances a private copy. The caller sweeps and accepts the resulting pose before committing it. */
export function stepVehicleDynamics(previous: CompanionVehicleDynamics, config: VehicleDynamicsConfig,
  input: VehicleDriverInput, delta: number, cast: VehicleGroundCast): CompanionVehicleDynamics {
  if (!validVehicleDynamics(previous, config) || !Number.isFinite(delta) || delta <= 0 || delta > .1
    || !finite3(input.throttle, input.brake, input.steering)
    || !finite3(input.accelerationScale ?? 1, input.brakingScale ?? 1, input.speedLimit ?? 35))
    throw new Error("Use a finite, current vehicle physics step.");
  const state = copyVehicleDynamics(previous), steps = Math.ceil(delta * 120), dt = delta / steps;
  for (let step = 0; step < steps; step++) integrate(state, config, input, dt, cast);
  if (!validVehicleDynamics(state, config)) throw new Error("Vehicle contact could not be resolved. Stop before continuing.");
  return state;
}

function integrate(state: CompanionVehicleDynamics, config: VehicleDynamicsConfig, input: VehicleDriverInput,
  dt: number, cast: VehicleGroundCast) {
  const q = state.orientation, up = rotate({ x: 0, y: 1, z: 0 }, q), down = scale(up, -1);
  const worldOmega = rotate(state.angularVelocity, q), speed = Math.hypot(state.velocity.x, state.velocity.z);
  const steering = clamp(input.steering, -1, 1) * vehicleSteeringLimit(config, speed);
  state.steering += clamp(steering - state.steering, -2.5 * dt, 2.5 * dt);
  const limit = input.speedLimit ?? 35;
  const throttle = clamp(input.throttle, 0, 1) * clamp(input.accelerationScale ?? 1, 0, 1)
    * clamp((limit - speed) / .6, 0, 1);
  const brake = Math.max(clamp(input.brake, 0, 1), clamp((speed - limit) * 2, 0, 1));

  // Engine, clutch, gearbox and an open rear differential. Airborne wheels still rotate.
  state.shiftRemaining = Math.max(0, state.shiftRemaining - dt);
  if (!state.shiftRemaining) {
    if (state.engineOmega > 490 && state.gear < config.gears.length - 1) { state.gear++; state.shiftRemaining = .2; }
    else if (state.engineOmega < 180 && state.gear > 0) { state.gear--; state.shiftRemaining = .2; }
  }
  const ratio = config.gears[state.gear] * config.finalDrive;
  let drivenCount = 0, drivenOmega = 0;
  for (let index = 0; index < config.wheels.length; index++) {
    if (config.wheels[index].front) continue;
    drivenCount++;
    drivenOmega += state.wheels[index].angularVelocity;
  }
  const shaftOmega = drivenOmega / drivenCount * ratio;
  const clutch = state.shiftRemaining ? 0 : clamp((state.engineOmega - shaftOmega) * .8,
    -config.engineTorque * 1.5, config.engineTorque * 1.5) * clamp(throttle * 4 + speed / 3, 0, 1);
  const engineTorque = throttle * config.engineTorque * clamp(1 - ((state.engineOmega - 350) / 420) ** 2, .25, 1);
  const idleTorque = Math.max(0, config.idleOmega - state.engineOmega) * config.engineInertia * 20;
  state.engineOmega = clamp(state.engineOmega + (engineTorque + idleTorque - clutch - .03 * state.engineOmega) / config.engineInertia * dt,
    0, config.redlineOmega);
  const driveTorque = clutch * ratio * .9 / drivenCount;
  for (let index = 0; index < config.wheels.length; index++) {
    const wheel = config.wheels[index], out = state.wheels[index], inertia = wheel.mass * wheel.radius ** 2 * .5;
    out.steering = wheel.front ? vehicleAckermannAngle(config, state.steering, wheel.x) : 0;
    out.angularVelocity += (wheel.front ? 0 : driveTorque) / inertia * dt;
    const braking = brake * config.brakeTorque * clamp(input.brakingScale ?? 1, .5, 1) / inertia * dt;
    out.angularVelocity -= clamp(out.angularVelocity, -braking, braking);
  }

  const contacts = config.wheels.map((wheel, index) => {
    const hardpoint = rotate({ x: wheel.x - config.centerOfMass.x,
      y: wheel.radius + config.restLength - config.sag - config.centerOfMass.y, z: wheel.z - config.centerOfMass.z }, q);
    const origin = add(state.position, hardpoint), hit = cast(origin, down, config.restLength + wheel.radius);
    const out = state.wheels[index];
    const valid = hit && Number.isFinite(hit.distance) && hit.distance >= 0 && hit.distance <= config.restLength + wheel.radius
      && finite3(hit.normal.x, hit.normal.y, hit.normal.z) && finite3(hit.velocity.x, hit.velocity.y, hit.velocity.z)
      && dot(hit.normal, up) > .15;
    out.grounded = !!valid; out.normalLoad = out.longitudinalForce = out.lateralForce = out.slipRatio = out.slipAngle = 0;
    out.compression = valid ? clamp(config.restLength - (hit.distance - wheel.radius), 0, config.restLength) : 0;
    if (!valid) return null;
    const normal = unit(hit.normal), patch = add(hardpoint, scale(down, hit.distance));
    const relativeX = hit.velocity.x - (state.velocity.x + (worldOmega.y * hardpoint.z - worldOmega.z * hardpoint.y));
    const relativeY = hit.velocity.y - (state.velocity.y + (worldOmega.z * hardpoint.x - worldOmega.x * hardpoint.z));
    const relativeZ = hit.velocity.z - (state.velocity.z + (worldOmega.x * hardpoint.y - worldOmega.y * hardpoint.x));
    const compressionVelocity = relativeX * up.x + relativeY * up.y + relativeZ * up.z;
    // Bump stop adds stiffness near full travel, without changing the rest ride height.
    const spring = wheel.spring * out.compression + wheel.spring * 4 * Math.max(0, out.compression - config.restLength * .8);
    const suspension = clamp(spring + wheel.damper * compressionVelocity, 0, config.mass * G * 4);
    return { patch, normal, velocity: hit.velocity, surface: hit.surface, suspension };
  });
  // Cross-couple each paired axle. A wheel without contact cannot receive tire load.
  for (const front of [true, false]) {
    const left = config.wheels.findIndex(wheel => wheel.front === front && wheel.x < 0);
    const right = config.wheels.findIndex(wheel => wheel.front === front && wheel.x > 0);
    const a = contacts[left], b = contacts[right];
    if (!a || !b) continue;
    const transfer = clamp(config.antiRoll * (state.wheels[left].compression - state.wheels[right].compression), -a.suspension, b.suspension);
    a.suspension += transfer; b.suspension -= transfer;
  }
  const force: VehicleVector = { x: 0, y: -config.mass * G, z: 0 }, torque = zero();
  // Accumulators belong to this substep; contact vectors remain independent.
  const apply = (x: number, y: number, z: number, patch: VehicleVector) => {
    force.x += x; force.y += y; force.z += z;
    torque.x += patch.y * z - patch.z * y;
    torque.y += patch.z * x - patch.x * z;
    torque.z += patch.x * y - patch.y * x;
  };
  contacts.forEach((contact, index) => {
    if (!contact) return;
    const wheel = config.wheels[index], out = state.wheels[index], inertia = wheel.mass * wheel.radius ** 2 * .5;
    out.normalLoad = contact.suspension * dot(up, contact.normal);
    apply(up.x * contact.suspension, up.y * contact.suspension, up.z * contact.suspension, contact.patch);
    const wheelForward = rotate({ x: Math.sin(out.steering), y: 0, z: Math.cos(out.steering) }, q);
    const forward = unit(sub(wheelForward, scale(contact.normal, dot(wheelForward, contact.normal))));
    const right = unit(cross(contact.normal, forward));
    const velocityX = state.velocity.x + (worldOmega.y * contact.patch.z - worldOmega.z * contact.patch.y) - contact.velocity.x;
    const velocityY = state.velocity.y + (worldOmega.z * contact.patch.x - worldOmega.x * contact.patch.z) - contact.velocity.y;
    const velocityZ = state.velocity.z + (worldOmega.x * contact.patch.y - worldOmega.y * contact.patch.x) - contact.velocity.z;
    const longitudinal = velocityX * forward.x + velocityY * forward.y + velocityZ * forward.z;
    const lateral = velocityX * right.x + velocityY * right.y + velocityZ * right.z, rimSpeed = out.angularVelocity * wheel.radius;
    out.slipRatio = (rimSpeed - longitudinal) / Math.max(.75, Math.abs(longitudinal), Math.abs(rimSpeed));
    out.slipAngle = Math.atan2(lateral, Math.max(.75, Math.abs(longitudinal)));
    const grip = vehicleSurfaceGrip[contact.surface] ?? vehicleSurfaceGrip.clean_asphalt;
    const capacityX = grip.longitudinal * out.normalLoad, capacityY = grip.lateral * out.normalLoad;
    const inverseMass = (axis: VehicleVector) => {
      const arm = local(cross(contact.patch, axis), q);
      return config.wheels.length / config.mass + arm.x ** 2 / config.inertia.x + arm.y ** 2 / config.inertia.y + arm.z ** 2 / config.inertia.z;
    };
    const rawX = capacityX * vehicleTireCurve(out.slipRatio, .12);
    const rawY = -capacityY * vehicleTireCurve(out.slipAngle, .13);
    // Bound the impulse by the relative velocity it can remove. This avoids the
    // zero-speed stiffness explosion without replacing wheel velocity with road speed.
    let fx = Math.sign(rawX) * Math.min(Math.abs(rawX), Math.abs(rimSpeed - longitudinal)
      / (dt * (wheel.radius ** 2 / inertia + inverseMass(forward))));
    let fy = Math.sign(rawY) * Math.min(Math.abs(rawY), Math.abs(lateral) / (dt * inverseMass(right)));
    const ellipse = Math.hypot(fx / Math.max(1e-6, capacityX), fy / Math.max(1e-6, capacityY));
    if (ellipse > 1) { fx /= ellipse; fy /= ellipse; }
    out.longitudinalForce = fx; out.lateralForce = fy;
    apply(forward.x * fx + right.x * fy, forward.y * fx + right.y * fy,
      forward.z * fx + right.z * fy, contact.patch);
    out.angularVelocity -= fx * wheel.radius / inertia * dt;
    const rolling = grip.rolling * out.normalLoad * wheel.radius / inertia * dt;
    out.angularVelocity -= clamp(out.angularVelocity, -rolling, rolling);
  });
  // A tipped or bottomed-out chassis still has ground collision. These passive
  // contacts are not wheels and never receive engine torque or tire grip.
  if (up.y < .9 || state.wheels.some(wheel => wheel.compression > config.restLength * .95)) {
    for (const x of [-1, 1]) for (const z of [-1, 1]) for (const y of [.16, config.centerOfMass.y * 2 + .3]) {
      const arm = rotate({ x: x * (config.track / 2 + .08), y: y - config.centerOfMass.y,
        z: z * (config.wheelbase / 2 + .25) - config.centerOfMass.z }, q);
      const corner = add(state.position, arm);
      const hit = cast({ x: corner.x, y: corner.y + .5, z: corner.z }, { x: 0, y: -1, z: 0 }, .5);
      if (!hit || !Number.isFinite(hit.distance) || hit.distance < 0 || hit.distance >= .5 || hit.normal.y < .3) continue;
      const normal = unit(hit.normal);
      const velocityX = state.velocity.x + (worldOmega.y * arm.z - worldOmega.z * arm.y) - hit.velocity.x;
      const velocityY = state.velocity.y + (worldOmega.z * arm.x - worldOmega.x * arm.z) - hit.velocity.y;
      const velocityZ = state.velocity.z + (worldOmega.x * arm.y - worldOmega.y * arm.x) - hit.velocity.z;
      const normalVelocity = velocityX * normal.x + velocityY * normal.y + velocityZ * normal.z;
      const moment = local(cross(arm, normal), q);
      const inverseMass = 8 / config.mass + moment.x ** 2 / config.inertia.x + moment.y ** 2 / config.inertia.y + moment.z ** 2 / config.inertia.z;
      const reaction = clamp(((.5 - hit.distance) * .2 / dt - normalVelocity) / (dt * inverseMass), 0, config.mass * G * 2);
      apply(normal.x * reaction, normal.y * reaction, normal.z * reaction, arm);
    }
  }
  for (const wheel of state.wheels) wheel.angle = ((wheel.angle + wheel.angularVelocity * dt) % TAU + TAU) % TAU;
  // Aerodynamic drag acts at the CoM. All propulsion and service braking above act at tires.
  const drag = -.45 * Math.hypot(state.velocity.x, state.velocity.y, state.velocity.z);
  force.x += state.velocity.x * drag; force.y += state.velocity.y * drag; force.z += state.velocity.z * drag;
  state.velocity = add(state.velocity, scale(force, dt / config.mass));
  state.position = add(state.position, scale(state.velocity, dt));
  const bodyTorque = local(torque, q), w = state.angularVelocity;
  const gyroscopic = cross(w, { x: w.x * config.inertia.x, y: w.y * config.inertia.y, z: w.z * config.inertia.z });
  state.angularVelocity = { x: w.x + (bodyTorque.x - gyroscopic.x) / config.inertia.x * dt,
    y: w.y + (bodyTorque.y - gyroscopic.y) / config.inertia.y * dt, z: w.z + (bodyTorque.z - gyroscopic.z) / config.inertia.z * dt };
  const omega = state.angularVelocity, half = dt * .5;
  const next = { x: q.x + half * (q.w * omega.x + q.y * omega.z - q.z * omega.y),
    y: q.y + half * (q.w * omega.y + q.z * omega.x - q.x * omega.z),
    z: q.z + half * (q.w * omega.z + q.x * omega.y - q.y * omega.x),
    w: q.w - half * dot(q, omega) };
  const norm = Math.hypot(next.x, next.y, next.z, next.w);
  state.orientation = { x: next.x / norm, y: next.y / norm, z: next.z / norm, w: next.w / norm };
}

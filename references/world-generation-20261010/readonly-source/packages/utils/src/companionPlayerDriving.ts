import type { CompanionPlayerVehicle, CompanionPlayerVehicleLocation } from "./companionPlayerVehicles";
import type { RoomNavigation, ScenePoint } from "./companionNavigation";
import type { CompanionVehicleDynamics } from "./companionVehicleDynamics";

/** Real server time, never the accelerated world clock or rendered frame time. */
export const companionPlayerDrivingRules = Object.freeze({
  version: "lagos-player-driving-v1", kind: "keke" as const,
  routeId: "unity-workshop-courier-v1", courierDefinitionId: "workshop-fuses",
  maxSpeed: 7, acceleration: 1.4, braking: 4.2,
  wheelbase: 1.8, halfWidth: .78, halfLength: 1.4, clearance: .15,
  maxSteering: .48, stepMs: 100, substepMs: 20, inputTtlMs: 250,
  maxPredictionMs: 250, boardingReach: 2.1, stoppedSpeed: .15,
  maxOccupants: 1, maxBodies: 128, maxReceipts: 8,
});
export type CompanionDrivingPose = Readonly<{ x: number; y: number; z: number; yaw: number }>;
export type CompanionDrivingInput = Readonly<{ throttle: number; brake: number; steering: number }>;
export const neutralCompanionDrivingInput: CompanionDrivingInput = Object.freeze({ throttle: 0, brake: 1, steering: 0 });
export type CompanionDrivingMode = "driving" | "stopped" | "pulled-over" | "suspended";
export type CompanionDrivingSurface = "clean_asphalt" | "laterite_mud" | "pothole" | "gutter_wood";
export type CompanionDrivingOccupant = Readonly<{ userId: string; characterId: string; lifeId: string; role: "driver"; seat: 0 }>;
export type CompanionDrivingContact = Readonly<{ id: string; bodyId: string; at: number; speed: number; pose: CompanionDrivingPose }>;
export type CompanionPlayerDrivingState = Readonly<{
  id: string; vehicleId: string; vehicleRevision: number; gameRoomId: string; instanceId: string;
  navigationRevision: string; routeId: string; driver: CompanionDrivingOccupant;
  occupants: readonly CompanionDrivingOccupant[]; revision: number; sequence: number;
  pose: CompanionDrivingPose; speed: number; mode: CompanionDrivingMode; input: CompanionDrivingInput;
  acceptedAt: number; inputExpiresAt: number; drivenMetres: number; fuelMetres: number;
  surface: CompanionDrivingSurface; contact: CompanionDrivingContact | null;
  /** Server-owned mechanical state. Older parked/active records initialise on their next accepted step. */
  dynamics?: CompanionVehicleDynamics;
}>;
/** Clients send input only. Pose, elapsed time, contact, fuel and passenger grants are not commands. */
export type CompanionDrivingCommand = Readonly<{
  drivingId: string; vehicleId: string; vehicleRevision: number; drivingRevision: number;
  gameRoomId: string; instanceId: string; lifeId: string; worldRevision: number;
  sequence: number; requestId: string;
  action: "input" | "stop" | "pull-over" | "exit" | "horn"; input: CompanionDrivingInput;
}>;
/** Road mechanics returns handling only; the driving owner alone accepts the final transform. */
export type CompanionDrivingHandling = Readonly<{
  accelerationScale: number; brakingScale: number; speedLimit: number;
  steeringBias: number; surface: CompanionDrivingSurface;
}>;
export type CompanionDrivingBody = Readonly<{
  id: string; kind: "vehicle" | "pedestrian"; pose: CompanionDrivingPose;
  halfWidth: number; halfLength: number; speed: number;
}>;
export type CompanionDrivingRoute = Readonly<{
  id: string; navigationRevision: string; points: readonly ScenePoint[]; halfWidth: number;
  start: CompanionDrivingPose; pullOvers: readonly CompanionDrivingPose[];
}>;
export type CompanionDrivingAuthority = Readonly<{
  userId: string; characterId: string; lifeId: string; gameRoomId: string; instanceId: string;
  worldRevision: number; now: number; present: boolean; ready: boolean; movementOwned: boolean;
  journeyActive: boolean; transitPassenger: boolean; position: ScenePoint;
  location: CompanionPlayerVehicleLocation;
  /** Complete current nearby occupancy, resolved by world, never supplied by the player. */
  representedOccupants: readonly Readonly<{ userId: string; vehicleId: string; role: "driver" | "passenger" }>[];
  navigation: RoomNavigation; route: CompanionDrivingRoute; bodies: readonly CompanionDrivingBody[];
}>;
export type CompanionDrivingResult = Readonly<{
  state: CompanionPlayerDrivingState; vehicle: CompanionPlayerVehicle;
  exitPoint: ScenePoint | null; driverAction: "input" | "stop" | "pull-over" | "exit" | "horn";
  /** False for local prediction. Only an accepted world receipt may feed outcomes. */
  accepted: boolean;
}>;

import type { CompanionCityParcel } from "./companionCity";
import type { CityArchitecturePart, CityArchitectureMaterial, CityArchitectureGeometry } from "./companionCityArchitectureDetails";

type ParkingKind = "compound" | "workshop" | "loading" | "market";
export type CityVehicleKind = "sedan" | "suv" | "van";
export type CityVehicleCondition = "clean" | "used" | "worn" | "shell";
type ParkingTreatment = { kind: ParkingKind; forecourtDepth: number };

function variation(id: string, salt: string) {
  let hash = 2166136261;
  for (const char of `${id}:${salt}`) hash = Math.imul(hash ^ char.charCodeAt(0), 16777619);
  return (hash >>> 0) / 4294967296;
}

/** Scenery stays in existing plot reserves. It never represents an owned or boardable vehicle. */
export function companionCityParkingTreatment(p: CompanionCityParcel): ParkingTreatment | null {
  if (p.depth < 18) return null;
  if (p.garden >= 3) {
    if (p.units >= 8) return { kind: "compound", forecourtDepth: 0 };
    const chance = p.district === "lagoon-gardens" ? .9 : p.district === "skyline" ? .8
      : p.district === "marina" ? .65 : p.district === "palm-courts" ? .7 : .6;
    return variation(p.id, "parked-vehicle") < chance ? { kind: "compound", forecourtDepth: 0 } : null;
  }
  // Recess only selected one-storey commercial buildings, never a dwelling or hosted signature.
  if (p.units !== 0 || p.floors !== 1 || p.width < 23) return null;
  const kind = p.style === "workshop" ? "workshop" : p.style === "warehouse" ? "loading"
    : p.style === "market-shophouse" ? "market" : null;
  return kind && variation(p.id, "parking-court") < (kind === "workshop" ? .3 : .16)
    ? { kind, forecourtDepth: 6.2 } : null;
}

/** Shared metre-scale bodies for parked scenery and the nearby background fleet. */
export function companionCityVehicleParts(id: string, kind: CityVehicleKind, condition: CityVehicleCondition = "clean",
  index = 0, turn = 0, hoodOpen = false, cx = 0, cz = 0): CityArchitecturePart[] {
  const parts: CityArchitecturePart[] = [];
  const add = (role: string, material: CityArchitectureMaterial, x: number, y: number, z: number,
    w: number, h: number, d: number, colour: string, geometry: CityArchitectureGeometry = "box",
    rotation?: readonly [number, number, number]) => {
    parts.push({ role: `parking ${role}`, material, x, y, z, w, h, d, colour, geometry,
      shadow: material !== "vehicleGlass", rotation });
  };
  const large = kind !== "sedan", van = kind === "van", shell = condition === "shell";
  const width = large ? 1.98 : 1.83, length = van ? 4.85 : large ? 4.72 : 4.46;
  const roof = van ? 2.14 : large ? 1.84 : 1.49;
  const half = width / 2, wheelZ = length * .31, wheelY = shell ? .26 : large ? .39 : .34;
  const bonnetDepth = length * .25, bonnetHinge = length * .245, bonnetPitch = hoodOpen ? -.92 : 0;
  const paint = condition === "clean" ? "vehiclePaint" : "wornVehiclePaint";
  const palette = van ? ["#c4a342", "#a69f8c", "#b3b2a1"] : condition === "clean"
    ? ["#deded6", "#292f32", "#b3bec0", "#ddd3c4"] : ["#8b8e80", "#929d98", "#a29383", "#617478"];
  const colour = palette[Math.floor(variation(id, `car-colour-${index}`) * palette.length)];
  const at = (role: string, material: CityArchitectureMaterial, x: number, y: number, z: number,
    w: number, h: number, d: number, tint: string, geometry: CityArchitectureGeometry = "box", tilt = 0) => {
    add(role, material, cx + x * Math.cos(turn) + z * Math.sin(turn), y,
      cz - x * Math.sin(turn) + z * Math.cos(turn), w, h, d, tint, geometry,
      tilt || turn ? [tilt, turn, 0] : undefined);
  };
  at("vehicle chassis", "rubber", 0, .38, 0, width * .83, .18, length * .87, "#30332f", "bevel");
  at("vehicle lower body", paint, 0, .66, 0, width, .57, length, colour, "bevel");
  // Keep the rear hinge against the body when the bonnet is propped open.
  at("vehicle bonnet", paint, 0, .94 - Math.sin(bonnetPitch) * bonnetDepth / 2,
    bonnetHinge + Math.cos(bonnetPitch) * bonnetDepth / 2,
    width * .91, .095, bonnetDepth, colour, "bevel", bonnetPitch);
  const cabinZ = van ? -.21 : -.1, cabinDepth = van ? length * .72 : large ? length * .64 : length * .54;
  at("vehicle glazing", "vehicleGlass", 0, (roof + 1.01) / 2, cabinZ,
    width * .85, roof - 1.01, cabinDepth, shell ? "#5b6257" : "#314548", van ? "bevel" : "vehicleCabin");
  at("vehicle roof", paint, 0, roof, cabinZ, width * .71, .095, cabinDepth * .67, colour, "bevel");
  for (const side of [-1, 1]) {
    at("window sill", paint, side * width * .42, 1.04, cabinZ, .075, .075, cabinDepth * .91, colour);
    at("window pillar", paint, side * width * .38, (roof + 1.07) / 2, cabinZ,
      .075, roof - 1.03, .095, colour);
    if (van) for (const z of [-.95, .57]) at("van window pillar", paint, side * width * .415,
      (roof + 1.07) / 2, z, .075, roof - 1.03, .085, colour);
    at("door handle", "metal", side * (half + .006), .91, -.33, .035, .045, .18, "#aab0a8");
    if (!shell) at("wing mirror", paint, side * (half + .06), 1.11, .72, .16, .115, .19, colour, "bevel");
    for (const z of [-wheelZ, wheelZ]) {
      if (shell) {
        at("support block", "stone", side * half * .82, .17, z, .32, .34, .45, "#8e9386");
      } else {
        const diameter = wheelY * 2;
        at("tyre", "rubber", side * half, wheelY, z, .22, diameter, diameter, "#272d2b", "vehicleWheel");
        if (condition !== "worn" || side < 0) at("wheel hub", "metal", side * (half + .115), wheelY, z,
          .028, diameter * .55, diameter * .55, condition === "clean" ? "#bcc2bc" : "#747c72", "vehicleWheel");
      }
    }
    if (condition === "worn" || shell) {
      at("primer patch", "wornVehiclePaint", side * (half + .005), .72, -.7, .012, .2, .63, "#8e9289");
      at("rusted sill", "wornVehiclePaint", side * (half + .009), .45, .2, .015, .055, 1.2, "#79583e");
    }
  }
  for (const side of [-1, 1]) {
    at("headlamp", "glass", side * width * .32, .76, length / 2 + .012, width * .21, .13, .035, "#c5c7b5");
    at("tail lamp", "recess", side * width * .35, .76, -length / 2 - .013, .19, .16, .035, "#854b40");
  }
  at("front bumper", "metal", 0, .48, length / 2 + .015, width * .94, .115, .1, "#7e8880", "bevel");
  at("rear bumper", "metal", 0, .47, -length / 2 - .025, width * .94, .115, .1, "#7e8880", "bevel");
  at("front plate", "stone", 0, .62, length / 2 + .072, .46, .12, .025, "#c1c9b4");
  at("rear plate", "stone", 0, .62, -length / 2 - .082, .46, .12, .025, "#c1c9b4");
  if (hoodOpen) {
    const supportHeight = .94 + (length * .29 - bonnetHinge) * Math.tan(-bonnetPitch) - 1;
    at("bonnet support", "wood", -.43, 1 + supportHeight / 2, length * .29,
      .035, supportHeight, .035, "#9d825f");
    at("engine bay", "rubber", 0, .99, length * .33, width * .74, .055, length * .2, "#333a34");
  }
  if (shell) {
    at("dusty bonnet", "wornVehiclePaint", 0, 1.002, length * .33, width * .72, .012, .65, "#aba58e");
    for (let i = 0; i < 3; i++) at("wares on bonnet", "recess", -.45 + i * .42, 1.09,
      length * .34, .25, .18, .27, ["#9d654d", "#687c68", "#b19856"][i], "bevel");
  }
  return parts;
}

/** Parcel-local construction, consumed by the existing batched architecture renderer. */
export function companionCityParkingParts(p: CompanionCityParcel): CityArchitecturePart[] {
  const treatment = companionCityParkingTreatment(p);
  if (!treatment) return [];
  const parts: CityArchitecturePart[] = [];
  const add = (role: string, material: CityArchitectureMaterial, x: number, y: number, z: number,
    w: number, h: number, d: number, colour: string, geometry: CityArchitectureGeometry = "box") => {
    parts.push({ role: `parking ${role}`, material, x, y, z, w, h, d, colour, geometry,
      shadow: material !== "vehicleGlass" });
  };
  const front = p.depth / 2;
  const entryX = (p.entrance[0] - p.x) * Math.cos(p.yaw) - (p.entrance[2] - p.z) * Math.sin(p.yaw);
  const vehicle = (x: number, z: number, kind: CityVehicleKind, condition: CityVehicleCondition,
    index: number, turn = 0, hoodOpen = false) => {
    parts.push(...companionCityVehicleParts(p.id, kind, condition, index, turn, hoodOpen, x, z));
  };

  if (treatment.kind === "compound") {
    const luxury = p.district === "lagoon-gardens" || p.district === "skyline";
    const side = variation(p.id, "parking-side") < .5 ? -1 : 1;
    const wanted = p.units >= 64 ? 6 : p.units >= 8 ? 3 + Math.floor(variation(p.id, "parking-count") * 2)
      : 1 + Number(p.garden >= 6 && variation(p.id, "second-car") < .6);
    let index = 0;
    for (let z = front - 4.05; z - 2.75 >= -front + .8 && index < wanted; z -= 6.1) {
      // Existing compound trees occupy the middle of each side reserve.
      if (Math.abs(z) < 3.65) continue;
      for (const parkingSide of [side, -side]) {
        if (index >= wanted) break;
        const x = parkingSide * (p.width / 2 + (p.garden >= 6 ? 2.5 : 1.4));
        if (Math.abs(x - entryX) < 4) continue;
        const car = luxury && variation(p.id, `parked-suv-${index}`) < .55 ? "suv" : "sedan";
        vehicle(x, z, car, luxury || p.district === "garden-quarter" ? "clean" : "used", index);
        add("paved bay", "stone", x, .009, z, 2.6, .018, 5.5, "#acae9e");
        for (const depth of [-1, 1]) add("parking bay end", "stone", x, .023, z + depth * 2.53,
          2.28, .01, .055, "#d3cfb9");
        if (index === 0 && luxury && p.garden >= 6 && variation(p.id, "carport") < .45) {
          add("fabric carport", "canvas", x, 2.73, z, 2.76, .09, 5.57, "#b9b49e", "bevel");
          for (const depth of [-1, 1]) add("carport post", "metal", x + parkingSide * 1.22, 1.33,
            z + depth * 2.47, .09, 2.66, .09, "#626e62");
        }
        index++;
      }
    }
  } else {
    const z = front - 3.12;
    add("forecourt ground", "stone", 0, .012, z, p.width - .5, .024, 5.9, "#9b9783");
    const slots = [-p.width / 2 + 2.0, -p.width / 2 + 4.8, p.width / 2 - 2.05]
      .filter(x => Math.abs(x - entryX) >= 4.0);
    const count = treatment.kind === "workshop" ? Math.min(3, slots.length) : 1;
    for (let i = 0; i < count; i++) {
      const kind = treatment.kind === "loading" || treatment.kind === "market" && i === 0 ? "van" : "sedan";
      const shell = treatment.kind === "market" && variation(p.id, "abandoned-shell") < .25;
      vehicle(slots[i], z, shell ? "sedan" : kind, shell ? "shell" : "worn", i,
        (variation(p.id, `parking-angle-${i}`) - .5) * .13, treatment.kind === "workshop" && i === 0);
    }
    const equipmentX = p.width / 2 - 5.1;
    if (treatment.kind === "workshop" && Math.abs(equipmentX - entryX) >= 4) {
      const x = equipmentX;
      add("compressor", "recess", x, .33, z + 1.7, .66, .55, .43, "#984f42", "bevel");
      add("compressor handle", "metal", x, .66, z + 1.7, .48, .045, .035, "#4b5750");
      add("tube testing drum", "metal", x, .48, z + .72, .66, .84, .66, "#627971", "pipe");
      add("drum water", "glass", x, .9, z + .72, .57, .025, .57, "#54746d", "pipe");
      add("engine pallet", "wood", x, .075, z - .7, .86, .15, .7, "#806b4e");
      add("removed engine", "metal", x, .39, z - .7, .56, .5, .48, "#59615b", "bevel");
      for (let i = 0; i < 3; i++) add("spare tyre", "rubber", x, .119 + i * .19, z - 1.83,
        .63, .19, .63, "#2d342e", "pipe");
    }
  }
  return parts;
}

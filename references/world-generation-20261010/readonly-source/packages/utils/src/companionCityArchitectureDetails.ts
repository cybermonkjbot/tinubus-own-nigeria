import type { CompanionCityParcel } from "./companionCity";
import { companionCityParkingParts, companionCityParkingTreatment } from "./companionCityParking";
import { companionCityFrontageBoundaryParts } from "./companionCityFrontages";
import { companionCityFootballFrontage } from "./companionCityFootballFrontages";
import { companionCitySpatialInfill, companionCityTradePressure } from "./companionCitySpatialInfill";

export type CityArchitectureMaterial = "plaster" | "stone" | "metal" | "glass" | "wood" | "canvas" | "roof" | "recess"
  | "vehiclePaint" | "wornVehiclePaint" | "vehicleGlass" | "rubber" | "groundSeam";
export type CityArchitectureGeometry = "box" | "bevel" | "pipe" | "gable" | "umbrella" | "vehicleWheel" | "vehicleCabin" | "ball"
  | "tyre" | "securityCoil";
/** Parcel-local metric geometry. +Z faces the parcel's authored street. */
export type CityArchitecturePart = {
  role: string;
  x: number; y: number; z: number;
  w: number; h: number; d: number;
  material: CityArchitectureMaterial;
  geometry: CityArchitectureGeometry;
  colour: string;
  shadow: boolean;
  text?: string;
  print?: "paint" | "poster" | "banner";
  printVariant?: number;
  /** Local pitch, yaw and roll in radians, applied in YXZ order before parcel yaw. */
  rotation?: readonly [number, number, number];
};

const finishes: Record<string, readonly string[]> = {
  "town-centre": ["#d9cdb7", "#e8dfce", "#b5b6ad", "#c5aa94", "#cbc8ba", "#eee5d8"],
  "palm-courts": ["#d4c28f", "#dfd2ae", "#a6b69b", "#cfad97", "#ddd6bf", "#b8bdac"],
  "garden-quarter": ["#e5e1d1", "#bbc5ac", "#cfc0ac", "#e0d1b6", "#c2cbbb", "#d5bfa9"],
  "south-market": ["#b98762", "#c6ab80", "#d4c198", "#ad9d85", "#c5b29b", "#cba480"],
  marina: ["#c3c4b5", "#cfd6cf", "#aab9b7", "#cebaa6", "#e2ddd0", "#a9b9bf"],
  skyline: ["#b8cacf", "#d5d9d0", "#9aaeb2", "#dedbd0", "#b4c0b8", "#b9b9ad"],
  "lagoon-gardens": ["#efe9db", "#d9d2be", "#e9e2d1", "#cfcec0", "#e0d8c8", "#c6d0c5"],
};

/** Stable variation survives cell release/re-entry and does not use global randomness. */
function variation(id: string, salt: string) {
  let hash = 2166136261;
  for (const char of `${id}:${salt}`) hash = Math.imul(hash ^ char.charCodeAt(0), 16777619);
  return (hash >>> 0) / 4294967296;
}

/** The Home interaction point stays outside the plot. This is its inward doorway axis. */
export function companionCityArchitectureEntrance(parcel: CompanionCityParcel) {
  const dx = parcel.entrance[0] - parcel.x, dz = parcel.entrance[2] - parcel.z;
  return { x: dx * Math.cos(parcel.yaw) - dz * Math.sin(parcel.yaw),
    z: dx * Math.sin(parcel.yaw) + dz * Math.cos(parcel.yaw) };
}

/** All parts, including roof overhangs, fit the existing oriented parcel obstacle. */
export function companionCityArchitectureParts(p: CompanionCityParcel): CityArchitecturePart[] {
  const parts: CityArchitecturePart[] = [];
  const parking = companionCityParkingTreatment(p), forecourt = parking?.forecourtDepth ?? 0;
  const w = p.width, d = p.depth, h = p.floors * 3.3, front = d / 2 - forecourt, back = -d / 2;
  const entry = companionCityArchitectureEntrance(p).x;
  const tint = (finishes[p.district] ?? finishes["town-centre"])[p.variant % 6];
  const trim = p.district === "south-market" ? "#b7a38b" : p.district === "skyline" ? "#d1d5cf" : "#d8cfbb";
  const steel = p.district === "marina" ? "#536c70" : "#505c55";
  const timber = p.district === "lagoon-gardens" ? "#9b805c" : "#80654a";
  const awningColours = ["#987655", "#687b6d", "#bd9368", "#5e737c", "#92715d", "#ac9c75"];
  const awning = awningColours[Math.floor(variation(p.id, "awning") * awningColours.length)];
  const courtyard = p.style === "courtyard";
  const luxury = p.style === "terrace-villa" || p.style === "courtyard-villa";
  const tower = p.style === "stepped-tower";
  const shed = p.style === "warehouse" || p.style === "workshop";
  const retail = p.units === 0 && !shed;
  const pitched = shed || p.style === "townhouse" || p.style === "garden-home";
  // Land use and individual frontage matter. A tower or villa does not inherit market clutter.
  const maintained = tower || luxury || p.style === "waterfront" || p.district === "skyline" || p.district === "lagoon-gardens";
  const informal = !maintained && (p.style === "market-shophouse" || shed);
  const tradePressure = informal ? companionCityTradePressure(p) : 0;
  const football = companionCityFootballFrontage(p);
  const shells: { x: number; z: number; width: number; depth: number; base: number; top: number }[] = [];
  // Recess the front structure so balconies and sheltered entrances do not overhang routes.
  const face = front - 1.3;
  const add = (role: string, material: CityArchitectureMaterial, x: number, y: number, z: number,
    width: number, height: number, depth: number, colour = trim,
    geometry: CityArchitectureGeometry = "box", shadow = true) => {
    parts.push({ role, material, x, y, z, w: width, h: height, d: depth, colour, geometry, shadow });
  };
  const sign = (text: string, x: number, y: number, z: number, width: number, height: number) => {
    add("painted street sign", "wood", x, y, z, width, height, .035, "#ffffff", "box", false);
    parts[parts.length - 1].text = text;
  };
  const slab = (role: string, x: number, z: number, width: number, depth: number, height: number, base = 0) => {
    // Reserve side/rear reveal depth inside the existing collision envelope.
    // The street-facing plane and saved doorway axis remain unchanged.
    const minX = Math.max(x - width / 2, -w / 2 + .22), maxX = Math.min(x + width / 2, w / 2 - .22);
    const minZ = Math.max(z - depth / 2, back + .22), maxZ = z + depth / 2;
    // Only the outer plot edges inset. Preserve masonry joins between wings.
    const shell = { x: (minX + maxX) / 2, z: (minZ + maxZ) / 2,
      width: maxX - minX, depth: maxZ - minZ, base, top: base + height };
    shells.push(shell);
    add(role, "plaster", shell.x, base + height / 2, shell.z, shell.width, height, shell.depth, tint);
    add("roof edge", "stone", x, base + height + .12, z, width, .24, depth, "#92978a");
  };
  const frame = (x: number, y: number, z: number, width: number, height: number, colour = steel) => {
    for (const side of [-1, 1]) add("window jamb", "metal", x + side * width / 2, y, z, .055, height + .09, .09, colour);
    for (const side of [-1, 1]) add("window rail", "metal", x, y + side * height / 2, z, width, .055, .09, colour);
    add("window mullion", "metal", x, y, z + .01, .045, height, .09, colour);
  };
  const window = (x: number, y: number, z: number, width: number, height: number, salt: string) => {
    const glass = variation(p.id, salt) > .45 ? "#567076" : "#354f54";
    add("window reveal", "recess", x, y, z - .03, width + .2, height + .2, .015, "#2e403c", "box", false);
    add("window glass", "glass", x, y, z - .025, width, height, .06, glass, "box", false);
    frame(x, y, z + .025, width, height);
    add("window sill", "stone", x, y - height / 2 - .08, z + .055, width + .22, .1, .22);
  };
  const facing = (x: number, z: number, yaw: number, draw: () => void) => {
    const first = parts.length;
    draw();
    const c = Math.cos(yaw), s = Math.sin(yaw);
    for (let i = first; i < parts.length; i++) {
      const part = parts[i], px = part.x, pz = part.z;
      part.x = x + px * c + pz * s; part.z = z - px * s + pz * c;
      part.rotation = [0, yaw, 0];
    }
  };
  const door = (x: number, z: number, width = 2.4) => {
    add("door recess", "recess", x, 1.45, z - .055, width + .34, 2.9, .13, "#24352f", "box", false);
    add("entrance door", luxury ? "wood" : "glass", x, 1.36, z + .03, width, 2.7, .055,
      luxury ? timber : "#486164", "box", false);
    frame(x, 1.4, z + .07, width + .12, 2.8);
    for (const side of [-1, 1]) add("door handle", "metal", x + side * .13, 1.25, z + .14, .035, .46, .07, "#9baba1");
  };
  const entrance = (x: number, z: number, openPassage = false) => {
    if (!openPassage) door(x, z - .48);
    // 2.85 m headroom and an open central axis, including courtyards.
    add("entrance canopy", "stone", x, 3.03, z - .4, 3.65, .22, .9, trim, "bevel");
    for (const side of [-1, 1]) add("entrance pier", "stone", x + side * 1.75, 1.43, z - .42, .22, 2.85, .82);
    if (!openPassage) for (const side of [-1, 1]) add("entrance return", "plaster", x + side * 1.55,
      1.43, face + .35, .17, 2.85, .7, tint);
    add("flush threshold", "stone", x, .015, z - .43, 3.26, .03, .86, "#b6b4a5", "box", false);
  };
  const balcony = (cx: number, width: number, y: number, z = front - .04) => {
    const depth = 1.18;
    add("balcony slab", "stone", cx, y, z - depth / 2, width, .18, depth, trim, "bevel");
    add("balcony handrail", "metal", cx, y + 1.06, z - .055, width, .055, .055, steel);
    add("balcony toe rail", "metal", cx, y + .16, z - .055, width, .055, .055, steel);
    const count = Math.ceil(width / .7);
    for (let i = 0; i <= count; i++) add("balcony baluster", "metal", cx - width / 2 + width * i / count,
      y + .6, z - .055, .032, .91, .04, steel);
    for (const side of [-1, 1]) {
      add("balcony return", "metal", cx + side * (width / 2 - .04), y + 1.06, z - depth / 2, .05, .05, depth, steel);
      add("balcony privacy end", "stone", cx + side * (width / 2 - .07), y + .56, z - depth / 2, .1, .95, depth, trim);
    }
    // Doors sit behind the rail. The balcony replaces the missing 1.3 m of the near shell.
    window(cx, y + 1.44, face + .08, Math.min(2.65, width - .55), 2.45, `balcony-${cx}-${y}`);
  };

  if (courtyard) {
    slab("left courtyard wing", -w * .35, -.65, w * .3, d - 1.3, h);
    slab("right courtyard wing", w * .35, -.65, w * .3, d - 1.3, h);
    slab("rear courtyard wing", 0, -d * .34, w * .4, d * .32, Math.max(6.6, h - 3.3));
    entrance(entry, front - .07, true);
    // The only courtyard door sits on the rear wing, with the approach left open.
    door(entry, -d * .18 + .055);
    for (const side of [-1, 1]) {
      add("courtyard inner pilaster", "stone", side * (w * .2 + .09), h / 2, -d * .06, .16, h, d * .2);
      const levels = p.floors;
      for (let level = 1; level < levels; level++) balcony(side * w * .35, w * .26, level * 3.3);
      window(side * w * .35, 1.5, face + .08, Math.min(2.7, w * .19), 1.85, `court-${side}`);
      const stairX = side * w * .475;
      add("courtyard stairwell", "glass", stairX, h / 2, face + .075, .8, h - .6, .07, "#607b77", "box", false);
      for (let level = 1; level < p.floors; level++)
        add("courtyard stair landing", "metal", stairX, level * 3.3, face + .13, .84, .2, .09, steel);
    }
  } else if (tower) {
    slab("tower podium", 0, -.65, w, d - 1.3, 9.9);
    const cx = (p.variant % 2 ? 1 : -1) * w * .08;
    slab("tower upper shell", cx, -d * .06, w * .68, d * .72, h - 9.9, 9.9);
    slab("stepped crown", w * .05, -d * .09, w * .4, d * .43, 6.6, h);
    entrance(entry, front - .07);
    for (let x = -w / 2 + 2.5; x < w / 2 - 1.4; x += 3.8) {
      if (Math.abs(x - entry) < 3.5) continue;
      window(x, 2.0, face + .08, 2.8, 3.4, `podium-${x}`);
      add("podium fin", "stone", x - 1.56, 4.8, front - .53, .22, 9.6, 1.06);
    }
    for (const y of [4.1, 7.4, 9.83]) add("podium horizontal joint", "stone", 0, y, front - .62, w, .16, 1.12);
    const upperFront = -d * .06 + d * .36;
    add("tower glazing band", "glass", cx, (h + 9.9) / 2, upperFront + .035, w * .66, h - 10.4, .07, "#5a777e", "box", false);
    for (let x = cx - w * .31; x <= cx + w * .31; x += 3.6)
      add("tower vertical fin", "metal", x, (h + 9.9) / 2, upperFront + .14, .16, h - 9.9, .24, "#b3c1ba");
    for (let y = 13.2; y < h; y += 6.6)
      add("tower spandrel", "metal", cx, y, upperFront + .09, w * .68, .48, .16, "#8b9d99");
    for (const side of [-1, 1]) {
      add("podium terrace parapet", "stone", side * (w / 2 - .16), 10.48, -.7, .22, 1.0, d - 1.6);
      add("tower crown blade", "metal", w * .05 + side * w * .18, h + 3.3, -d * .09, .13, 6.6, d * .4, "#b5c1b9");
    }
  } else if (luxury) {
    slab("villa main", -w * .18, -.65, w * .64, d - 1.3, h);
    slab("villa terrace wing", w * .32, -d * .2, w * .36, d * .6, h - 3.3);
    entrance(entry, front - .07);
    const level = h - 3.1, cx = w * .32, cz = -d * .2;
    for (const x of [cx - w * .14, cx + w * .14]) for (const z of [cz - d * .23, cz + d * .23])
      add("pergola post", "wood", x, level + 1.35, z, .14, 2.7, .14, timber, "bevel");
    for (const z of [cz - d * .23, cz + d * .23])
      add("pergola beam", "wood", cx, level + 2.68, z, w * .3, .24, .19, timber, "bevel");
    const laths = Math.ceil(w * .28 / .55);
    for (let i = 0; i <= laths; i++) add("pergola lath", "wood", cx - w * .14 + i * w * .28 / laths,
      level + 2.87, cz, .105, .16, d * .48, timber);
    add("terrace front parapet", "stone", cx, level + .51, d * .1 - .1, w * .35, .85, .17);
    for (const x of [-w * .35, w * .02])
      add("villa roof coping", "stone", x, h + .44, -.65, .17, .62, d - 1.3);
    for (const x of [-w * .34, -w * .08]) {
      window(x, 1.6, face + .08, Math.min(w * .18, 3.3), 2.2, `villa-${x}`);
      if (p.floors > 1) balcony(x, Math.min(w * .2, 4.5), 3.3);
    }
  } else {
    if (p.style === "urban-apartment" || p.style === "waterfront") {
      slab("apartment main", -w * .08, -.65, w * .84, d - 1.3, h);
      slab("apartment return", w * .4, -d * .12, w * .2, d * .76, Math.max(6.6, h - 3.3));
    } else slab("street building", 0, -.65 - forecourt / 2, w, d - 1.3 - forecourt, h);
    entrance(entry, front - .07);
    if (retail || shed) {
      const bayCount = Math.max(2, Math.floor((w - 1.0) / (shed ? 5.7 : 4.4)));
      const bayWidth = (w - .8) / bayCount;
      let kioskPlaced = false;
      for (let i = 0; i < bayCount; i++) {
        const x = -w / 2 + .4 + bayWidth * (i + .5);
        if (Math.abs(x - entry) < bayWidth / 2 + 1.85) continue;
        const shutter = !football && (shed || variation(p.id, `shutter-${i}`) < .34);
        if (shutter) {
          add("rolling shutter", "metal", x, 1.48, face + .09, bayWidth - .2, 2.92, .09, "#858d83");
          for (let y = .22; y < 2.9; y += .26)
            add("shutter corrugation", "metal", x, y, face + .16, bayWidth - .24, .045, .06, "#68736b");
        } else {
          window(x, 1.65, face + .09, bayWidth - .26, 2.85, `shop-${i}`);
          add("shop bulkhead", "stone", x, .22, face + .13, bayWidth - .13, .44, .18, "#9e9f8d");
        }
        for (const side of [-1, 1]) add("shopfront return", "stone", x + side * (bayWidth / 2 - .065), 1.55,
          front - .64, .13, 3.1, 1.16, trim);
        add("awning roof", "canvas", x, 3.21, front - .58, bayWidth - .16, .12, 1.08, awning);
        add("awning valance", "canvas", x, 3.02, front - .1, bayWidth - .16, .27, .07, awning);
        // A rolled edge and slender stays make the canopy a supported fitting.
        add("awning front rail", "metal", x, 3.17, front - .08, bayWidth - .2, .035, .035, steel);
        for (const side of [-1, 1]) {
          add("awning stay", "metal", x + side * (bayWidth / 2 - .18), 2.87,
            face + .58, .035, Math.hypot(1, .66), .035, steel);
          parts[parts.length - 1].rotation = [Math.atan2(1, .66), 0, 0];
        }
        // Generic sign copy identifies scenery, not an additional interactive service.
        add("shop fascia", "wood", x, 3.75, face + .09, bayWidth - .18, .65, .1, timber);
        if (football || (informal || p.style === "corner-shop") && variation(p.id, `fascia-${i}`) < (informal ? .65 : .5)) {
          const services = shed ? ["MECHANIC", "TYRE REPAIRS"] : ["BARBER", "TAILORING", "PROVISIONS", "PRINT & COPY", "HARDWARE"];
          sign(football?.name ?? services[Math.floor(variation(p.id, `service-${i}`) * services.length)], x, 3.75, face + .165,
            bayWidth - .4, .49);
        }
        if (football) {
          // Exterior display stays inside the shop recess, away from the entrance.
          const displayZ = front - .59;
          add("sports display plinth", "stone", x, .43, displayZ, 1.38, .86, .48, "#b6b19e", "bevel");
          for (const side of [-1, 1]) {
            const bx = x + side * .36;
            add("display football", "recess", bx, 1.01, displayZ, .3, .3, .3, "#eee8d5", "ball");
            add("football panel", "recess", bx, 1.03, displayZ + .139, .095, .095, .028, "#34413a", "ball", false);
            add("football upper panel", "recess", bx, 1.15, displayZ + .065, .075, .036, .065, "#34413a", "ball", false);
          }
          add("kit display backboard", "wood", x, 2.05, face + .2, 1.45, 1.24, .075, "#dbcdb1");
          add("display jersey body", "canvas", x, 2.01, face + .255, .48, .67, .04, football.kit);
          add("display jersey sleeves", "canvas", x, 2.26, face + .256, .82, .23, .045, football.kit);
          add("jersey collar", "canvas", x, 2.39, face + .28, .18, .065, .03, "#e7e1cb", "box", false);
          add("jersey stripe", "canvas", x, 2.04, face + .282, .075, .52, .015, "#e7e1cb", "box", false);
        }
        if (informal && !shed && !kioskPlaced && variation(p.id, "kiosk") < .24 + tradePressure * .48) {
          kioskPlaced = true;
          const z = front - .62, shade = ["#b49c44", "#558369", "#a86250"][p.variant % 3];
          add("stall umbrella", "canvas", x, 2.48, z, 1.18, .32, 1.18, shade, "umbrella");
          add("umbrella pole", "metal", x, 1.18, z, .035, 2.36, .035, steel, "pipe");
          add("kiosk tabletop", "wood", x, .81, z, .92, .08, .53, timber, "bevel");
          for (const side of [-1, 1]) for (const depth of [-1, 1])
            add("kiosk leg", "wood", x + side * .38, .38, z + depth * .2, .05, .76, .05, timber);
          if (variation(p.id, "kiosk-type") < .5) {
            sign("POS", x, 1.04, z + .29, .57, .3);
            const cx = x + .88;
            add("plastic chair seat", "recess", cx, .43, z, .43, .055, .4, "#657a65");
            add("plastic chair back", "recess", cx, .67, z - .18, .43, .43, .045, "#657a65");
            for (const side of [-1, 1]) for (const depth of [-1, 1])
              add("plastic chair leg", "recess", cx + side * .17, .2, z + depth * .15, .045, .4, .045, "#657a65");
          } else {
            add("display case backing", "glass", x, 1.04, z - .2, .82, .38, .025, "#638080", "box", false);
            for (const side of [-1, 1]) add("display case side", "glass", x + side * .4, 1.04, z,
              .025, .38, .42, "#638080", "box", false);
            for (const side of [-1, 1]) add("display case upright", "metal", x + side * .39, 1.04, z + .2,
              .025, .4, .025, steel);
            add("display case top", "metal", x, 1.25, z, .86, .025, .46, steel);
            for (let item = 0; item < 4; item++) add("snack stock", "wood", x - .26 + item * .17, .89,
              z + .15, .12, .07, .1, "#b18a4e", "bevel", false);
          }
        }
        if (shed) add("warehouse lintel", "metal", x, 3.02, face + .24, bayWidth, .24, .35, steel);
      }
      if (shed) for (const x of [-w * .37, w * .37]) window(x, h - .86, face + .07, 2.8, .86, `shed-${x}`);
      else for (let level = 1; level < p.floors; level++) for (let i = 0; i < bayCount; i++) {
        const x = -w / 2 + .4 + bayWidth * (i + .5);
        window(x, level * 3.3 + 1.52, face + .08, Math.min(2.5, bayWidth - .65), 1.95, `shop-upper-${i}-${level}`);
      }
    } else {
      const mainWidth = p.style === "urban-apartment" || p.style === "waterfront" ? w * .84 : w;
      const centre = mainWidth === w ? 0 : -w * .08;
      const slots = Math.max(2, Math.floor(mainWidth / 5.7));
      const gap = mainWidth / slots;
      for (let i = 0; i < slots; i++) {
        const x = centre - mainWidth / 2 + gap * (i + .5);
        if (Math.abs(x - entry) > 3.1) window(x, 1.56, face + .08, Math.min(2.5, gap - .7), 1.95, `ground-${i}`);
        for (let level = 1; level < p.floors; level++) {
          if (p.style === "garden-home" && i % 2 === 1) {
            window(x, level * 3.3 + 1.5, face + .08, 1.9, 1.9, `garden-${i}-${level}`);
            for (const side of [-1, 1]) add("timber shutter", "wood", x + side * 1.19, level * 3.3 + 1.5,
              face + .18, .37, 2.03, .14, timber);
          } else balcony(x, Math.min(gap - .38, p.style === "waterfront" ? 5.8 : 4.6), level * 3.3);
        }
      }
      const stairX = centre - mainWidth / 2 + .72;
      const stairHeight = h - .9;
      add("stairwell recess", "recess", stairX, stairHeight / 2 + .45, face + .015, .94, stairHeight, .06, "#30443d", "box", false);
      add("stairwell glazing", "glass", stairX, stairHeight / 2 + .45, face + .08, .78, stairHeight, .06, "#607b77", "box", false);
      for (let level = 1; level < p.floors; level++)
        add("stair landing band", "metal", stairX, level * 3.3, face + .13, .84, .2, .09, steel);
    }
  }

  // Detail actual exposed wings, including side and rear elevations. Overlapping
  // wings do not receive hidden windows or fittings inside their shared walls.
  for (const shell of shells) {
    const concealed = (x: number, z: number, y: number) => shells.some(other => other !== shell
      && y > other.base && y < other.top
      && Math.abs(x - other.x) < other.width / 2 && Math.abs(z - other.z) < other.depth / 2);
    const facades = [
      { x: shell.x - shell.width / 2, z: shell.z, yaw: -Math.PI / 2, span: shell.depth },
      { x: shell.x + shell.width / 2, z: shell.z, yaw: Math.PI / 2, span: shell.depth },
      { x: shell.x, z: shell.z - shell.depth / 2, yaw: Math.PI, span: shell.width },
    ];
    for (const facade of facades) {
      const columns = Math.max(1, Math.floor((facade.span - 1.6) / (tower ? 6.8 : 5.6)));
      const spacing = (facade.span - 1.6) / columns;
      for (let column = 0; column < columns; column++) {
        const x = -.5 * (facade.span - 1.6) + (column + .5) * spacing;
        const width = Math.min(shed ? 2.6 : luxury ? 2.4 : 1.9, spacing - .55);
        const px = facade.x + x * Math.cos(facade.yaw) + .19 * Math.sin(facade.yaw);
        const pz = facade.z - x * Math.sin(facade.yaw) + .19 * Math.cos(facade.yaw);
        for (let floor = Math.ceil(shell.base / 3.3); floor * 3.3 + 2.6 < shell.top; floor++) {
          const y = shed ? shell.top - 1.05 : floor * 3.3 + 1.7;
          if (width < .8 || [-width / 2 - .15, 0, width / 2 + .15].some(offset =>
            concealed(px + offset * Math.cos(facade.yaw), pz - offset * Math.sin(facade.yaw), y))) continue;
          facing(facade.x, facade.z, facade.yaw, () => {
            window(x, y, .025, width, shed ? .74 : luxury ? 2.05 : 1.62,
              `side-${facade.yaw}-${shell.x}-${shell.z}-${column}-${floor}`);
            if (!maintained && !shed && floor === 0 && variation(p.id, "security-bars") < .37) {
              for (let bar = -width * .4; bar < width * .45; bar += .32)
                add("ground window security bar", "metal", x + bar, y, .15, .025, 1.62, .025, steel);
              add("security grille rail", "metal", x, y, .15, width, .026, .025, steel);
            }
          });
          if (shed) break;
        }
        const middle = (shell.base + shell.top) / 2;
        if (!concealed(px, pz, middle)) facing(facade.x, facade.z, facade.yaw, () => {
          if (maintained) {
            add("facade vertical joint", "recess", x - spacing * .42, middle, .012,
              .025, shell.top - shell.base - .32, .025, "#858e80", "box", false);
          } else if (variation(p.id, `side-finish-${column}-${facade.yaw}`) < .3) {
            add("facade rain wash", "plaster", x + width * .53, shell.top - .8, .013,
              .12, 1.35, .024, p.district === "south-market" ? "#a08d73" : "#b3b19f", "box", false);
          }
          if (!pitched) {
            add("parapet shadow line", "recess", x, shell.top - .16, .018,
              spacing - .08, .055, .035, "#687166", "box", false);
            add("parapet cap return", "stone", x, shell.top + .19, .055,
              spacing - .06, .13, .15, "#b3b5a6");
          }
        });
      }
    }
  }
  // Floor joints stay on the occupied front wings, not across courtyard openings.
  for (const shell of shells.filter(shell => Math.abs(shell.z + shell.depth / 2 - face) < .01)) {
    for (let level = 1; level * 3.3 < shell.top; level++) {
      add("front floor shadow joint", "recess", shell.x, level * 3.3 - .12, face + .016,
        shell.width, .045, .03, "#747d70", "box", false);
    }
    if (pitched) {
      add("eaves gutter", "metal", shell.x, h + .02, front - .28,
        shell.width, .13, .14, steel);
      add("gutter downpipe connector", "metal", -w * .46, h - .06, front - .73,
        .11, .12, .97, steel);
    }
  }
  if (football) {
    // Mounted roof board and sports-colour pennants identify the neighbourhood.
    // Match entry remains through the existing Travel destination.
    sign("FOOTBALL PITCH\nNEARBY", 0, h + .94, face + .11, Math.min(w - 2, 5), 1.05);
    add("football sign backing", "metal", 0, h + .94, face + .07, Math.min(w - 1.8, 5.2), 1.22, .06, "#36584a");
    for (const side of [-1, 1]) {
      add("football board support", "metal", side * 1.85, h + .63, face + .055, .055, 1.66, .055, steel);
      const x = side * w * .4;
      add("sports pennant support", "metal", x, 4.94, face + .22, .045, 2.4, .045, steel);
      add("sports pennant bracket", "metal", x, 6.02, face + .3, .55, .045, .22, steel);
      add("sports pennant", "canvas", x, 5.24, face + .35, .5, 1.45, .045, football.kit);
      add("pennant stripe", "canvas", x, 5.24, face + .38, .065, 1.45, .015, "#e7e1cb", "box", false);
    }
  }

  if (pitched) add("pitched roof", "roof", 0, h + 1.6, -.1 - forecourt / 2, w - .08, 3.0, d - .2 - forecourt,
    shed ? "#697b7c" : p.variant % 2 ? "#8b5e45" : "#777669", "gable");
  // Continuous plinths and ground seams are fitted to occupied wings by the infill recipe below.
  const drainXs = courtyard ? [-w * .48, w * .48] : [-w * .46];
  for (const x of drainXs) {
    add("downpipe", "metal", x, h / 2, face + .13, .105, h, .105, steel, "pipe");
    for (let y = .6; y < h; y += 3.3) add("downpipe collar", "metal", x, y, face + .13, .17, .045, .17, "#8c9585");
    add("downpipe shoe", "metal", x, .12, face + .22, .15, .2, .3, steel, "bevel");
  }
  const frontageBoundary = companionCityFrontageBoundaryParts(p);
  if (frontageBoundary) parts.push(...frontageBoundary);
  else if (p.garden > 0) {
    const outer = w / 2 + p.garden;
    for (const side of [-1, 1]) {
      add("compound side wall", "stone", side * outer, .72, 0, .24, 1.44, d, "#b9b5a3");
      const start = side < 0 ? -outer : entry + 2.3;
      const end = side < 0 ? entry - 2.3 : outer;
      if (end > start) {
        add("compound front wall", "stone", (start + end) / 2, .72, front - .14, end - start, 1.44, .24, "#b9b5a3");
        add("wall coping", "stone", (start + end) / 2, 1.49, front - .14, end - start, .12, .27);
        if (!maintained && end - start > 5 && variation(p.id, `wall-sign-${side}`) < .16) {
          const message = variation(p.id, "wall-message") < .6 ? "POST NO BILLS" : "THIS LAND IS NOT FOR SALE\nBEWARE OF 419";
          sign(message, (start + end) / 2, .91, front - .004, Math.min(3.7, end - start - .7), .52);
        }
      }
      add("gate pier", "stone", entry + side * 2.25, .88, front - .21, .26, 1.76, .4, trim, "bevel");
      // The gate leaves are folded inward beside the opening, never across its axis.
      add("open gate frame", "metal", entry + side * 2.38, .77, front - 1.19, .065, 1.5, 1.88, steel);
      for (let z = front - 2.05; z < front - .3; z += .24)
        add("open gate bar", "metal", entry + side * 2.4, .78, z, .055, 1.4, .035, steel);
    }
    add("compound rear wall", "stone", 0, .72, back + .13, w + p.garden * 2, 1.44, .24, "#b9b5a3");
  }
  if (!luxury) {
    // Utilities sit on an occupied wing. Pitched buildings use a roof vent rather than a floating tank.
    const utilityX = courtyard ? -w * .35 : tower ? w * .05 : -w * .18;
    const utilityZ = courtyard ? -d * .12 : tower ? -d * .09 : pitched ? -.1 - forecourt / 2 : -d * .19;
    const roofY = tower ? h + 6.84 : pitched ? h + 2.7 : h + .24;
    const raisedTank = !pitched && !maintained && variation(p.id, "tank-frame") < .38;
    const tankBase = roofY + (raisedTank ? 1.2 : 0);
    add(pitched ? "roof vent" : "water tank", pitched ? "metal" : "recess", utilityX, tankBase + (pitched ? .43 : .67), utilityZ,
      pitched ? .65 : 1.5, pitched ? .86 : 1.34, pitched ? .65 : 1.5,
      pitched || maintained ? "#4d5a50" : "#292e2b", "pipe");
    add("roof utility base", "stone", utilityX, roofY + .09, utilityZ, pitched ? .75 : 1.7, .18,
      pitched ? .75 : 1.7, "#b1b3a1");
    if (!pitched && !maintained) {
      add("water tank lid", "recess", utilityX, tankBase + 1.38, utilityZ, .64, .1, .64, "#242926", "pipe");
      for (let level = .25; level < 1.2; level += .29)
        add("water tank rib", "recess", utilityX, tankBase + level, utilityZ, 1.54, .035, 1.54, "#363c36", "pipe");
    }
    if (raisedTank) {
      for (const side of [-1, 1]) for (const depth of [-1, 1])
        add("tank stand leg", "metal", utilityX + side * .65, roofY + .6, utilityZ + depth * .65, .065, 1.2, .065, steel);
      add("tank stand platform", "metal", utilityX, tankBase, utilityZ, 1.7, .09, 1.7, steel);
      for (const side of [-1, 1]) add("tank stand cross rail", "metal", utilityX, roofY + .42,
        utilityZ + side * .65, 1.36, .06, .06, steel);
    }
    const utilityLimit = courtyard ? -w * .2 - .7 : tower ? w * .25 - .7 : w * .25;
    if (!pitched) for (let x = utilityX + 2.1; x < Math.min(utilityLimit, utilityX + 6); x += 1.7) {
      add("roof condenser", "metal", x, roofY + .43, utilityZ, 1.2, .7, .85, "#afb7ab", "bevel");
      for (let y = roofY + .2; y < roofY + .69; y += .12)
        add("condenser grille", "metal", x, y, utilityZ + .44, 1.02, .034, .025, steel);
    }
  }
  const exposedFront = courtyard || p.style === "townhouse" || p.style === "garden-home" || informal;
  if (!maintained && exposedFront && p.floors > 1 && variation(p.id, "window-ac") < .58) {
    const x = courtyard ? -w * .35 : w * .35, y = 4.25;
    add("window AC housing", "metal", x, y, face + .3, .84, .53, .49, "#b5b2a2", "bevel");
    for (let slot = 0; slot < 5; slot++) add("window AC grille", "recess", x, y - .18 + slot * .085,
      face + .56, .72, .026, .015, "#555c51", "box", false);
    add("AC runoff stain", "plaster", x + .23, 2.9, face + .006, .13, 2.05, .012, "#9a9984", "box", false);
    add("AC drain hose", "recess", x + .4, 2.08, face + .52, .028, 3.62, .028, "#d5d1bb", "pipe", false);
    add("AC drain outlet", "recess", x + .4, .27, face + .57, .035, .04, .15, "#c6c5b0", "box", false);
    add("AC damp patch", "groundSeam", x + .4, -.003, face + .62, .41, .018, .45, "#74745b", "box", false);
  }
  if (!maintained && exposedFront && variation(p.id, "generator") < (informal ? .3 : .14)) {
    const x = courtyard ? -w * .35 : w * .38, z = front - .52;
    add("generator base", "stone", x, .04, z, .91, .08, .75, "#858979");
    add("generator housing", "metal", x, .35, z, .7, .55, .51, "#596b56", "bevel");
    add("generator fuel tank", "metal", x, .63, z, .5, .12, .38, "#a39453", "bevel");
    for (const side of [-1, 1]) for (const depth of [-1, 1])
      add("generator cage upright", "metal", x + side * .41, .41, z + depth * .33, .035, .74, .035, steel);
    for (const depth of [-1, 1]) for (const y of [.12, .75])
      add("generator cage rail", "metal", x, y, z + depth * .33, .85, .035, .035, steel);
    for (let grille = -.27; grille < .3; grille += .09)
      add("generator vent", "recess", x + grille, .34, z + .263, .035, .31, .015, "#30392f", "box", false);
    add("generator cage latch", "metal", x + .14, .49, z + .35, .12, .04, .035, "#898d79");
    add("generator padlock", "metal", x + .14, .44, z + .38, .08, .1, .05, "#a49967", "bevel", false);
  }
  parts.push(...companionCityParkingParts(p));
  parts.push(...companionCitySpatialInfill(p, parts));
  // This check describes geometry admission, not a separate movement model. The existing
  // obstacle reserves the full plot. A changed plan cannot silently spill detail onto a street.
  const limitX = w / 2 + p.garden + .15;
  return parts.filter(part => {
    if (part.w <= 0 || part.h <= 0 || part.d <= 0) return false;
    if (!part.rotation) return Math.abs(part.x) + part.w / 2 <= limitX + 1e-7
      && Math.abs(part.z) + part.d / 2 <= d / 2 + 1e-7;
    const [pitch, yaw, roll] = part.rotation;
    const cx = Math.cos(pitch), sx = Math.sin(pitch), cy = Math.cos(yaw), sy = Math.sin(yaw);
    const cz = Math.cos(roll), sz = Math.sin(roll);
    // Absolute X/Z rows of the renderer's YXZ rotation bound the entire rotated part.
    const width = Math.abs(cy * cz + sy * sx * sz) * part.w
      + Math.abs(-cy * sz + sy * sx * cz) * part.h + Math.abs(sy * cx) * part.d;
    const depth = Math.abs(-sy * cz + cy * sx * sz) * part.w
      + Math.abs(sy * sz + cy * sx * cz) * part.h + Math.abs(cy * cx) * part.d;
    return Math.abs(part.x) + width / 2 <= limitX + 1e-7
      && Math.abs(part.z) + depth / 2 <= d / 2 + 1e-7;
  });
}

/** No additional obstacles are required. No props extend outside admitted parcel footprints. */
export function companionCityArchitectureAdditionalFootprints(): readonly never[] { return []; }

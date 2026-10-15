import type { CityStreetFixture, CityStreetWire } from "./companionCityStreetDetails";

export type CityUtilityPoleProfile = {
  pitch: number; roll: number; weathered: boolean; meters: number; fibreLoops: number;
};
export type CityUtilityPolePart = {
  geometry: "box" | "pole" | "loop" | "cable";
  x: number; y: number; z: number; w: number; h: number; d: number;
  yaw: number; pitch: number; roll: number; colour: string;
};
type Point = [number, number, number];

/** Stable, district-led wear. The lean is fixed scenery, not an oscillating wind animation. */
export function cityUtilityPoleProfile(seed: number, setting: "dense" | "mixed" | "tidy"): CityUtilityPoleProfile {
  const dense = setting === "dense", tidy = setting === "tidy";
  const leaning = seed % 100 < (dense ? 62 : tidy ? 12 : 32);
  const angle = leaning ? (dense ? .035 : tidy ? .01 : .022) + seed % 7 * (dense ? .005 : .002) : 0;
  const direction = (seed % 359) * Math.PI / 180;
  return { pitch: Math.cos(direction) * angle, roll: Math.sin(direction) * angle,
    weathered: dense || !tidy && seed % 3 === 0,
    meters: dense ? 5 + seed % 4 : tidy ? 1 + seed % 2 : 2 + seed % 3,
    fibreLoops: dense ? 3 + seed % 3 : 1 + seed % 2 };
}

/** Same YXZ transform for the shaft, attachments, lamp and every cable endpoint. */
export function cityUtilityPolePoint(fixture: CityStreetFixture, x: number, y: number, z: number): Point {
  const pitch = fixture.utility?.pitch ?? 0, roll = fixture.utility?.roll ?? 0;
  const cr = Math.cos(roll), sr = Math.sin(roll), cp = Math.cos(pitch), sp = Math.sin(pitch);
  const rx = x * cr - y * sr, ry = x * sr + y * cr;
  const py = ry * cp - z * sp, pz = ry * sp + z * cp;
  const c = Math.cos(fixture.yaw), s = Math.sin(fixture.yaw);
  return [fixture.x + rx * c + pz * s, py, fixture.z - rx * s + pz * c];
}

export function cityUtilityPolePorts(fixture: CityStreetFixture) {
  const at = (x: number, y: number, z: number) => cityUtilityPolePoint(fixture, x, y, z);
  return {
    // The crossarm runs across the road, perpendicular to the span direction.
    primary: [-.9, 0, .9].map(z => at(0, fixture.height - .44, z)),
    phases: [0, 1, 2, 3].map(index => at(.29, fixture.height - 2 - index * .4, -.18)),
    fibre: at(-.18, fixture.height - 4.4, .22),
  };
}

/** Eight distinct conductors. These are not the old three coincident decorative strands. */
export function cityUtilityPoleSpan(a: CityStreetFixture, b: CityStreetFixture, id: string): CityStreetWire[] {
  const from = cityUtilityPolePorts(a), to = cityUtilityPolePorts(b), distance = Math.hypot(b.x - a.x, b.z - a.z);
  const wires: CityStreetWire[] = [];
  for (let i = 0; i < 3; i++) wires.push({ id: `${id}:primary:${i}`, roadId: a.roadId,
    from: from.primary[i], to: to.primary[i], sag: .42 + distance * .006, strands: 1, diameter: .024 });
  for (let i = 0; i < 4; i++) wires.push({ id: `${id}:phase:${i}`, roadId: a.roadId,
    from: from.phases[i], to: to.phases[i], sag: .32 + distance * .005, strands: 1, diameter: .019 });
  wires.push({ id: `${id}:fibre`, roadId: a.roadId, from: from.fibre, to: to.fibre,
    sag: Math.min(.65, .3 + distance * .004), strands: 1, diameter: .038 });
  return wires;
}

/** Reference-led multi-service concrete pole, kept in the existing instanced street batches. */
export function cityUtilityPoleParts(fixture: CityStreetFixture): CityUtilityPolePart[] {
  const profile = fixture.utility;
  if (!profile) return [];
  const parts: CityUtilityPolePart[] = [], height = fixture.height;
  const metal = profile.weathered ? "#756b55" : "#64716b", concrete = profile.weathered ? "#a29c85" : "#b2b4a7";
  const add = (geometry: CityUtilityPolePart["geometry"], x: number, y: number, z: number,
    w: number, h: number, d: number, colour: string) => {
    const point = cityUtilityPolePoint(fixture, x, y, z);
    parts.push({ geometry, x: point[0], y: point[1], z: point[2], w, h, d, colour,
      yaw: fixture.yaw, pitch: profile.pitch, roll: profile.roll });
  };
  const cable = (a: Point, b: Point, diameter = .024, colour = "#292e2b") => {
    const from = cityUtilityPolePoint(fixture, ...a), to = cityUtilityPolePoint(fixture, ...b);
    const dx = to[0] - from[0], dy = to[1] - from[1], dz = to[2] - from[2];
    parts.push({ geometry: "cable", x: (from[0] + to[0]) / 2, y: (from[1] + to[1]) / 2,
      z: (from[2] + to[2]) / 2, w: diameter, h: Math.hypot(dx, dy, dz), d: diameter,
      yaw: Math.atan2(dx, dz), pitch: Math.atan2(Math.hypot(dx, dz), dy), roll: 0, colour });
  };
  const loop = (x: number, y: number, z: number, scale: number) =>
    add("loop", x, y, z, scale, scale * 1.18, .16, "#292e2b");
  add("pole", 0, height / 2, 0, .3, height, .3, concrete);
  // The footing stays level, even when the column leans.
  parts.push({ geometry: "box", x: fixture.x, y: .18, z: fixture.z, w: .38, h: .36, d: .38,
    yaw: fixture.yaw, pitch: 0, roll: 0, colour: "#a7a38e" });
  add("box", 0, height - .7, 0, .14, .14, 2.22, metal);
  for (const z of [-.9, 0, .9]) {
    add("pole", 0, height - .48, z, .17, .29, .17, "#787464");
    for (const y of [-.06, .035, .13]) add("pole", 0, height - .54 + y, z, .24, .045, .24, "#bab9a4");
  }
  for (const side of [-1, 1]) cable([0, height - 1.5, 0], [0, height - .76, side * .78], .055, metal);
  // Four lower insulators form the vertical low-voltage rack.
  add("box", .18, height - 2.6, -.18, .065, 1.65, .07, metal);
  for (let index = 0; index < 4; index++) {
    const y = height - 2 - index * .4;
    add("box", .22, y, -.18, .3, .045, .055, metal);
    add("pole", .29, y, -.18, .15, .21, .15, "#b8b8a4");
    cable([.29, y, -.18], [.18, y - .2, .03], .023);
  }
  // Telecom closure, slack loops and straps sit below the power conductors.
  add("box", -.18, height - 4.4, .2, .24, .56, .22, "#343d38");
  for (let i = 0; i < profile.fibreLoops; i++) loop(-.17 + (i % 2) * .22, height - 4.8 - i * .1, .3 + i * .025, .48 + i * .065);
  for (const y of [height - 4.65, height - 4.15, 2.7, 3.9]) add("box", 0, y, .07, .37, .055, .32, metal);
  // Meter clusters remain above pedestrian headroom on existing roadside poles.
  for (let i = 0; i < profile.meters; i++) {
    const x = profile.meters === 1 ? 0 : (i % 2 ? 1 : -1) * .235;
    const y = 2.73 + Math.floor(i / 2) * .62 + (i % 2) * .08;
    add("box", x / 2, y, .19, .58, .075, .06, metal);
    add("box", x, y, .3, .36, .52, .19, profile.weathered ? i % 2 ? "#b0af9c" : "#8f978b" : "#bbc1b8");
    add("box", x, y + .11, .401, .23, .17, .018, "#354b48");
    add("box", x, y - .13, .405, .18, .11, .022, "#777f72");
    add("box", x + .13, y + .02, .402, .025, .075, .025, "#9c784c");
    const offset = (i % 2 ? 1 : -1) * (.25 + i * .026);
    cable([.2, height - 3.4, .04], [offset, y + .37, .25], .025);
    cable([offset, y + .37, .25], [x, y + .26, .34], .022);
    cable([x, y - .25, .33], [offset, y - .4, .21], .024);
    if (profile.weathered || i === 0) loop(offset, y - .22, .27, .29 + i * .016);
  }
  if (profile.weathered) for (const y of [1.1, 2.1, 4.9])
    add("box", .01, y, .145, .17, .21 + y * .02, .009, y < 2 ? "#777b64" : "#b9b39c");
  add("box", 0, .8, -.13, .09, .28, .018, "#747c6e");
  return parts;
}

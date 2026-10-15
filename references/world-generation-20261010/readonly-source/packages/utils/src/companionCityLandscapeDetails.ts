import plan from "../../assets/city/lagos-city-v3.json";

export type CityLandscapeAmenity = typeof plan.amenities[number];
export type CityLandscapeSpace = typeof plan.greenSpaces[number];
export type CityLandscapeTreatment = {
  kind: "forest" | "heritage" | "palm" | "shore" | "civic" | "market" | "flower" | "grove" | "common";
  ground: string;
  leaf: string;
  timber: string;
  flower: string;
  seed: number;
};

/** Stable art variation. No random state, navigation changes or filesystem work. */
export function companionCityLandscapeSeed(id: string): number {
  let seed = 2166136261;
  for (let i = 0; i < id.length; i++) seed = Math.imul(seed ^ id.charCodeAt(i), 16777619);
  return seed >>> 0;
}

const finishes: Record<CityLandscapeTreatment["kind"], readonly [string, string, string, string]> = {
  forest: ["#6e7655", "#41613e", "#827254", "#c5bda2"],
  heritage: ["#829068", "#627c4d", "#88704f", "#bc8f9c"],
  palm: ["#8b9365", "#647e43", "#927958", "#d4bf75"],
  shore: ["#8b9774", "#5c7955", "#98815e", "#ddd3ad"],
  civic: ["#c0b9a5", "#6c8259", "#877459", "#d0c49a"],
  market: ["#b3a083", "#647b50", "#896a4b", "#c29681"],
  flower: ["#879966", "#637e49", "#9a8263", "#bb8f9c"],
  grove: ["#7d8a59", "#4e6c43", "#7f6b4f", "#c5b48a"],
  common: ["#92976b", "#708153", "#8c7759", "#d6c587"],
};

const namedTreatments: Record<string, CityLandscapeTreatment["kind"]> = {
  "palm-commons": "heritage", "botanic-gardens": "forest", "civic-square": "civic",
  "market-court": "market", "marina-green": "shore", "skyline-garden": "civic",
  "lagoon-gardens": "flower", "west-grove": "grove", "college-green": "common", "southern-common": "palm",
  "beach:south": "shore", "beach:north": "palm",
};
const neighbourhoodTreatments = ["grove", "flower", "common", "palm", "shore"] as const;

export function companionCityLandscapeTreatment(id: string): CityLandscapeTreatment {
  const seed = companionCityLandscapeSeed(id);
  const parkNumber = Number(id.replace("neighbourhood-park-", ""));
  const kind = namedTreatments[id] ?? neighbourhoodTreatments[
    Number.isFinite(parkNumber) ? (parkNumber - 1 + neighbourhoodTreatments.length) % neighbourhoodTreatments.length : seed % neighbourhoodTreatments.length
  ];
  const [ground, leaf, timber, flower] = finishes[kind];
  return { kind, ground, leaf, timber, flower, seed };
}

/** Existing authoritative reserves only. Consumers must not append these again. */
export function companionCityLandscapeReservedFootprint(a: CityLandscapeAmenity) {
  const w = a.kind === "market-canopy" ? 8 : a.kind === "fountain" ? 5 : a.kind === "clock-tower" ? 2
    : a.kind === "beach-umbrella" || a.kind === "flower-bed" ? 3 : a.kind === "bench" ? 1.3 : .4;
  const d = a.kind === "bench" ? .55 : a.kind === "flower-bed" ? 2 : w;
  return { minX: a.x - w, maxX: a.x + w, minZ: a.z - d, maxZ: a.z + d, yaw: a.yaw };
}

/** Admit these before attaching cable-anchor sleeves. The base cable fans stay unchanged. */
export function companionCityLandscapeAdditionalFootprints() {
  return plan.landmarkBeams.filter(b => b.id === "link-stay").map((b, index) => ({
    id: "landscape-link-anchor-" + index,
    minX: b.to[0] - .16, maxX: b.to[0] + .16,
    minZ: b.to[2] - .3, maxZ: b.to[2] + .3,
    yaw: 0,
  }));
}

export function companionCityLandscapeCoverage() {
  return plan.greenSpaces.map(space => ({
    id: space.id,
    treatment: companionCityLandscapeTreatment(space.id).kind,
    amenities: plan.amenities.filter(a => a.id.startsWith(space.id + "-")).length,
    paths: plan.roads.filter(r => r.name.startsWith(space.name + " ") && ["path", "sand-path", "boardwalk"].includes(r.kind)).length,
  }));
}

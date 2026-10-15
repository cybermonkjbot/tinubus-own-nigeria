import type { CompanionPlaceFixture, CompanionPlaceFixtureKind, CompanionPlaceGeneration, CompanionPlaceLayout,
  CompanionPlaceNpc, CompanionPlaceService, CompanionPlaceServiceKind } from "./companionPlaces";

type Builder = {
  layout: CompanionPlaceLayout;
  add(kind: CompanionPlaceFixtureKind, id: string, x: number, z: number, width: number, depth: number, height: number,
    material?: CompanionPlaceFixture["material"], collides?: boolean, y?: number): void;
  service(kind: CompanionPlaceServiceKind, id: string, label: string, x: number, z: number, npcX: number, npcZ: number,
    role: CompanionPlaceNpc["role"], offers?: readonly string[], capacity?: number): CompanionPlaceService;
  zones: CompanionPlaceLayout["zones"][number][];
  fixtures: CompanionPlaceFixture[];
  finish(): CompanionPlaceLayout;
};
type BuilderFactory = (input: CompanionPlaceGeneration, width: number, depth: number, openAir?: boolean) => Builder;

function zone(b: Builder, id: string, label: string, x: number, z: number, w: number, d: number, activity?: "gather") {
  b.zones.push({ id, label, bounds: { minX: x - w / 2, maxX: x + w / 2, minZ: z - d / 2, maxZ: z + d / 2 },
    access: "place-membership", activity });
}
function counter(b: Builder, id: string, label: string, x: number, z: number, kind: CompanionPlaceServiceKind, role: CompanionPlaceNpc["role"]) {
  b.add("counter", `${id}-counter`, x, z, 2.4, .8, .95, "wood");
  b.service(kind, id, label, x, z + 1.2, x, z - 1.2, role);
}
function ride(b: Builder) {
  b.service("driver", "driver", "Board a ride", -2.4, b.layout.navigation.bounds.maxZ - 2.5, -3.4,
    b.layout.navigation.bounds.maxZ - 2.5, "driver");
}
function mealTable(b: Builder, id: string, x: number, z: number) {
  b.add("table", `${id}-table`, x, z, 2, 1, .76, "wood");
  b.add("bench", `${id}-bench`, x, z - 1.1, 2, .5, .48, "wood");
  b.service("food", id, "Eat", x, z + 1.3, x, z, "resident", ["eat-carried-food"], 3);
}
function gap(b: Builder, message: string) {
  b.layout.qualityGaps = [...b.layout.qualityGaps, message];
}

/** Uses the existing builder's seed reflection, collision admission, NPC art,
 * asset hashing and cleanup contracts. All corridors and queues stay at floorY.
 * New transactions and activity effects remain with their current owners.
 */
export function generateCompanionPlaceExpansionLayout(input: CompanionPlaceGeneration, create: BuilderFactory): CompanionPlaceLayout {
  switch (input.kind) {
    case "market": {
      const b = create(input, 28, 16, true);
      b.layout.theme.floor = "#b7aa92";
      for (const [i, x] of [-10, -5, 5, 10].entries()) {
        b.add("market-stall", `stall-${i}`, x, -4.5, 3.1, 1.7, 2.7, i % 2 ? "sage" : "rose");
      }
      counter(b, "market-contact", "Market deliveries", -8, 1.1, "mission", "mission-contact");
      counter(b, "market-pos", "POS", -4.5, 1.1, "pos", "vendor");
      mealTable(b, "market-meal", 7, 1.1);
      zone(b, "market-aisle", "Market aisle", 0, -1.5, 3, 10);
      ride(b);
      gap(b, "Stall goods are scenery, not owned stock. Market purchases and delivery offers need their existing service owners.");
      return b.finish();
    }
    case "supermarket": {
      const b = create(input, 18, 24);
      b.layout.theme.floor = "#ddd9cd";
      for (const x of [-6, 6]) for (const [i, z] of [-7.5, -3, 1.5].entries())
        b.add("shelf", `grocery-shelf-${x}-${i}`, x, z, 3.8, .8, 1.8, "wood");
      counter(b, "collection", "Order collection", -5, 6, "mission", "mission-contact");
      b.add("counter", "packing-counter", 5, 6, 2.8, .8, .95, "stone");
      zone(b, "shopping-aisle", "Shopping aisle", 0, -3, 3, 16);
      ride(b);
      gap(b, "Grocery shelving is scenery. Finite household orders, payment and stock must use the food owner; collection is not a stock grant.");
      return b.finish();
    }
    case "bus-terminal": {
      const b = create(input, 20, 22, true);
      b.layout.theme.floor = "#b8b7ad";
      for (const x of [-6, 6]) {
        b.add("canopy", `shelter-${x}`, x, -4.5, 5, 6, 3.1, "metal", false);
        for (const dx of [-2.35, 2.35]) for (const z of [-7.25, -1.75])
          b.add("cover", `shelter-post-${x}-${dx}-${z}`, x + dx, z, .12, .12, 3.1, "metal");
        b.add("bench", `waiting-bench-${x}`, x, -5, 3.2, .65, .5, "wood");
      }
      b.service("driver", "terminal-driver", "Choose transport", -5, 1, -6.2, 1, "driver");
      mealTable(b, "terminal-meal", 5, 2);
      zone(b, "boarding", "Boarding concourse", 0, -1, 3, 14);
      gap(b, "Transport quotes, vehicle seats, boarding and arrival stay with the existing journey owner. No parked vehicle or ticket entitlement is invented.");
      return b.finish();
    }
    case "fuel-station": {
      const b = create(input, 20, 24, true);
      b.layout.theme.floor = "#bbb9af";
      for (const x of [-5, 5]) {
        b.add("fuel-pump", `pump-${x}`, x, -3, 1.5, 1, 1.65, "metal");
        b.add("canopy", `pump-canopy-${x}`, x, -3, 6, 8, 3.4, "metal", false);
        for (const z of [-6.5, .5]) b.add("cover", `pump-post-${x}-${z}`, x + (x < 0 ? -2.6 : 2.6), z, .16, .16, 3.4, "metal");
      }
      counter(b, "station-contact", "Station deliveries", -5, 5, "mission", "mission-contact");
      ride(b);
      zone(b, "forecourt", "Forecourt", 0, -2, 3.5, 16);
      gap(b, "Pump geometry does not refuel a vehicle. Bind vehicle identity, finite capacity, a current quote and atomic fuel/payment receipts through the vehicle owner before offering refueling.");
      return b.finish();
    }
    case "mechanic": {
      const b = create(input, 24, 16, true);
      b.layout.theme.floor = "#aaa99c";
      b.add("canopy", "workshop-roof", -6, -3.8, 8.4, 5.4, 3.3, "metal", false);
      for (const x of [-9.9, -2.1]) for (const z of [-6.3, -1.3])
        b.add("cover", `workshop-post-${x}-${z}`, x, z, .14, .14, 3.3, "metal");
      b.add("tool-cabinet", "tools", -6, -5, 3, .75, 1.25, "metal");
      b.add("tyre-rack", "spares", 8, -5, 2.4, .8, 1.8, "metal");
      b.add("table", "workbench", 7, -.8, 3, 1, .95, "wood");
      counter(b, "workshop-contact", "Workshop deliveries", -6, 1, "mission", "mission-contact");
      ride(b);
      zone(b, "workshop-lane", "Workshop lane", 0, -1, 3, 10);
      gap(b, "Repair, towing and parts purchases are not implemented by this layout. Existing vehicle condition and owned-car records stay unchanged.");
      return b.finish();
    }
    case "salon": {
      const b = create(input, 18, 18);
      b.layout.theme.accent = "#b8958b";
      for (const x of [-5.5, 5.5]) {
        b.add("salon-chair", `styling-chair-${x}`, x, -3, 1.1, 1.3, 1.25, "rose");
        b.fixtures[b.fixtures.length - 1].yaw = Math.PI;
        b.add("mirror", `styling-mirror-${x}`, x, -6.5, 1.6, .12, 1.8, "metal", true, .7);
        b.add("counter", `styling-console-${x}`, x, -5.4, 2.2, .65, .85, "wood");
      }
      b.add("bench", "salon-waiting", 5.5, 3, 3.4, .75, .55, "rose");
      b.service("gather", "salon-host", "Waiting area", -5, 2, -6.2, 2, "host", ["conversation-circle"], 4);
      zone(b, "salon-waiting-area", "Waiting area", -4, 2, 5, 4, "gather");
      ride(b);
      gap(b, "Styling chairs and non-reflective mirrors are scenery. No paid treatment or appearance change is enabled; saved looks keep their existing editor.");
      return b.finish();
    }
    case "gym": {
      const b = create(input, 22, 18);
      b.layout.theme.floor = "#adada2";
      for (const [i, x] of [-8, -5, 5, 8].entries())
        b.add("exercise-bike", `bike-${i}`, x, -4.7, 1.3, 2.1, 1.5, "metal");
      b.add("stage", "stretching-mat", 5.5, 1, 6, 4, .025, "sage", false);
      b.add("bench", "gym-bench", -7, 3, 3.2, .65, .5, "wood");
      b.service("gather", "gym-host", "Training group", -5.5, .5, -6.8, .5, "host", ["conversation-circle"], 6);
      zone(b, "training-floor", "Training floor", 5.5, 1, 6, 4);
      ride(b);
      gap(b, "Exercise machines have no workout commands, score, stamina or health effects. A training group uses only the existing gathering owner.");
      return b.finish();
    }
    case "cinema": {
      const b = create(input, 24, 24);
      b.layout.theme = { background: "#27282c", floor: "#47484a", wall: "#595955", accent: "#a8837b", openAir: false };
      b.add("screen", "cinema-screen", 0, -10.3, 12, .18, 2.7, "linen", true, .35);
      for (const [row, z] of [-6, -2.5, 1].entries()) for (const [seat, x] of [-8, -5, 5, 8].entries()) {
        b.add("cinema-seat", `seat-${row}-${seat}`, x, z, 1.15, 1.3, 1.2, "rose");
        b.fixtures[b.fixtures.length - 1].yaw = Math.PI;
      }
      b.service("gather", "cinema-host", "Meet in the foyer", -6, 5.5, -7.3, 5.5, "host", ["conversation-circle"], 8);
      zone(b, "foyer", "Foyer", -6, 5.5, 6, 3.5, "gather");
      ride(b);
      gap(b, "The screen is blank. Film playback, licensed content, paid tickets and seat reservations are not enabled by this destination.");
      return b.finish();
    }
    case "library": {
      const b = create(input, 22, 20);
      b.layout.theme.floor = "#d3cebd";
      for (const x of [-8.5, 8.5]) for (const z of [-6, -1.5])
        b.add("shelf", `books-${x}-${z}`, x, z, 3.5, .65, 2.1, "wood");
      for (const x of [-4.8, 4.8]) {
        b.add("table", `reading-table-${x}`, x, -4, 2.4, 1.1, .76, "wood");
        b.add("bench", `reading-bench-${x}`, x, -2.8, 2.4, .5, .48, "wood");
      }
      b.service("gather", "library-host", "Reading group", -5, 3.5, -6.3, 3.5, "host", ["conversation-circle"], 6);
      zone(b, "reading-circle", "Reading circle", -5, 3.5, 5.5, 3, "gather");
      ride(b);
      gap(b, "Books are authored scenery with no catalogue or borrowing entitlement. Reading groups reuse existing social activity, not a new learning reward.");
      return b.finish();
    }
    case "arts-centre": {
      const b = create(input, 24, 20);
      b.layout.theme.accent = "#bd9b82";
      for (const x of [-8, 8]) {
        b.add("cover", `gallery-panel-${x}`, x, -4.5, 2.8, .2, 2.4, "plaster");
        b.add("counter", `exhibit-plinth-${x}`, x, -.5, 1.3, 1.3, .8, "stone");
      }
      b.add("stage", "arts-stage", 0, -5, 8, 4, .025, "wood", false);
      b.service("gather", "arts-host", "Meet the group", -5, 3, -6.3, 3, "host", ["conversation-circle"], 8);
      counter(b, "arts-contact", "Studio deliveries", 6, 4, "mission", "mission-contact");
      zone(b, "arts-stage-area", "Stage", 0, -5, 8, 4);
      ride(b);
      gap(b, "Gallery panels are original blank fixtures. Exhibitions, performances and publishing require their existing content/activity owners; no third-party artwork is imported.");
      return b.finish();
    }
    case "campus": {
      const b = create(input, 28, 22, true);
      b.layout.theme.floor = "#c5c4b2";
      for (const x of [-9, 9]) {
        b.add("canopy", `study-shelter-${x}`, x, -4, 6, 7, 3.1, "wood", false);
        for (const dx of [-2.8, 2.8]) for (const z of [-7.2, -.8])
          b.add("cover", `study-post-${x}-${dx}-${z}`, x + dx, z, .13, .13, 3.1, "wood");
        b.add("table", `study-table-${x}`, x, -4, 3.2, 1.1, .76, "wood");
        b.add("bench", `study-bench-${x}`, x, -2.7, 3.2, .55, .48, "wood");
      }
      for (const x of [-4.5, 4.5]) b.add("planter", `courtyard-tree-${x}`, x, -6.5, 1.3, 1.3, 2.3, "sage");
      b.service("gather", "college-host", "Study group", -6, 3, -7.3, 3, "host", ["conversation-circle"], 8);
      mealTable(b, "college-meal", 7, 3);
      zone(b, "college-courtyard", "Courtyard", 0, -1, 5, 12);
      ride(b);
      gap(b, "This is a fictional college courtyard, not a real campus. No enrolment, course, examination or learning reward is enabled.");
      return b.finish();
    }
    case "office": {
      const b = create(input, 24, 22);
      b.layout.theme.floor = "#d5d0c4";
      for (const x of [-7, 7]) for (const [i, z] of [-6, -1.5].entries()) {
        b.add("table", `desk-${x}-${i}`, x, z, 3.2, 1.2, .76, "wood");
        b.add("bench", `desk-seat-${x}-${i}`, x, z + 1.3, 2.8, .5, .5, "sage");
      }
      counter(b, "office-contact", "Office deliveries", -6, 4.2, "mission", "mission-contact");
      b.service("gather", "office-host", "Meet the group", 6, 4, 7.3, 4, "host", ["conversation-circle"], 6);
      zone(b, "office-lobby", "Lobby", 0, 4, 4, 4);
      ride(b);
      gap(b, "Desks do not earn income. Work offers, parcel collection and carried rewards use the current mission/economy owners; no offline earnings are added.");
      return b.finish();
    }
    default: throw new Error("Choose an expansion venue kind.");
  }
}

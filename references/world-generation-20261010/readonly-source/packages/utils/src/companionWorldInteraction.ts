import { companionEmotes } from "./companionEmotes";
import { companionStations } from "./companionLife";
import { companionKitchenObjectTargets } from "./companionKitchenObjects";
import { activePlayRound, playChoices, playKinds, playNames, playColours, playIngredients, playSymbols, type PlayChoice, type PlayPresentation } from "./companionPlay";

export type WorldFocus = {
  id: string; kind: "entry" | "target" | "care" | "memory" | "kitchen"; index: number;
  label: string; x: number; y: number; reachable: boolean;
  point: readonly [number, number, number];
};
export const worldObjectNames = [null, "Dance mirror", "Mystery box", "Clothes rail", "Cooking station", "Keepsake box"] as const;

export function worldChoices(presentation: PlayPresentation, focus: WorldFocus, now: number): PlayChoice[] {
  const { playerId } = presentation.snapshot;
  const round = activePlayRound(presentation.snapshot.round);
  if (focus.kind === "care" || focus.kind === "memory" || focus.kind === "kitchen") return [];
  if (focus.kind === "entry") {
    const kind = playKinds[focus.index];
    if (!kind || kind === "playground") return [];
    return kind === "kitchen"
      ? [{ label: "Start cooking", command: { type: "start", kind } }, { label: "Three-minute rush", command: { type: "start", kind, rush: true } }]
      : [{ label: playNames[kind], command: { type: "start", kind } }];
  }
  if (!round) return [];
  if (!round.players.some(p => p.id === playerId)) return [{ label: "Join in", command: { type: "join" } }];
  const game = round.game, choices = playChoices(round, playerId, now), target = focus.index;
  switch (game.kind) {
    case "playground": return [];
    case "mirror": return choices.filter(c => c.command.type !== "emote" || c.command.emote === companionEmotes[target].id);
    case "mystery":
      if (game.unlocked) return [];
      if (target < 3) return [{ label: game.found.includes(target) ? `${playSymbols[game.order[target]]} · ${game.order[target] + 1}` : "Examine clue", command: { type: "touch", target }, disabled: game.found.includes(target) }];
      return [{ label: `${playSymbols[game.order[target - 3]]} · ${game.order[target - 3] + 1}`, command: { type: "touch", target }, disabled: game.found.length < 3 }];
    case "style": return choices.filter(c => c.command.type !== "look" || !c.colour || c.command.look.topColor === playColours[target]);
    case "kitchen": return choices.filter(c => c.command.type !== "ingredient" || c.command.ingredient === target);
    case "memories": return choices.filter(c => c.command.type !== "emote" || c.command.emote === companionEmotes[target].id);
  }
}

export function worldTargetLabel(presentation: PlayPresentation, kind: WorldFocus["kind"], index: number) {
  if (kind === "entry") return worldObjectNames[index] ?? "Room object";
  if (kind === "memory") return "Saved surprises";
  if (kind === "kitchen") return presentation.room ? companionKitchenObjectTargets(presentation.room)[index]?.label ?? "Kitchen" : "Kitchen";
  if (kind === "care") {
    const station = presentation.room ? companionStations(presentation.room).filter(s => s.kind !== "play")[index] : null;
    return station ? ({ sleep: "Bed", sit: "Chair", work: "Computer", cook: "Kitchen island", eat: "Dining seat", bathe: "Bath", play: "Workstation",
      "watch-tv": "TV", read: "Reading chair", stretch: "Stretching space", wash: "Sink" })[station.kind] : "Room object";
  }
  const game = presentation.snapshot.round?.game;
  if (!game) return "Room object";
  switch (game.kind) {
    case "playground": return "Room object";
    case "mirror": return companionEmotes[index].label;
    case "mystery": return index < 3 ? `Clue ${index + 1}` : "Symbol lock";
    case "style": return `Look ${index + 1}`;
    case "kitchen": return game.chopped ? game.plated ? "Serving plate" : game.cooked ? "Plate" : "Pan" : playIngredients[index];
    case "memories": return `${companionEmotes[index].label} keepsake`;
  }
}

/** Saved care and queued tasks share the same activity vocabulary. */
export const companionActivityKinds = ["sleep", "sit", "play", "cook", "eat", "bathe", "work", "watch-tv", "read", "stretch", "wash"] as const;
export type CompanionActivityKind = typeof companionActivityKinds[number];
export const isCompanionActivityKind = (value: unknown): value is CompanionActivityKind =>
  typeof value === "string" && companionActivityKinds.some(kind => kind === value);

export const companionActivityLabels: Record<CompanionActivityKind, string> = {
  sleep: "Sleep", sit: "Sit", play: "Play", cook: "Cook", eat: "Eat", bathe: "Take a bath", work: "Work at computer",
  "watch-tv": "Watch TV", read: "Read a book", stretch: "Stretch", wash: "Wash up",
};
export const companionActivityFinished: Record<CompanionActivityKind, string> = {
  sleep: "sleeping", sit: "resting", play: "playing", cook: "cooking", eat: "eating", bathe: "bathing", work: "working",
  "watch-tv": "watching TV", read: "reading", stretch: "stretching", wash: "washing up",
};
export const seatedCompanionActivity = (kind: CompanionActivityKind) =>
  kind === "sit" || kind === "eat" || kind === "work" || kind === "watch-tv" || kind === "read";

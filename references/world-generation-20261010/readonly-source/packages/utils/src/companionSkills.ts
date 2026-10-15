export const companionSkillKeys = ["cooking", "fighting", "selfDefence", "charisma", "fitness", "hustle", "dance", "flirting"] as const;
export type CompanionSkillKey = typeof companionSkillKeys[number];
export type CompanionSkills = { cooking: { experience: number } } & Partial<Record<Exclude<CompanionSkillKey, "cooking">, { experience: number }>>;
export const companionSkillLabels: Record<CompanionSkillKey, string> = {
  cooking: "Cooking", fighting: "Fighting", selfDefence: "Self-defence", charisma: "Charisma",
  fitness: "Fitness", hustle: "Hustle", dance: "Dance", flirting: "Flirting",
};
export const companionSkillPractice: Record<CompanionSkillKey, string> = {
  cooking: "Complete cooked batches.", fighting: "Land hits in combat.", selfDefence: "Block attacks or escape an agbero encounter.",
  charisma: "Talk with NPCs.", fitness: "Complete football matches or dance rounds.", hustle: "Complete computer work sessions or courier deliveries.",
  dance: "Complete nightclub rhythm objectives.", flirting: "Give a friendly compliment to the delivery contact.",
};

export const COMPANION_SKILL_MAX_EXPERIENCE = 300;
export const COMPANION_COOKING_MAX_EXPERIENCE = COMPANION_SKILL_MAX_EXPERIENCE;
const skillLevels = [0, 40, 100, 180, COMPANION_SKILL_MAX_EXPERIENCE] as const;

export function newCompanionSkills(): CompanionSkills {
  return { cooking: { experience: 0 }, fighting: { experience: 0 }, selfDefence: { experience: 0 }, charisma: { experience: 0 },
    fitness: { experience: 0 }, hustle: { experience: 0 }, dance: { experience: 0 }, flirting: { experience: 0 } };
}

export function validCompanionSkills(value: unknown): value is CompanionSkills {
  if (!value || typeof value !== "object" || Array.isArray(value) || !("cooking" in value)) return false;
  const record = value as Record<string, unknown>;
  for (const key of companionSkillKeys) {
    const progress = record[key];
    if (progress === undefined && key !== "cooking") continue;
    if (!progress || typeof progress !== "object" || Array.isArray(progress) || !("experience" in progress)
      || typeof progress.experience !== "number" || !Number.isSafeInteger(progress.experience)
      || progress.experience < 0 || progress.experience > COMPANION_SKILL_MAX_EXPERIENCE) return false;
  }
  return true;
}

/** Saved experience never decays with care needs. Missing legacy progress starts at zero. */
export function companionSkillProgress(skill: CompanionSkillKey, skills?: CompanionSkills) {
  if (!companionSkillKeys.includes(skill) || skills !== undefined && !validCompanionSkills(skills)) throw new Error("Saved skill progress needs recovery.");
  const experience = skills?.[skill]?.experience ?? 0;
  const index = skillLevels.reduce<number>((level, threshold, candidate) => experience >= threshold ? candidate : level, 0);
  const next = skillLevels[index + 1] ?? null;
  const floor = skillLevels[index] ?? 0;
  return { experience, level: index + 1, maxLevel: skillLevels.length,
    levelExperience: experience - floor,
    nextLevelExperience: next === null ? null : next - floor,
    experienceToNextLevel: next === null ? 0 : next - experience };
}

export function companionCookingProgress(skills?: CompanionSkills) {
  const progress = companionSkillProgress("cooking", skills);
  return { ...progress, fasterPercent: (progress.level - 1) * 5 };
}

/** Fixed server-owned awards only. Existing progress for other skills is preserved. */
export function awardCompanionSkillExperience(skills: CompanionSkills | undefined, skill: CompanionSkillKey, experience: number): CompanionSkills {
  if (!Number.isSafeInteger(experience) || experience < 1 || experience > 100)
    throw new Error("Provide bounded experience from a completed action.");
  const progress = companionSkillProgress(skill, skills);
  return { ...(skills ?? newCompanionSkills()), [skill]: { experience: Math.min(COMPANION_SKILL_MAX_EXPERIENCE, progress.experience + experience) } };
}

/** Call only after a server-admitted batch completes, never from a local countdown. */
export function awardCompanionCookingExperience(skills: CompanionSkills | undefined, experience: number): CompanionSkills {
  return awardCompanionSkillExperience(skills, "cooking", experience);
}

export function companionCookingGameSeconds(gameSeconds: number, skills?: CompanionSkills) {
  if (!Number.isSafeInteger(gameSeconds) || gameSeconds < 1 || gameSeconds > 300)
    throw new Error("Provide a bounded recipe duration.");
  return Math.ceil(gameSeconds * (100 - companionCookingProgress(skills).fasterPercent) / 100);
}

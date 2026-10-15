import type { CompanionWorldUtilityScope } from "../../../../../packages/utils/src/companionWorldUtilities";

const fields = { version: true, worldKey: true, clockEpochKey: true, zoneKey: true } satisfies Record<keyof CompanionWorldUtilityScope, true>;
const keys = Object.keys(fields) as (keyof CompanionWorldUtilityScope)[];

/** Per-owner copy of the shared utility scope, not its changing power sample. */
export function createUtilityScopeKey() {
  let saved: { scope: CompanionWorldUtilityScope; key: string } | undefined;
  return {
    read(scope: CompanionWorldUtilityScope) {
      if (saved) {
        let unchanged = true;
        for (const key of keys) if (saved.scope[key] !== scope[key]) { unchanged = false; break; }
        if (unchanged) return saved.key;
      }
      const key = JSON.stringify(scope);
      saved = { scope: { ...scope }, key };
      return key;
    },
    clear() { saved = undefined; },
  };
}

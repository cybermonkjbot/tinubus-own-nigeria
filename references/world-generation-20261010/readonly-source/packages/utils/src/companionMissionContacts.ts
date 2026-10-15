import { companionMissions } from "./companionMissions";
import { companionCourierCatalog } from "./companionCourierCatalog";
import type { CompanionPlaceLayout } from "./companionPlaces";

/** Generated staff can serve authored clues and parcels until a resident owns their slot. */
export function companionMissionVenueStaff(layout: CompanionPlaceLayout | null | undefined, npcId: string) {
  if (!layout?.npcs.some(npc => npc.id === npcId)) return false;
  return layout.services.some(service => service.npcId === npcId && service.access !== "policing-owner"
    && (companionMissions.some(mission => mission.steps.some(step => step.kind === "inspect"
      && step.placeId === layout.placeId && step.target.kind === "service" && step.target.id === service.id))
      || service.kind === "mission" && companionCourierCatalog.some(job => [job.employer, job.destination].some(contact =>
        contact.placeId === layout.placeId && contact.serviceId === service.id && contact.npcId === npcId))));
}

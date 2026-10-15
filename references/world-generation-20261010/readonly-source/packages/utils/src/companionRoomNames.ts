import type { PersonalRoom } from "./companion";

export function companionRoomName(room: Pick<PersonalRoom, "environmentId" | "layoutId">): string {
  if (room.environmentId === "terrace") return "Sky Terrace";
  switch (room.layoutId) {
    case "pat-studio-v01": return "The Studio";
    case "pink-bedroom-v01": case "pink-bedroom-v02": return "Blush Suite";
    case "modern-kitchen-v01": return "Supper Club";
    case "bathroom-v01": return "The Spa";
    default: return "The Nook";
  }
}

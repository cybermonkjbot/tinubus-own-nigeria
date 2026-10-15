import type { CompanionJobContact, CompanionJobTerms } from "./companionJobs";

export type CompanionCourierDefinition = Readonly<{
  id: string;
  label: string;
  employer: Readonly<CompanionJobContact>;
  destination: Readonly<CompanionJobContact>;
  parcel: Readonly<CompanionJobTerms["parcel"]>;
  credits: number;
  periodMs: number;
  requiresTrustedCourier?: true;
}>;

const contacts = {
  mamaPut: { placeId: "mama-put", serviceId: "delivery-contact", npcId: "delivery-contact-npc" },
  contentHouse: { placeId: "content-house", serviceId: "creator-contact", npcId: "creator-contact-npc" },
  market: { placeId: "south-market", serviceId: "market-contact", npcId: "market-contact-npc" },
  supermarket: { placeId: "corner-supermarket", serviceId: "collection", npcId: "collection-npc" },
  fuelStation: { placeId: "unity-fuel-station", serviceId: "station-contact", npcId: "station-contact-npc" },
  workshop: { placeId: "mechanic-yard", serviceId: "workshop-contact", npcId: "workshop-contact-npc" },
  arts: { placeId: "arts-centre", serviceId: "arts-contact", npcId: "arts-contact-npc" },
  office: { placeId: "co-working-studio", serviceId: "office-contact", npcId: "office-contact-npc" },
} as const satisfies Record<string, Readonly<CompanionJobContact>>;

const hourMs = 60 * 60_000;

/** Parcel work only. Rewards are noncash lagos-credit, not stock, repairs or event access.
 * Added route amounts follow the authored corridor lengths, not live route pricing.
 * IDs 0 and 1 retain their original terms and six-hour issuance windows.
 */
export const companionCourierCatalog: readonly CompanionCourierDefinition[] = [
  {
    id: "0", label: "Lunch delivery", employer: contacts.mamaPut, destination: contacts.contentHouse,
    parcel: { key: "courier-lunch", label: "the lunch parcel" }, credits: 60, periodMs: 6 * hourMs,
  },
  {
    id: "1", label: "Return a supply parcel", employer: contacts.contentHouse, destination: contacts.mamaPut,
    parcel: { key: "courier-supplies", label: "the supply parcel" }, credits: 85, periodMs: 6 * hourMs,
  },
  {
    id: "arts-refreshments", label: "Take refreshments to Arts centre", employer: contacts.mamaPut, destination: contacts.arts,
    parcel: { key: "courier-arts-refreshments", label: "the packed refreshments" }, credits: 95, periodMs: hourMs,
  },
  {
    id: "trusted-studio-lunch", label: "Deliver the studio lunch order", employer: contacts.mamaPut, destination: contacts.office,
    parcel: { key: "courier-studio-lunch", label: "the packed studio lunch order" }, credits: 145, periodMs: hourMs,
    requiresTrustedCourier: true,
  },
  {
    id: "market-greens", label: "Deliver greens to Mama put", employer: contacts.market, destination: contacts.mamaPut,
    parcel: { key: "courier-market-greens", label: "the bag of greens and peppers" }, credits: 95, periodMs: hourMs,
  },
  {
    id: "pantry-staples", label: "Deliver pantry staples", employer: contacts.supermarket, destination: contacts.mamaPut,
    parcel: { key: "courier-pantry-staples", label: "the sealed rice and beans parcel" }, credits: 80, periodMs: hourMs,
  },
  {
    id: "market-fruit", label: "Take fruit to Corner supermarket", employer: contacts.market, destination: contacts.supermarket,
    parcel: { key: "courier-market-fruit", label: "the packed fruit box" }, credits: 105, periodMs: hourMs,
  },
  {
    id: "office-groceries", label: "Deliver the studio grocery order", employer: contacts.supermarket, destination: contacts.office,
    parcel: { key: "courier-office-groceries", label: "the tea, milk and biscuit parcel" }, credits: 145, periodMs: hourMs,
  },
  {
    id: "arts-light-stands", label: "Take light stands to Arts centre", employer: contacts.contentHouse, destination: contacts.arts,
    parcel: { key: "courier-arts-light-stands", label: "the folded light stands" }, credits: 60, periodMs: hourMs,
  },
  {
    id: "studio-mics", label: "Deliver microphones to Content house", employer: contacts.office, destination: contacts.contentHouse,
    parcel: { key: "courier-studio-mics", label: "the microphone case" }, credits: 155, periodMs: hourMs,
  },
  {
    id: "office-print-box", label: "Deliver prints to the co-working studio", employer: contacts.arts, destination: contacts.office,
    parcel: { key: "courier-office-print-box", label: "the wrapped print box" }, credits: 160, periodMs: hourMs,
  },
  {
    id: "workshop-fuses", label: "Take fuses to Mechanic yard", employer: contacts.fuelStation, destination: contacts.workshop,
    parcel: { key: "courier-workshop-fuses", label: "the boxed workshop fuses" }, credits: 75, periodMs: hourMs,
  },
  {
    id: "workshop-fasteners", label: "Deliver workshop fasteners", employer: contacts.market, destination: contacts.workshop,
    parcel: { key: "courier-workshop-fasteners", label: "the sealed nuts and bolts box" }, credits: 70, periodMs: hourMs,
  },
  {
    id: "station-hose-kit", label: "Take a hose kit to Unity fuel station", employer: contacts.workshop, destination: contacts.fuelStation,
    parcel: { key: "courier-station-hose-kit", label: "the packed hose and clamp kit" }, credits: 75, periodMs: hourMs,
  },
  {
    id: "festival-bunting", label: "Deliver festival bunting to South Market", employer: contacts.arts, destination: contacts.market,
    parcel: { key: "courier-festival-bunting", label: "the festival bunting box" }, credits: 95, periodMs: hourMs,
  },
  {
    id: "festival-cups", label: "Take festival cups to Arts centre", employer: contacts.supermarket, destination: contacts.arts,
    parcel: { key: "courier-festival-cups", label: "the paper cup and napkin parcel" }, credits: 100, periodMs: hourMs,
  },
];

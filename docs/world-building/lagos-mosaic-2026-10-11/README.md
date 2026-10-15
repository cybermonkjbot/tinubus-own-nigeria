# Lagos neighborhood mosaic

October 11, 2026. Parallel research and authoring assignment. This folder contains design documents, not registered runtime map data or completed 3D assets.

## Work allocation

| Research chat | Scope | Owned dossier |
| --- | --- | --- |
| Helmholtz | Surulere and an Agege/Mushin working-residential slice | everyday-mainland.md |
| Gibbs | Yaba and Ikeja mixed centers | tech-and-mixed-centres.md |
| Mill | Two complementary old Lagos Island slices | old-lagos-island.md |
| Darwin | Island mixed frontage and a Lekki/Ajah or outer-coastal slice | island-and-coast.md |
| Archimedes | Makoko/Iwaya shore-water threshold and a contrasting lagoon edge | lagoon-neighborhoods.md |
| Sartre | Apapa public logistics edge and an Ijora/Orile working street | port-and-working-streets.md |

The coordinator owns this index, evidence review, cross-dossier reconciliation and the eventual composite placement plan. Scope names are research assignments; they do not certify features or boundaries.

## Shared authoring contract

Create two specific neighborhood slices per dossier. Each needs a main street, connected side streets, an authored 150-300 metre walking sequence, everyday activity and usable local destinations. The five original biome anchors remain reference themes, not the limit of neighborhood coverage.

Compress journeys between selected slices, not buildings, people, vehicles or the ordinary neighborhood spaces between landmarks. Preserve residential streets, yards, service access, workshops and quieter intervals. Do not impose the earlier 6 by 4.5 kilometre envelope before this research is reconciled.

Each dossier must distinguish source-confirmed facts, map-supported inference and authored game proposals. Coordinate precision is not accuracy. Record source URLs, access dates, feature IDs, coordinate order, source dates and licensing limits. Do not claim photographs were inspected when only captions or addresses were available.

OSM street names and nodes do not establish current buildings, road widths, bridge decks, drain capacities, water depth or material condition. Unknown measurements stay unknown; proposed gameplay dimensions must be labeled design values. Historical planning proposals do not establish present construction.

Business names, architecture dates, dialogue, branded signage and culturally specific props require suitable evidence. Use unnamed authored businesses where real branding is unnecessary, identifying them as fictional in production metadata. Evidence does not automatically grant permission to reproduce photos, music, logos or private interiors.

## Required slice deliverable

- Source anchors and an evidence ledger with unresolved gaps.
- Connected street topology and the walking-height frontage sequence.
- Everyday interiors, courtyards, livelihoods and time-of-day routines.
- Developer Asset Checklist: meshes and architecture; shaders and textures; dynamic systems and spawners.
- Spatial audio, lighting, rain/runoff and seasonal behavior, distinguishing observations from simulation choices.
- One fictional local disruption with alternate routes and a clearing condition.
- Incoming and outgoing connector briefs, distinguishing real connections from composite joins.

## Connector handoff

For each proposed connector, identify the source-side reference feature, destination-side reference feature, supported real-world relationship, omitted intervening geography and the proposed composite join. Do not describe omitted districts as empty land.

Record movement modes, public/private access, proposed clearances, shoreline or deck-height unknowns, pedestrian continuation, drainage continuity and audio/traffic continuity. Local incident logic must not disconnect a whole district or turn private courtyards into mandatory public shortcuts.

Third Mainland and Lekki-Ikoyi references must retain their different endpoint relationships. Makoko's selected water slice must include a shore transition without representing the whole community as stilt housing. Port reference slices must distinguish public streets from controlled facilities. These are evidence-review requirements, not a replacement for fresh geographic research.

## Coordinate and integration boundary

Keep geographic source coordinates separate from composite placements. Source convention is metres with x east, y up, z south. Unreal conversion is UE(X,Y,Z) = 100 * source(z,x,y), applied once.

No final engine placements are assigned by individual research chats. The existing additive geographic reference is `output/cribtest11-visible-world-2026-10-11/parent-geography/lagos-geographic-authoring-v1.json`; it explicitly distinguishes estimated authoring envelopes from surveyed geography and has no established height datum.

Preserve canonical IDs, theatre placement, existing maps, runtime destinations and unrelated work. The coordinator must resolve overlapping slices and connector conflicts before any implementation consumes these dossiers.

## Delivery status

First round delivered six dossiers covering twelve reference slices. The coordinator reviewed their scope, source registers, selected walking sequences, connector offers and documented gaps. Geographic and contemporary visual acceptance remain incomplete.

The [assembly plan](assembly-plan-v1.md) registers those slices, proposes local circuits, consolidates asset requirements and addresses repeated activities/events. It is a design proposal, not engine scene data.

The same six chats are carrying out a second round:

| Research chat | New task | Owned output |
| --- | --- | --- |
| Helmholtz | Agege street pocket | agege-extension.md |
| Gibbs | Ebute Metta/Oyingbo street pocket | ebute-metta-extension.md |
| Mill | Obalende/Ikoyi intermediate threshold | obalende-ikoyi-threshold.md |
| Darwin | Outer-coastal or peripheral street pocket | outer-coast-extension.md |
| Archimedes | Dated-imagery evidence review | imagery-evidence-review.md |
| Sartre | Cross-dossier connector evidence audit | connector-evidence-ledger.md |

Second-round outputs are pending delivery and review. File presence alone does not establish evidence acceptance. No mesh creation, engine integration, packaging, tests, builds or deployment is part of this assignment.

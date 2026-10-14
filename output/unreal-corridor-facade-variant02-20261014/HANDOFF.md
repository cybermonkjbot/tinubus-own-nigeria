# Corridor Facade Variant02

## Delivery

Bounded Windows W03/W22 environment art, October 14, 2026. World building first; reviews remain deferred, not accepted or resumed. Native preservation hold unchanged. No canonical parcel assignment.

- Host/workspace: Cribtest11, C:\dev\cribtest-game.
- Start commit: c30282041c94b36220a0573105fa6a219d268774. This additive checkpoint's commit is the enclosing Git commit; only this new output directory is owned.
- Requested model/effort: gpt-6.1-sol / MEDIUM; runtime model selection is not independently observable from the available tools.
- One installed Blender 5.2.2 LTS process, PID 5384, created, saved and reopened CorridorFacadeVariant02.blend in scene CorridorFacadeVariant02, Cycles. Normal completion/exit recorded in production.log.
- Saved blend SHA256: 6e8c340d0050f6c01e4f1b04f5ba81a4d1b62c37a7f9e27f386cf57a7488da4e.
- Preserved live workbench: PID 6116, LagosWorkbench.blend, BaddieAssetWorkbench, Cycles, idle; confirmed read-only after production. Unreal PID 10404/session 2 still runs BaddieCompanion.uproject. No Unreal API, scene or package operations; loaded map was not queried during this asset-only slice.

## Owned Art And Interfaces

Replaced only WindowHood, Canopy, CanopyWallBracket, CanopyBracketArm, CanopyWallBracket.001 and CanopyBracketArm.001 in the new scene. New collection: Variant02_ShadeAndAwning. Protected base evaluated meshes, transforms, normals and material-slot identities match before/after/reopen in protected-geometry.json. Original masonry, slabs, corrected returns, support ground, apertures, glazing, frames, grille and threshold are unchanged. Earlier contact repairs were not reopened.

Metres; origin [0,0,0], +Y street, +X frontage, +Z up. Bay 3.9m, two 3m storeys at Z0/3/6, wall .25m, window 1.3x1.35m, shop aperture 2.8x2.5m. Source-derived architecture, not surveyed measurements.

New authored dimensions: three 1.70m-wide blades with approximately .715m assembly projection; 3.14m-wide pitched awning with .86m projection, rear Z2.76/front Z2.62, 50mm downstand and 35mm hem. Thin closed solids, real folded edges, wall plates, arms, diagonal braces and visible small fasteners. New single-user painted-metal copies use bronze [.19,.115,.061]/roughness .44 and charcoal [.07,.08,.075]/roughness .48, dielectric coatings. Original materials and glass unchanged. No external textures, exposure, lighting or camera concealment.

attachment-readback.json retains evaluated geometry, bounds, manifold/consistent winding results and support scope. Four plate bounds butt to wall Y0 with nominal signed gap 0m. Nominal footprints: two shade plates .01785m2 each; two awning plates .018m2 each; bevels reduce the flat perimeter. These are nominal footprint measurements, not measured Boolean contact areas. Lowest shade-to-upper-aperture clearance .132559776m; folded-drip-to-shop-aperture clearance .069999933m. Beam/sheet junction intersections are intentional fixed/welded assembly overlaps; no load certification or fastener penetration claim. No exhaustive pairwise intersection proof is claimed.

## Actual Inspection And Defects

All five actual 800x800 PNGs opened: front.png, grazing.png, reverse.png, player165.png and wide.png. Retained Cycles CPU, 24 samples, six threads, AgX; baseline poses/lenses recorded in asset-contract.json. First front preview was inspected before save/reopen; remaining views were rendered after reopen and inspected before closing the same process.

Blade depth, spaced silhouette, mounting arms and brackets read in grazing/player-height views. Awning pitch and folded edge read in angled views. Glazing remains visible, openings unobstructed; no renewed rectangular wall/slab patches, visible floating attachments or light leaks found in these five views. The dark band beneath the awning is a retained cast shadow, not a repaired wall contact patch. No preview-driven correction was needed.

Remaining limitations: restrained constant base finishes rather than photographic surface detail; small fasteners/bevels are not fully resolved at 800px; reverse view intentionally exposes the sectional open back. No full rooms, stock, signage, brand or playable door. This is authored reusable art, not full photorealism, Unreal placement/coverage, collision/navigation, thermal performance, structural certification, gameplay or release acceptance.

## Preservation And Dependencies

preservation-before.json and preservation-after.json contain all 68 Variant01-tree paths/hashes; the after receipt confirms byte-identical preservation. Original failure records and contact deliveries remain immutable. Six unrelated tracked dirty paths remain untouched: .uproject, DefaultEngine.ini, DefaultInput.ini, add_adjoining_ground.py, native_generator.py and standalone-companion-unreal-port.md. Existing unrelated untracked assets remain untouched.

Standalone author_variant02.py SHA256 cf3c76e21418ad75420d813584e7ecec0cb81079e2319beb55bec07cccd07c6d: standard library plus installed Blender bpy/bmesh/mathutils only; no imported authoring helpers or old generator execution. It intentionally refuses an existing blend/PNG output; this directory is frozen evidence, not a rerun target.

Direct file dependencies:

- ../unreal-corridor-facade-variant01-20261014/front-contact-finish01/CorridorFacadeVariant01_FrontContactFinish01.blend: dce613cebc5efed46930e98aedb3aab7f13d13619c44650b27b5659f6611ffa5; committed before this slice.
- ../unreal-corridor-facade-variant01-20261014/asset-contract.json: 5992b46681b483245b1b39117490a5183dd4d13a3c230d7686f18af7f55332a0; committed before this slice.
- ../unreal-corridor-facade-variant01-20261014/contact-repair02/repair-contract.json: b911c082cd50a2bf9ced2098d6191193b37a09a3d18980c14f2736577f39d9b8; committed before this slice, read only for matched poses/lenses.
- ../cribtest11-visible-world-2026-10-11/parent-realism-kit/kit-contract.json: 37314e3e16633b7f5edc9b2c11e105561813bfe2e6c3c1bed6ba942b04f991ff; retained local input, verified before production, not Git tracked. Its provenance is preserved in the contract, but this checkpoint does not claim a self-contained Git checkout rerun without that retained file. No unrelated original source was staged.

Inherited author_yaba_frontage.py source pin is provenance only; it is not executed or imported. No tests, lint, typechecks, builds, exports, downloads, restarts or new servers. Only authorized Blender author/save/reopen/stills, read-only identity/hash inspection and source/diff inspection performed. Hook samples inspected; commit hooks disabled. Next art slice requires its own scope; native hold remains intact.

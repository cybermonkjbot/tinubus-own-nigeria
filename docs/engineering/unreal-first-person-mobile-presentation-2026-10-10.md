# Unreal first-person mobile presentation

Research date: October 10, 2026.

## Scope and version

Our installation record identifies UE 5.8.3, changelist `58210709`. Our expedition brief selects first-person, native Character Movement and Mobile Forward with precomputed lighting; it does not establish configured or playable presentation. [R1][R2] Epic pages below identify themselves as UE 5.8. Their documentation does not certify our installed patch or devices.

## Canopy-to-clearing exposure

Mobile Forward supports Histogram and Basic auto exposure, plus Local Exposure. HDR-disabled Mobile Forward supports Manual exposure but not those automatic modes or Local Exposure. Local Exposure adjusts regional contrast; it is not the temporal adaptation control. [1][2]

Use native Post Process exposure with a bounded adaptation range. Set Min/Max EV100 when extended luminance range is enabled, otherwise Min/Max Brightness. Equal limits disable adaptation, providing a fixed-exposure comfort option. Histogram range bounds are not substitutes for these adaptation clamps. [1]

`Speed Up` controls dark-to-bright adaptation; `Speed Down` controls bright-to-dark adaptation, in f-stops per second. Epic recommends faster bright-entry and slower dark-entry adjustment. Avoid prolonged whiteout or darkness for tension. A center-weighted metering mask reduces sensitivity to bright screen edges. Choose clamps and rates against our authored canopy/clearing lighting, not universal numbers. [1]

Verify effective Mobile HDR, `r.Mobile.EyeAdaptation=1`, `r.EyeAdaptationQuality>0` and device-profile overrides. Epic notes an `Android_Low` exception, while the Scalable project preset disables auto exposure. Feature support is not proof it is enabled. [1][3] UE 5.8 release notes include a mobile histogram fix when adaptation is skipped because no adaptation range exists. [4]

## First-person rendering and clipping

Native Camera Component first-person FOV and scale, primitive tagging, and per-vertex interpolation work across platforms/configurations. Scale brings hands/equipment inside player bounds to avoid most world intersections. Excessive shrinking crosses the near clip plane and makes geometry disappear; it is not a collision solution. Keep world FOV independent of equipment framing. [5]

The advanced GBuffer-dependent path excludes mobile and forward renderers. First-person self-shadow, World Space Representation shadow/reflection features and the first-person Scene Texture mask cannot be assumed on our baseline. The documented GBuffer path also requires disabling static lighting; do not sacrifice our baked-lighting baseline for it. These limitations retain wording dating to 5.6 inside the 5.8 page. [5]

## Weapon lowering through animation reuse

Character Movement already owns walking, falling, swimming and movement limits. Animation Blueprints can consume its velocity, horizontal speed and `IsFalling`, including through thread-safe Property Access. [6][7] Blend authored lowered/ready poses using actual movement plus accepted sprint, stance or crossing state. Mud slowdown must come from movement policy, not an animation that merely looks slower. Keep presentation from changing movement or firing permission. No replacement camera or movement loop is needed.

Native AnimGraph blending supports additive and bone-filtered overlays, preserving locomotion while changing arms/equipment. [8] Inspect the matching First Person Arena Shooter weapon animations before authoring replacements. Epic documents those assets, not an automatic terrain-triggered lowering policy. [9]

IK Retargeting transfers existing animation across different skeletons and can preserve hand/foot contacts. [10] Our approved Companion has no walk/run clips, and its recorded clip inventory contains no lowering animation. Reuse still requires suitable source poses, rig mapping and grip/appearance acceptance. [R3]

## Unresolved acceptance

Confirm 5.8.3 first-person limitations and the equal-limit histogram behavior. Select named Android devices, graphics APIs and effective quality profiles for acceptance. Inspect exposure pumping, bright-entry readability, return-to-canopy visibility, weapon/near-plane intersections and retargeted grips on those devices. No measured performance or comfort result exists from this research.

This research changed only the note. The parent implemented audio-compass source separately. No runtime validation, asset operations or remote mutations occurred during this research.

## Sources

Epic primary sources, consulted October 10, 2026, with UE 5.8 selected:

1. [Auto Exposure](https://dev.epicgames.com/documentation/en-us/unreal-engine/auto-exposure-in-unreal-engine?application_version=5.8), metering, clamps, adaptation speed and Mobile Usage.
2. [Mobile rendering feature matrix](https://dev.epicgames.com/documentation/en-us/unreal-engine/rendering-features-reference?application_version=5.8), Post Process Volume exposure rows.
3. [Setting up a project for mobile platforms](https://dev.epicgames.com/documentation/en-us/unreal-engine/setting-up-an-unreal-engine-project-for-mobile-platforms?application_version=5.8), Scalable defaults.
4. [UE 5.8 release notes](https://dev.epicgames.com/documentation/en-us/unreal-engine/unreal-engine-5-8-release-notes?application_version=5.8), mobile histogram exposure bug fix.
5. [First Person Rendering](https://dev.epicgames.com/documentation/en-us/unreal-engine/first-person-rendering?application_version=5.8), scale, near clipping and Limitations and Notes.
6. [Movement Components](https://dev.epicgames.com/documentation/en-us/unreal-engine/movement-components-in-unreal-engine?application_version=5.8), Character Movement.
7. [How to Get Animation Variables](https://dev.epicgames.com/documentation/en-us/unreal-engine/how-to-get-animation-variables-in-animation-blueprints-in-unreal-engine?application_version=5.8), velocity, speed and falling.
8. [Blending Animations](https://dev.epicgames.com/documentation/en-us/unreal-engine/blending-animations-in-unreal-engine?application_version=5.8), AnimGraph blending.
9. [First Person Template](https://dev.epicgames.com/documentation/en-us/unreal-engine/first-person-template-in-unreal-engine?application_version=5.8), Arena Shooter weapons and animation assets.
10. [IK Rig Animation Retargeting](https://dev.epicgames.com/documentation/en-us/unreal-engine/ik-rig-animation-retargeting-in-unreal-engine?application_version=5.8), skeleton compatibility and contact preservation.

Repository evidence:

- R1. [Installed version and mobile reuse decisions](unreal-rural-mobile-reuse-2026-10-10.md).
- R2. [Rural expedition brief](standalone-companion-rural-expedition.md), first-person view and terrain movement policy.
- R3. [Approved Companion asset inventory](standalone-companion-unreal-assets.md), Import and rendering requirements.

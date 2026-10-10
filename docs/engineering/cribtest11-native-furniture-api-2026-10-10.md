# Cribtest11 native saved-furniture API evidence

Inspected installed Epic source on October 10, 2026 through read-only SSH. `Build.version` identifies UE 5.8.3, changelist 58210709, branch `++UE5+Release-5.8` [V:1-10]. Source references below use one-based lines and whole-file SHA-256 hashes. The research pass changed only this note. The parent integration follow-up below records how the source implementation uses those findings. No engine execution, validation command or asset operation ran.

## Decision for the owner implementation

Native spawning into the loaded room and delegate-free mesh loading are supported. Hidden-level membership is not an activation barrier. `SpawnActor` can register native components and run `BeginPlay` immediately. The proposed hidden, noncolliding, unticked constructor defaults are therefore necessary [AC:4276-4367,4430-4530].

The proposed transform branch has the right coordinate math for an unattached root and a rigid level-instance transform, but **it is unsafe to assume that branch alone completes the streaming lifecycle**. UE's first visibility transition writes level transforms directly into relative properties. Its registration pass can skip an actor already registered by `SpawnActor`. The direct setters do not refresh component-to-world state [WC:3781-3793; LU:464-470,582-597; SCC:4378-4403; LC:2005-2017]. Before later activation, explicitly reconcile the spawned root and children after the level transform phase using the expected saved-local-to-instance transform and normal transform-update machinery. Do not assume native streaming repairs the pre-registered actor. This is a source-derived integration risk, not an observed in-game failure.

Keep the owner's streamable handle active while any furniture configuration still consumes the catalog's soft mesh references. Completion alone does not mean every mesh loaded [SH:195-223,315-327].

### Parent integration follow-up

The parent implementation retains the expected apartment pose derived from the saved local placement when spawning before the transform phase. The existing owner poll waits for `bAlreadyMovedActors`, updates the root's component-to-world cache, applies that retained pose through `SetActorTransform`, then refreshes registered component transforms. It checks the resulting root transform and clears the pending pose. It does not multiply the current actor transform, write the level phase flag or activate content. Both hidden preparation and this reconciliation remain behind the existing closed admission/render-readiness checks.

The parent independently inspected `USceneComponent::UpdateComponentToWorld` at [SCH:958-960], its implementation at [SCC:760-785], `AActor::SetActorTransform` at [AC:5156-5180] and `AActor::UpdateComponentTransforms` at [AH:3272-3273; AC:6341-6349]. The actor method calls `UpdateComponentToWorld` on registered components. The scene update propagates to attached children [SCC:1022-1025,1054-1058,2909-2950]. Parent SHA-256 checks of `Actor.cpp`, `SceneComponent.cpp` and `SceneComponent.h` matched the register below. This was a bounded source read, not engine execution.

The source also rejects duplicate bulb slots and missing/non-finite scalar values. Cleanup empties override materials before dropping the mesh/MID, clears saved IDs and the pending transform, and checks the result of `Destroy(false, false)`. Failed destruction leaves only an empty, hidden, inactive actor in the level requested for unload and records an unavailable status. The load handle stays active until all required meshes have been assigned to their actors. Later reconfiguration would need a new retained load handle; it is not implemented here.

## Spawn and streamed-level ownership

Installed `World.h` provides these overloads [W:3770-3775,3819-3825]:

```cpp
AActor* SpawnActor(UClass* Class, FTransform const* Transform,
    const FActorSpawnParameters& SpawnParameters = FActorSpawnParameters());

template<class T>
T* SpawnActor(UClass* Class, FTransform const& Transform,
    const FActorSpawnParameters& SpawnParameters = FActorSpawnParameters());
```

The native subclass call shape is:

```cpp
FActorSpawnParameters Params;
Params.OverrideLevel = LoadedRoomLevel;
Params.ObjectFlags = RF_Transient;
Params.SpawnCollisionHandlingOverride =
    ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

auto* Actor = World->SpawnActor<ABaddieCompanionFurnitureActor>(
    ABaddieCompanionFurnitureActor::StaticClass(), SpawnTransform, Params);
```

`OverrideLevel` is a `ULevel*`; `ObjectFlags` is `EObjectFlags` [W:436-437,514-515]. The implementation selects that level, uses it as the new actor's outer, verifies its level, adds it to `Level->Actors`, then calls `PostSpawnInitialize`. No Blueprint prefab is required [LA:533-541,669-675,739-755]. `RF_Transient` does not substitute for hiding, collision suppression or cleanup. `AlwaysSpawn` selects the collision policy that neither rejects nor relocates for overlap; it does not bypass invalid class, world teardown or invalid-transform failures [LA:468-517,623-667; AC:4454-4495]. Keep the native root's default transform identity or account for the CDO root transform multiplication [AC:4304-4324].

Poll the existing stream on the game thread. `ULevel* GetLoadedLevel() const` returns its loaded level, not proof of visibility [SLH:555,588]. `SetLoadedLevel` sets `OwningWorld` and records either `LoadedVisible` or `LoadedNotVisible` [SLC:1404-1437]. Require the same live owner world and current loaded-level identity; do not spawn against a stale level after cancellation or unload. `SetShouldBeVisible(false)` only changes the requested visibility and asks the world to reconsider streaming [SLC:2282-2291].

### Transform phase

`ULevel::bAlreadyMovedActors` is a public bit used during making a level visible [LH:626-648]. When false, `UWorld::AddToWorld` applies the level transform and sets it true. That operation includes newly spawned actors already in the level's actor list [WC:3781-3793; LU:582-597]. For the proposed unattached, identity-default root:

| Phase at configuration | Root coordinates | Owner obligation |
| --- | --- | --- |
| `!bAlreadyMovedActors` | Saved room-local pose in UE units | Leave room-instance placement for UE's level-transform phase. After that phase, explicitly reconcile component-to-world state before activation. |
| `bAlreadyMovedActors` | Saved local pose multiplied by the same level-instance transform | Use the normal actor transform setter. Do not apply the instance transform again to an already transformed pose. |

The caveat is specific. `ApplyLevelTransform` uses direct relative location, rotation and scale assignments for unattached components. Those setters only assign properties. The subsequent `IncrementalRegisterComponents` path skips already registered actors, and actor registration itself skips registered components [LU:464-470,582-597; SCC:4378-4403; WC:3854-3896; LC:2005-2017; AC:6207-6235]. Hiding prevents presentation, not cached-transform staleness. Reconciliation must derive the expected pose from saved local data, never multiply the actor's current transform again. Do not write UE's phase flag or make the level visible just to force it to advance.

### Cleanup

Installed signatures [AH:1922-1926,2320-2327]:

```cpp
virtual void SetActorHiddenInGame(bool bNewHidden);
void SetActorEnableCollision(bool bNewActorEnableCollision);
bool Destroy(bool bNetForce = false, bool bShouldModifyLevel = true);
```

Hidden and collision state are separate. The setters update component visibility and collision/overlap state respectively [AC:5313-5339]. On invalidation, suppress visibility, collision, light output and activity first; clear private IDs and material/mesh references, then destroy retained actors and request stream unload. `Destroy(false, false)` avoids the default request to modify the level during removal. Check its result rather than assuming immediate deletion. The header describes latent destruction; the world implementation can defer during `BeginPlay`, otherwise removes the actor, unregisters components, marks actor/components for collection and unregisters ticks. An actor without a valid world cannot complete normal `Destroy` through this wrapper [AH:2320-2327; AC:5343-5359; LA:896-904,1033-1065].

## Streamable request and lifetime

The manager accessor is static and returns a reference [AM:93-105]:

```cpp
static FStreamableManager& GetStreamableManager();
```

In this installation, the ordinary path-container overload is a forwarding template, not a fixed `const TArray&` overload [SH:755-763]:

```cpp
template<typename PathContainerType = TArray<FSoftObjectPath>,
    typename FuncType = FStreamableDelegateWithHandle,
    typename = std::enable_if_t<!std::is_same_v<
        std::decay_t<PathContainerType>, FStreamableAsyncLoadParams>, void>>
TSharedPtr<FStreamableHandle> RequestAsyncLoad(
    PathContainerType&& TargetsToStream,
    FuncType&& DelegateToCall = FStreamableDelegateWithHandle(),
    TAsyncLoadPriority Priority = DefaultAsyncLoadPriority,
    bool bManageActiveHandle = false,
    bool bStartStalled = false,
    FString DebugName = FString(),
    UE::FSourceLocation Location = UE::FSourceLocation::Current());
```

Thus `UAssetManager::GetStreamableManager().RequestAsyncLoad(MeshPaths)` accepts an ordinary `TArray<FSoftObjectPath>` and supplies an empty completion delegate. The wrapper constructs its own target array and forwards load parameters [SH:1004-1048]. No completion callback is required for owner polling. Use the initialized engine's asset manager; `IsInitialized()` and `GetIfInitialized()` exist if the owner can run before manager construction [AM:91-105].

The relevant handle methods are [SH:195-223,315-327]:

```cpp
bool HasLoadCompleted() const;
bool WasCanceled() const;
bool IsActive() const;
void CancelHandle();
void ReleaseHandle();
```

- Retain the returned `TSharedPtr` in the owner. With the default `bManageActiveHandle=false`, do not rely on a manager-owned persistent reference. `true` adds the request to the manager's explicit retained-handle list [SH:744-763; SC:2086-2089].
- Poll handle validity, cancellation and completion separately. Failed assets can be null even when `HasLoadCompleted()` is true. Resolve and type-check every required mesh before configuration [SH:195-217]. Do not replace a missing mesh with an unrelated primitive or synchronously load during polling.
- Active handles keep their loaded assets resident. Keep this handle active across all pending furniture creation and any later reconfiguration that resolves the catalog's soft paths. Once assigned, the mesh component has a reflected `TObjectPtr<UStaticMesh>`, but that only covers assigned meshes, not other soft catalog entries [SH:195-223; SMH:130-133].
- `CancelHandle()` asserts game-thread execution, makes the request inactive, removes its references and suppresses completion callbacks. It is not proof that underlying package work never loaded anything [SC:981-1047; SH:208-217,322-327].
- `ReleaseHandle()` also asserts the game thread. If complete, it drops the request's retained asset references. If unfinished, it sets `bReleaseWhenLoaded`; that is not cancellation. The header also documents implicit release when shared ownership ends [SC:1065-1113; SH:315-320]. Keeping a released `TSharedPtr` does not keep the handle active.

## Mesh and per-instance material calls

These methods are available at runtime, not inside an editor-only block [SMH:448-453,665-672; PRH:1624-1629; MH:96-101]:

```cpp
virtual bool UStaticMeshComponent::SetStaticMesh(UStaticMesh* NewMesh);
virtual int32 UStaticMeshComponent::GetMaterialIndex(FName MaterialSlotName) const;
virtual TArray<FName> UStaticMeshComponent::GetMaterialSlotNames() const;
virtual UMaterialInstanceDynamic* UPrimitiveComponent::CreateDynamicMaterialInstance(
    int32 ElementIndex, UMaterialInterface* SourceMaterial = NULL,
    FName OptionalName = NAME_None);
void UMeshComponent::EmptyOverrideMaterials();
```

`SetStaticMesh` returns false both when the supplied mesh is already assigned and when a runtime change is disallowed on a registered static component. A false return is not universally a load failure. For the proposed movable component, clear/reconfiguration can use the normal setter; do not bypass it by mutating the field. Successful changes update render/physics state and navigation relevance [SMC:2366-2424; SCH:1363-1365].

`GetMaterialIndex` delegates to the mesh and returns `-1` without a mesh. `GetMaterialSlotNames` appends each `MaterialSlotName` from the mesh's static-material array, with no deduplication [SMC:2872-2888]. Count matching `FName` entries and require exactly one for the emissive binding, then resolve its index. A found index alone does not prove uniqueness.

`CreateDynamicMaterialInstance` optionally installs a supplied source material, otherwise reads the component's current material. It creates an MID with the component as outer and assigns it back to that slot; an existing MID is returned as-is [PRC:2620-2643]. Therefore reconfiguration must clear old overrides before creating the new instance. `EmptyOverrideMaterials()` calls removal callbacks where applicable, clears both override and slot-overlay arrays, dirties render state and requests PSO precaching. It is a runtime method despite its editor-oriented comment [MH:96-101; MC:246-280]. Also clear the actor's own MID pointer. Clearing only `BulbMaterial`, or changing only the static mesh, is not equivalent to removing component overrides. Do not mutate `OverrideMaterials` directly; its header warns of a GC/render-thread race [MH:28-31].

### Scalar parameter lookup

The exact installed signature uses the hashed-info alias [MI:1211-1213]:

```cpp
bool GetScalarParameterValue(const FHashedMaterialParameterInfo& ParameterInfo,
    float& OutValue, bool bOveriddenOnly = false) const;
```

`FHashedMaterialParameterInfo` aliases `FMemoryImageMaterialParameterInfo`, which has a non-explicit constructor from `FMaterialParameterInfo`. Therefore the proposed `GetScalarParameterValue(FMaterialParameterInfo(ParameterName), Value)` call matches this declaration through conversion. The `FName` constructor selects `GlobalParameter` and `INDEX_NONE` by default [MP:54-59,118-122,183-184]. It will not select a layer/blend parameter with the same name. The getter returns false when scalar lookup fails and assigns `OutValue` only on success [MIC:1295-1303]. This verifies lookup compatibility, not that an imported material's graph actually drives emission with that parameter.

## Point-light calls

The proposed component inherits the following runtime calls [PH:19,61-62; LLH:52-57; LCH:286; SCH:912-919]:

```cpp
void UPointLightComponent::SetUseInverseSquaredFalloff(bool bNewValue);
void ULocalLightComponent::SetIntensityUnits(ELightUnits NewIntensityUnits);
void ULocalLightComponent::SetAttenuationRadius(float NewRadius);
void ULightComponent::SetIntensity(float NewIntensity);
void USceneComponent::SetVisibility(bool bNewVisibility,
    bool bPropagateToChildren = false);
```

`ELightUnits::Candelas` is a real branch in installed point-light brightness calculation when inverse-square falloff is enabled. UE applies its centimetre-to-metre factor internally. Do not add that factor again to a value already expressed in candelas [PC:127-156]. The older point-light property comment that always describes enabled falloff as lumens is less precise than this implementation [PH:23-30; LLH:22-28].

Set units and inverse-square mode explicitly before assigning the saved intensity. Units, falloff and intensity setters require dynamic data changes to be allowed. The radius setter uses the stricter check that excludes registered stationary lights as well as static ones. The proposed movable light satisfies that mobility condition [PC:67-74; LLC:20-39; LCC:1076-1087; SCH:1363-1365]. Radius is a cutoff for visible influence, not intensity or a saved furniture scale [LLH:41-46]. Keep it non-visible during preparation and invalidation. Light render-state creation checks scene/render eligibility and valid intensity before adding visible light output [LCC:964-979].

## Evidence register

All paths below are on Cribtest11. Every SHA-256 covers the entire installed file, not just its excerpts.

| Ref | Exact path | SHA-256 |
| --- | --- | --- |
| V | `C:\Program Files\Epic Games\UE_5.8\Engine\Build\Build.version` | `eab58750e84719489f3ee0d42d05a533ceb357eb3ee511777e9408d27a85f43c` |
| W | `C:\Program Files\Epic Games\UE_5.8\Engine\Source\Runtime\Engine\Classes\Engine\World.h` | `bb193a298086bbf10acfc9793071180dab3055dd4e5c66fbdd7f45ad8e01dff6` |
| LA | `C:\Program Files\Epic Games\UE_5.8\Engine\Source\Runtime\Engine\Private\LevelActor.cpp` | `cab64fb71b9ab5397ddef5beb661ae3179cff1c0d71df394e82c8f5148146a86` |
| AH | `C:\Program Files\Epic Games\UE_5.8\Engine\Source\Runtime\Engine\Classes\GameFramework\Actor.h` | `2ee1effb7ab4ac4cf65cf6caa90a27cea45361feefaae0cb673f7265e00bc5c9` |
| AC | `C:\Program Files\Epic Games\UE_5.8\Engine\Source\Runtime\Engine\Private\Actor.cpp` | `53022fa18c2e33a389fdfa8cc49e63f2795eef648fb755157f78625965e6f329` |
| LH | `C:\Program Files\Epic Games\UE_5.8\Engine\Source\Runtime\Engine\Classes\Engine\Level.h` | `85b8cd38abbc5219a646374a9321b5c971799dd19ecdf75789440c03c8838a15` |
| LC | `C:\Program Files\Epic Games\UE_5.8\Engine\Source\Runtime\Engine\Private\Level.cpp` | `59729ccbc23f83aa9cdee3ef1ba6e95c268e3a7b8a2ed437559574ebc020e0ce` |
| WC | `C:\Program Files\Epic Games\UE_5.8\Engine\Source\Runtime\Engine\Private\World.cpp` | `15dbeb16cd7e11060231eb9ca4f3017ee0faf507d625198012d089979cb74748` |
| LU | `C:\Program Files\Epic Games\UE_5.8\Engine\Source\Runtime\Engine\Private\LevelUtils.cpp` | `1dcddf45d9481d7bd2682391b872b316e5c4d81251acf04a76c970f935f0c5c3` |
| SLH | `C:\Program Files\Epic Games\UE_5.8\Engine\Source\Runtime\Engine\Classes\Engine\LevelStreaming.h` | `c3f82e44bdaca02da0e88249c3294033ae341819c0eb35bbbd6d0413503aaa99` |
| SLC | `C:\Program Files\Epic Games\UE_5.8\Engine\Source\Runtime\Engine\Private\LevelStreaming.cpp` | `9c6ff1eb62a913387845629b8a880bb84e37fd741b29391381fbf7c18f3e6197` |
| SH | `C:\Program Files\Epic Games\UE_5.8\Engine\Source\Runtime\Engine\Classes\Engine\StreamableManager.h` | `0c9562bb00445398060d1768a9259aef51ea264445bfe1fc8295bd22b928df59` |
| SC | `C:\Program Files\Epic Games\UE_5.8\Engine\Source\Runtime\Engine\Private\StreamableManager.cpp` | `dadd785d8c217d6993984a2fd3cc27a511f82b59de8f8091fb57d6b6e4458e83` |
| AM | `C:\Program Files\Epic Games\UE_5.8\Engine\Source\Runtime\Engine\Classes\Engine\AssetManager.h` | `31d748d2459b29dd2301b5007fe2a1f4b7bf861c09364b4e3f18181bebb31cdb` |
| SMH | `C:\Program Files\Epic Games\UE_5.8\Engine\Source\Runtime\Engine\Classes\Components\StaticMeshComponent.h` | `7582e57da6a4ed56e920618bfa7dbbb6e15645e69774d3d684f9d24c97a98ddc` |
| SMC | `C:\Program Files\Epic Games\UE_5.8\Engine\Source\Runtime\Engine\Private\Components\StaticMeshComponent.cpp` | `c2dda531aab04fca86fbbd7fcffb66f3dcf1d331e9b5bdff3e0703b0614014e9` |
| PRH | `C:\Program Files\Epic Games\UE_5.8\Engine\Source\Runtime\Engine\Classes\Components\PrimitiveComponent.h` | `814841dbc22d80d3ef9d73d87dcbbf8b1e4905bb0fd5b6a34a8c4d64509b939e` |
| PRC | `C:\Program Files\Epic Games\UE_5.8\Engine\Source\Runtime\Engine\Private\Components\PrimitiveComponent.cpp` | `5ce231cfef144d2263883a495290108fa7fee0162ed4ed99912c4e34377b39ee` |
| MH | `C:\Program Files\Epic Games\UE_5.8\Engine\Source\Runtime\Engine\Classes\Components\MeshComponent.h` | `77a229264c7fd057f8010fe77b29f9f1ff9ba0118a08c1261b6ce2562ce151e5` |
| MC | `C:\Program Files\Epic Games\UE_5.8\Engine\Source\Runtime\Engine\Private\Components\MeshComponent.cpp` | `5c47d2fbe83e083476555c21e88ae07a6c6f3d062a716a6f76b124d91517f55e` |
| MI | `C:\Program Files\Epic Games\UE_5.8\Engine\Source\Runtime\Engine\Public\Materials\MaterialInterface.h` | `bea770f73328fb86c54d3ccd7199925d99dc4d5954d6424981af3b6dd15d66c5` |
| MIC | `C:\Program Files\Epic Games\UE_5.8\Engine\Source\Runtime\Engine\Private\Materials\MaterialInterface.cpp` | `6b4638ee9abb023941251584215584f5062490e9d8f1f3c5b029b65c6f3e4ad7` |
| MP | `C:\Program Files\Epic Games\UE_5.8\Engine\Source\Runtime\Engine\Public\Materials\MaterialParameters.h` | `854852d8c6cc9b48ef2d89bd1de689d1d9bd1c460ce0118c6de114ccf456803c` |
| PH | `C:\Program Files\Epic Games\UE_5.8\Engine\Source\Runtime\Engine\Classes\Components\PointLightComponent.h` | `3a813731a610151ef77030e47abd58916b0c414734fa3aa898c450aaeb3ee9d2` |
| PC | `C:\Program Files\Epic Games\UE_5.8\Engine\Source\Runtime\Engine\Private\Components\PointLightComponent.cpp` | `6b5c8bc32f72bd66e1bf0d0618f9261710be95dd34130176c085cfefe9837fa4` |
| LLH | `C:\Program Files\Epic Games\UE_5.8\Engine\Source\Runtime\Engine\Classes\Components\LocalLightComponent.h` | `6d4bc71587fa508d899391335e428b08499c792a08620e88e75bfe6aff66c0a7` |
| LLC | `C:\Program Files\Epic Games\UE_5.8\Engine\Source\Runtime\Engine\Private\Components\LocalLightComponent.cpp` | `074c3493d28b7a81219a6ddea3b297409abb2a29a156e1da960273d5804675be` |
| LCH | `C:\Program Files\Epic Games\UE_5.8\Engine\Source\Runtime\Engine\Classes\Components\LightComponent.h` | `29222ef3b3afedef5c3b07db0c7375179172e3c5c9698ff75db778b1e6bad815` |
| LCC | `C:\Program Files\Epic Games\UE_5.8\Engine\Source\Runtime\Engine\Private\Components\LightComponent.cpp` | `016f92005cbfd09f3505f54dbb46a73ba836237c2e81ab437382d4de0bb13c82` |
| SCH | `C:\Program Files\Epic Games\UE_5.8\Engine\Source\Runtime\Engine\Classes\Components\SceneComponent.h` | `a9e1cecc43547fe2dc10bae4cfdcb49d3951e89717c16c8c3e1bd0831030b1d5` |
| SCC | `C:\Program Files\Epic Games\UE_5.8\Engine\Source\Runtime\Engine\Private\Components\SceneComponent.cpp` | `58c6337430d41c3fb9acd8060221a8b46696e0f372c2cc590bc2b865ed89b757` |

These findings establish installed source signatures and identify integration obligations. They do not establish compiled code, imported furniture fidelity, a visible apartment, successful gameplay admission or a playable result.

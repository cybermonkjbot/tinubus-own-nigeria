import { useEffect, useImperativeHandle, useRef, useState, type Ref } from "react";
import * as THREE from "three";
import { createSceneLoop, createViewVolume } from "../../../packages/character-runtime/src/performance";
import { createCharacterAssetSlot, type CharacterAssetState } from "./interactions/appearance/characterAsset";
import { createSoloRig } from "./interactions/appearance/soloRig";
import { captureLocalTransforms, type LocalTransformSnapshot } from "./interactions/scenes/objectTransformSnapshot";
import { createWardrobeRig } from "./interactions/appearance/wardrobeRig";
import { useCompanionAssets } from "./interactions/appearance/CompanionAssets";
import { createRendererAssetDelivery } from "./interactions/appearance/gpuTextureDelivery";
import { ResourceDownloadState, useResourceDownloadProgress } from "./interactions/appearance/ResourceDownloadState";
import { createCharacterExpressionMemory, createExpressionRig, unavailableExpressionCapabilities, type CharacterExpressionCapabilities, type CharacterExpressionEvent, type CharacterExpressionTarget } from "./interactions/appearance/expressionRig";
import type { CharacterCommand } from "../../../packages/utils/src/companionExpression";
import type { CompanionCareReceipt } from "../../../packages/utils/src/companionLife";
import { usesBoundedCamera, type CompanionEnvironment } from "./interactions/scenes/environments";
import { createEnvironmentSlot, type EnvironmentLoadState } from "./interactions/scenes/environmentSlot";
import { createTownSquareLighting } from "./interactions/scenes/townSquareLighting";
import { createRigActivity, type RigActivity } from "./interactions/scenes/rigActivity";
import { routeSegmentClear } from "./interactions/scenes/navigation";
import type { ScenePoint } from "./interactions/scenes/environments";
import { createRoomCamera, type RoomCameraControl } from "./interactions/scenes/roomCamera";
import { createBaseRig } from "./interactions/appearance/baseRig";
import type { CompanionAppearance } from "../../../packages/utils/src/companion";
import { companionBodyColor } from "../../../packages/utils/src/companionAppearance";
import { createShopEquipment, type ShopAssetState } from "./interactions/shop/items3d";
import type { ShopEquipment } from "./interactions/shop/catalog";
import type { CompanionSceneRendererState, CompanionSceneRoomUtilities } from "../../../packages/utils/src/companionSceneBridge";
import { projectCompanionWorldUtilities } from "../../../packages/utils/src/companionWorldUtilities";
import type { FurnitureSceneAttachment } from "./native-scene/furnitureInteraction";
import type { CompanionFurniture } from "../../../packages/utils/src/companion";
import { directionalWalk } from "./interactions/scenes/directionalWalk";
import type { WalkDirection } from "./interactions/scenes/WorldDpad";
import type { CompanionPace } from "../../../packages/utils/src/companionMovement";
import { createWorldSoundDesign } from "./interactions/audio/worldSoundDesign";
import { createWorldSoundClock } from "./interactions/audio/worldSoundClock";
import { useWorldSoundPreference } from "./interactions/audio/worldSoundPreference";
import type { PlayPresentation } from "../../../packages/utils/src/companionPlay";
import { WorldSoundControl, attachWorldSoundInput, type WorldSoundDesign } from "./interactions/audio/WorldSoundControl";
import { createSpeechFraming, type SpeechInsets } from "./interactions/scenes/speechFraming";
import { companionSpeechExcerpt } from "../../../packages/utils/src/companionVoicePresentation";
import "./interactions/scenes/speech-bubble.css";

export type CharacterState = "idle" | "listening" | "processing" | "speaking";
export type CharacterReaction = { id: number; kind: "nod" | "delight" | "greeting" };
export type CharacterHandle = { snapshotCompanionPng(): Promise<Blob>; pickCharacter(clientX: number, clientY: number): boolean; pickWalkDestination(clientX: number, clientY: number): ScenePoint | null; walkDirection(direction: WalkDirection, pace?: CompanionPace): ScenePoint | null };
export type CompanionCanvas = HTMLCanvasElement & CharacterHandle;
export type CharacterProps = CompanionAppearance & {
  ref?: Ref<CharacterHandle>;
  light: "daylight" | "golden" | "evening";
  animated: boolean;
  /** A covered scene keeps its resources and audio owner, but does not render. */
  renderActive?: boolean;
  greeting: number;
  rotation: number;
  state?: CharacterState;
  /** Normalized output-audio amplitude, not microphone input. Omit for silent speech. */
  speechLevel?: number;
  /** Current assistant reply only, never the user's transcript. */
  speechText?: string;
  speechInsets?: SpeechInsets;
  reaction?: CharacterReaction;
  /** Stable saved identity. Required before this instance accepts AI commands. */
  characterId?: string;
  expression?: CharacterCommand | null;
  expressionTargets?: readonly CharacterExpressionTarget[];
  /** User manipulation, editing or host-owned presentation takes priority. */
  interactionActive?: boolean;
  onExpressionCapabilities?: (capabilities: CharacterExpressionCapabilities) => void;
  onExpressionEvent?: (event: CharacterExpressionEvent, command: CharacterCommand) => void;
  framing?: "home" | "conversation" | "appearance";
  appearanceFocus?: "full-body" | "face";
  environment?: CompanionEnvironment;
  wideView?: boolean;
  cameraOrbit?: number;
  cameraControl?: RoomCameraControl;
  activity?: RigActivity;
  onCareEvent?: (receipt: CompanionCareReceipt) => void;
  onWalkEvent?: (id: number, status: "completed" | "blocked") => void;
  shopEquipment?: ShopEquipment;
  onRendererStateChange?: (state: CompanionSceneRendererState) => void;
  /** Native bridge input is valid only after this revision is rendered. */
  presentationRevision?: number;
  onPresentationReady?: (revision: number) => void;
  onSceneFrame?: (attachment: FurnitureSceneAttachment | null, seconds: number) => void;
  /** Capture only pixels from this completed draw, before the drawing buffer is discarded. */
  onRenderedFrame?: (canvas: HTMLCanvasElement, revision: number) => void;
  furnitureItems?: readonly CompanionFurniture[];
  roomUtilities?: CompanionSceneRoomUtilities | null;
  play?: PlayPresentation;
  worldSoundActive?: boolean;
  lightweight?: boolean;
};

export default function Character(props: CharacterProps) {
  const downloading = useResourceDownloadProgress() !== null;
  const assetDelivery = useCompanionAssets();
  const sourceDelivery = useRef(assetDelivery); sourceDelivery.current = assetDelivery;
  const host = useRef<HTMLDivElement>(null);
  const speechBubble = useRef<HTMLDivElement>(null);
  const capture = useRef<() => Promise<Blob>>(() => Promise.reject(new Error("Wait for Bestie to finish loading.")));
  const pickDestination = useRef<(x: number, y: number) => ScenePoint | null>(() => null);
  const pickCharacter = useRef<(x: number, y: number) => boolean>(() => false);
  const directionDestination = useRef<(direction: WalkDirection, pace?: CompanionPace) => ScenePoint | null>(() => null);
  useImperativeHandle(props.ref, () => ({ snapshotCompanionPng: () => capture.current(), pickCharacter: (x, y) => pickCharacter.current(x, y), pickWalkDestination: (x, y) => pickDestination.current(x, y), walkDirection: (direction, pace) => directionDestination.current(direction, pace) }), []);
  const latest = useRef(props);
  latest.current = props;
  const invalidate = useRef<() => void>(() => {});
  useEffect(() => { invalidate.current(); });
  const [error, setError] = useState(false);
  const [ready, setReady] = useState(false);
  const [worldSound, setWorldSound] = useState<WorldSoundDesign | null>(null);
  const soundPreference = useWorldSoundPreference(worldSound, `solo:${props.characterId ?? "local"}`);
  useEffect(() => { if (props.worldSoundActive === false || props.renderActive === false) worldSound?.stop(); },
    [worldSound, props.worldSoundActive, props.renderActive]);
  const soundChanged = useRef(soundPreference.publish); soundChanged.current = soundPreference.publish;
  const [rendererRetry, setRendererRetry] = useState(0);
  const [characterAssets, setCharacterAssets] = useState<CharacterAssetState>({ loading: false, version: null, error: null });
  const retryCharacterAssets = useRef<() => void>(() => {});
  const expressionMemory = useRef(createCharacterExpressionMemory());
  const [shopAssets, setShopAssets] = useState<ShopAssetState>({ loading: false, errors: [] });
  const [roomAssets, setRoomAssets] = useState<EnvironmentLoadState>({ loading: false, error: null });
  const retryRoomAssets = useRef<() => void>(() => {});
  const retryShopAssets = useRef<() => void>(() => {});
  useEffect(() => {
    const element = host.current;
    if (!element) return;
    let rendererState: CompanionSceneRendererState | undefined;
    let presentedRevision: number | undefined;
    const publishRendererState = (state: CompanionSceneRendererState) => {
      if (rendererState === state) return;
      rendererState = state;
      try { latest.current.onRendererStateChange?.(state); } catch { /* The scene owns its cleanup. */ }
    };
    publishRendererState("loading");
    setError(false); setReady(false);
    setCharacterAssets({ loading: false, version: null, error: null });
    const publishExpressionCapabilities = (capabilities: CharacterExpressionCapabilities) => {
      element.dataset.expressionCapabilities = JSON.stringify(capabilities);
      try { latest.current.onExpressionCapabilities?.(capabilities); } catch { /* Keep renderer ownership local. */ }
    };
    let renderer: THREE.WebGLRenderer;
    try { renderer = new THREE.WebGLRenderer({ alpha: true, antialias: false, powerPreference: "low-power", precision: "mediump" }); }
    catch { publishRendererState("unavailable"); publishExpressionCapabilities(unavailableExpressionCapabilities); setError(true); return; }
    renderer.setPixelRatio(Math.min(window.devicePixelRatio || 1, 1.0));
    const rendererAssets = createRendererAssetDelivery(renderer);
    const latestDelivery = { get current() { return rendererAssets.resolve(sourceDelivery.current); } };
    renderer.setClearColor(0x000000, 0);
    renderer.toneMapping = THREE.LinearToneMapping;
    renderer.toneMappingExposure = .95;
    renderer.shadowMap.enabled = false;
    renderer.shadowMap.autoUpdate = false;
    let shadowsDirty = false;
    element.appendChild(renderer.domElement);
    const sound = createWorldSoundDesign(); setWorldSound(sound);
    const releaseSoundInput = attachWorldSoundInput(element.parentElement ?? element, sound,
      () => !disposed && !rendererUnavailable && latest.current.worldSoundActive !== false
        && latest.current.renderActive !== false && !!roomSlot.world
        && !roomSlot.pending && latest.current.framing !== "appearance" && !latest.current.furnitureItems,
      () => { soundChanged.current(); invalidate.current(); });
    const scene = new THREE.Scene();
    const camera = new THREE.PerspectiveCamera(29, 1, .1, 100);
    const roomCamera = createRoomCamera();
    const speechFraming = createSpeechFraming(element, () => speechBubble.current);
    const viewVolume = createViewVolume(camera);
    camera.position.set(0, 3.25, 8.8); camera.lookAt(0, 2.05, 0);
    const ambient = new THREE.HemisphereLight("#fff4df", "#c2a994", 1.6);
    scene.add(ambient);
    const key = new THREE.DirectionalLight("#ffe0ba", 2.6);key.position.set(-3,5,4);scene.add(key);
    key.castShadow = false;
    const floor = new THREE.Mesh(new THREE.PlaneGeometry(12,12),new THREE.ShadowMaterial({opacity:.18}));
    floor.rotation.x = -Math.PI/2;floor.position.y = -.11;floor.receiveShadow = false;
    floor.visible = false;scene.add(floor);
    const fill = new THREE.DirectionalLight("#fff0db", 1.35);fill.position.set(2,1.3,4);scene.add(fill);
    const rim = new THREE.DirectionalLight("#fff4db", 2.3);rim.position.set(-2,4,-3);scene.add(rim);
    const townSquareLighting = createTownSquareLighting(scene, renderer, [ambient, key, fill, rim]);
    let model: THREE.Group | undefined;
    let restPose: LocalTransformSnapshot | undefined;
    let restMorphs = new Map<THREE.Mesh, number[]>();
    let baseRig: ReturnType<typeof createBaseRig> | undefined;
    let wardrobe: ReturnType<typeof createWardrobeRig> | undefined;
    let soloRig: ReturnType<typeof createSoloRig> | undefined;
    let shopEquipment: ReturnType<typeof createShopEquipment> | undefined;
    let equipmentState: ShopAssetState = { loading: false, errors: [] };
    let environment: THREE.Group | undefined;
    let environmentId: string | undefined;
    let environmentInstance: string | undefined;
    let followAfterCare = false;
    const roomSlot = createEnvironmentSlot(scene, status => {
      shadowsDirty = true; settleUntil = time + 1; setRoomAssets(status); invalidate.current();
      if (!roomSlot.group) publishRendererState(status.error ? "unavailable" : "loading");
    });
    retryRoomAssets.current = () => roomSlot.retry();
    let rigActivity: ReturnType<typeof createRigActivity> | undefined;
    let activityMoving = false;
    const raycaster = new THREE.Raycaster();
    pickCharacter.current = (x, y) => {
      const current = latest.current;
      if (!model || roomSlot.pending || rendererUnavailable || disposed || !current.animated || current.interactionActive
        || current.renderActive === false || current.framing === "appearance") return false;
      if (current.presentationRevision !== undefined && presentedRevision !== current.presentationRevision) return false;
      const rect = renderer.domElement.getBoundingClientRect();
      if (!rect.width || !rect.height) return false;
      raycaster.setFromCamera(new THREE.Vector2((x - rect.left) / rect.width * 2 - 1, 1 - (y - rect.top) / rect.height * 2), camera);
      const hit = raycaster.intersectObjects([model, ...(roomSlot.group ? [roomSlot.group] : [])], true).find(value => {
        for (let node: THREE.Object3D | null = value.object; node; node = node.parent) if (!node.visible) return false;
        const mesh = value.object as THREE.Mesh;
        const material = Array.isArray(mesh.material) ? mesh.material[value.face?.materialIndex ?? 0] : mesh.material;
        return material?.visible !== false;
      });
      for (let node: THREE.Object3D | null = hit?.object ?? null; node; node = node.parent) if (node === model) return true;
      return false;
    };
    directionDestination.current = (direction, pace) => {
      const nav = roomSlot.world?.navigation;
      if (!nav || !model || roomSlot.pending || !latest.current.animated || latest.current.interactionActive || latest.current.framing === "appearance" || latest.current.renderActive === false) return null;
      if (latest.current.presentationRevision !== undefined && presentedRevision !== latest.current.presentationRevision) return null;
      return directionalWalk(camera, model.position, nav, direction, pace);
    };
    pickDestination.current = (x, y) => {
      const nav = roomSlot.world?.navigation;
      const visibleRoom = roomSlot.group;
      if (latest.current.presentationRevision !== undefined && presentedRevision !== latest.current.presentationRevision) return null;
      if (roomSlot.pending || !nav || !visibleRoom || !latest.current.animated || latest.current.framing === "appearance") return null;
      const rect = renderer.domElement.getBoundingClientRect();
      if (!rect.width || !rect.height) return null;
      raycaster.setFromCamera(new THREE.Vector2((x - rect.left) / rect.width * 2 - 1, 1 - (y - rect.top) / rect.height * 2), camera);
      // Only the nearest visible floor hit is a walk request, never a wall or furniture click.
      const hit = raycaster.intersectObject(visibleRoom, true).find(value => {
        for (let node: THREE.Object3D | null = value.object; node; node = node.parent) if (!node.visible) return false;
        const mesh = value.object as THREE.Mesh;
        const material = Array.isArray(mesh.material) ? mesh.material[value.face?.materialIndex ?? 0] : mesh.material;
        return material?.visible !== false;
      });
      if (!hit?.face || Math.abs(hit.point.y - nav.floorY) > .06) return null;
      const normal = hit.face.normal.clone().applyNormalMatrix(new THREE.Matrix3().getNormalMatrix(hit.object.matrixWorld));
      if (normal.y < .9) return null;
      const point: ScenePoint = [hit.point.x, nav.floorY, hit.point.z];
      return routeSegmentClear(nav, point, point) ? point : null;
    };
    let mixer: THREE.AnimationMixer | undefined;
    let idle: THREE.AnimationAction | undefined;
    let blink: THREE.AnimationAction | undefined;
    let greeting: THREE.AnimationAction | undefined;
    let disposed = false, time = 0, nextBlink = 2.5;
    let previousGreeting = latest.current.greeting;
    let previousAnimated = latest.current.animated;
    let greetingEnd = 0;
    let previousReaction = latest.current.reaction?.id;
    let reactionStart = -10;
    let reactionKind: CharacterReaction["kind"] = "nod";
    let headResponse: THREE.Group | undefined;
    let mouth: THREE.Object3D | undefined;
    let smile: THREE.Object3D | undefined;
    let mouthRestScaleY = 1;
    let mouthLevel = 0;
    let expressionRig: ReturnType<typeof createExpressionRig> | undefined;
    let pendingExpressionReceipt: { event: CharacterExpressionEvent; command: CharacterCommand } | undefined;
    let rendererUnavailable = false;
    let previousRotation = latest.current.rotation;
    let expressionDragUntil = 0;
    const cameraTarget = new THREE.Vector3(0, 2.05, 0);
    const desiredCamera = new THREE.Vector3();
    const desiredTarget = new THREE.Vector3();
    const careFocus = new THREE.Vector3();
    const speechFocus = new THREE.Vector3(), speechOffset = new THREE.Vector3();
    let previousSpeech = "";
    let previousAppearance = "", settleUntil = 1;
    let appearanceInput: CharacterProps | undefined;
    let fabric: THREE.MeshStandardMaterial[] = [], ribbing: THREE.MeshStandardMaterial[] = [];
    let appearance: { material: THREE.MeshStandardMaterial; original: THREE.Color; slot: "bodyColor" | "topColor" | "trousersColor" | "footwearColor" }[] = [];
    const targetColor = new THREE.Color();
    const rose = new THREE.Color("#b97e74"), ivory = new THREE.Color("#e7dfcc");
    const roseRib = new THREE.Color("#95685f"), ivoryRib = new THREE.Color("#c6bca5");
    const warm = new THREE.Color("#ffc58f"), daylight = new THREE.Color("#ffe0ba"), evening = new THREE.Color("#bfc8ff");
    const characterSlot = createCharacterAssetSlot(gltf => {
      const next: ReturnType<typeof createSoloRig> = createSoloRig(gltf, { memory: expressionMemory.current, onExpressionEvent: (event, command) => {
        if (disposed || soloRig !== next) return;
        if (event.status === "applied") pendingExpressionReceipt = { event, command };
        else {
          if (pendingExpressionReceipt?.command.requestId === command.requestId) pendingExpressionReceipt = undefined;
          element.dataset.expressionStatus = event.reason ?? event.status;
          try { latest.current.onExpressionEvent?.(event, command); } catch { /* A host receipt must not interrupt rig disposal. */ }
        }
      } });
      try {
        const current = latest.current, world = roomSlot.world;
        next.baseRig.apply(current.baseId); next.wardrobe.apply(current);
        next.model.scale.setScalar(world?.characterScale ?? 1);
        if (model) {
          next.model.position.copy(model.position); next.model.quaternion.copy(model.quaternion);
          if (rigActivity) next.rigActivity.restoreProgress(rigActivity.captureProgress());
        } else {
          if (world) next.rigActivity.reset(world.spawn);
          next.model.rotation.y = world?.spawnYaw ?? current.rotation;
        }
        next.appearance.forEach(({ material, original, slot }) => {
          const value = slot === "bodyColor" ? companionBodyColor(current) : current[slot];
          if (value && /^#[0-9a-f]{6}$/i.test(value)) material.color.set(value);
          else if (slot === "topColor") material.color.copy(material.name === "Jacket ribbing" ? (current.look === "rose" ? roseRib : ivoryRib) : (current.look === "rose" ? rose : ivory));
          else material.color.copy(original);
        });
        next.faceRig.apply(current);
        if (current.animated && next.idle) { next.idle.play(); next.idle.time = idle?.time ?? 0; }
        if (current.animated && blink?.isRunning() && next.blink) { next.blink.play(); next.blink.time = blink.time; }
        if (current.animated && greetingEnd > time && next.greeting) { next.greeting.play(); next.greeting.time = greeting?.time ?? 0; }
        next.mixer.update(0);
        // Move the existing equipment, including pending attachments, only
        // after the candidate's controls and appearance are ready.
        shopEquipment?.rebind(next.model);
      } catch (reason) { next.dispose(); throw reason; }
      const previous = soloRig;
      expressionRig?.cancel("character-updated", true); pendingExpressionReceipt = undefined;
      soloRig = next;
      ({ model, restPose, restMorphs, baseRig, wardrobe, rigActivity, mixer, idle, blink, greeting,
        headResponse, mouth, smile, mouthRestScaleY, fabric, ribbing, appearance, expressionRig } = next);
      scene.add(model); previous?.dispose();
      if (!shopEquipment) shopEquipment = createShopEquipment(model, status => {
        if (disposed) return;
        shadowsDirty = true; settleUntil = time + 1; equipmentState = status; setShopAssets(status); invalidate.current();
      }, () => latestDelivery.current);
      retryShopAssets.current = () => shopEquipment?.retry();
      settleUntil = time + 1;
      shadowsDirty = true; invalidate.current();
      setReady(true);
    }, status => {
      if (disposed) return;
      setCharacterAssets(status); settleUntil = time + 1; invalidate.current();
      if (!model) publishRendererState(status.error ? "unavailable" : "loading");
      element.dataset.characterAsset = status.version ?? "loading";
      publishExpressionCapabilities(expressionRig?.capabilities ?? unavailableExpressionCapabilities);
    });
    retryCharacterAssets.current = () => characterSlot.retry();
    characterSlot.request(latestDelivery.current);
    const snapshotCompanionPng = (): Promise<Blob> => {
      if (disposed || !model || renderer.getContext().isContextLost()) return Promise.reject(new Error("Wait for Bestie to finish loading."));
      shopEquipment?.apply(latest.current.shopEquipment);
      if (equipmentState.loading || equipmentState.errors.length) return Promise.reject(new Error("Wait for the outfit to load or retry its items."));
      const livePose = captureLocalTransforms(model, true);
      const liveMorphs = new Map<THREE.Mesh, number[]>();
      restMorphs.forEach((_, node) => { if (node.morphTargetInfluences) liveMorphs.set(node, [...node.morphTargetInfluences]); });
      const colors = appearance.map(({ material }) => material.color.clone());
      const background = scene.background;
      const floorVisible = floor.visible;
      const snapshotEnvironment = roomSlot.group;
      const environmentVisible = snapshotEnvironment?.visible;
      const renderSize = renderer.getSize(new THREE.Vector2());
      const pixelRatio = renderer.getPixelRatio();
      const canvas = document.createElement("canvas");
      canvas.width = 512; canvas.height = 512;
      try {
        restPose?.restore();
        restMorphs.forEach((weights, node) => { node.morphTargetInfluences?.splice(0, weights.length, ...weights); });
        baseRig?.apply(latest.current.baseId === "bear" ? "bestie" : "bear");
        baseRig?.apply(latest.current.baseId);
        wardrobe?.apply(latest.current, true);
        model.position.set(0, 0, 0); model.rotation.set(0, 0, 0); model.scale.setScalar(1);
        model.visible = true;
        appearance.forEach(({ material, original, slot }) => {
          const value = slot === "bodyColor" ? companionBodyColor(latest.current) : latest.current[slot];
          if (value && /^#[0-9a-f]{6}$/i.test(value)) material.color.set(value);
          else if (slot === "topColor") material.color.copy(material.name === "Jacket ribbing" ? (latest.current.look === "rose" ? roseRib : ivoryRib) : (latest.current.look === "rose" ? rose : ivory));
          else material.color.copy(original);
        });
        soloRig?.faceRig.apply(latest.current, true);
        if (mouth) mouth.visible = false;
        if (smile) smile.visible = true;
        scene.background = null; floor.visible = false;
        if (snapshotEnvironment) snapshotEnvironment.visible = false;
        model.updateMatrixWorld(true);
        const bounds = new THREE.Box3().setFromObject(model);
        const center = bounds.getCenter(new THREE.Vector3());
        const dimensions = bounds.getSize(new THREE.Vector3());
        const half = Math.max(dimensions.x, dimensions.y) * .55;
        const portraitCamera = new THREE.OrthographicCamera(-half, half, half, -half, .1, 100);
        portraitCamera.position.set(center.x, center.y, bounds.max.z + 10);
        portraitCamera.lookAt(center);
        renderer.setPixelRatio(1); renderer.setSize(512, 512, false);
        renderer.setClearColor(0x000000, 0);
        renderer.shadowMap.needsUpdate = false;
        renderer.render(scene, portraitCamera);
        const context = canvas.getContext("2d");
        if (!context) throw new Error("Couldn't capture the character image.");
        context.drawImage(renderer.domElement, 0, 0);
      } catch (reason) {
        return Promise.reject(reason);
      } finally {
        livePose.restore(); livePose.dispose();
        liveMorphs.forEach((weights, node) => { node.morphTargetInfluences?.splice(0, weights.length, ...weights); });
        appearance.forEach(({ material }, index) => material.color.copy(colors[index]));
        scene.background = background; floor.visible = floorVisible;
        if (snapshotEnvironment && environmentVisible !== undefined) snapshotEnvironment.visible = environmentVisible;
        renderer.setPixelRatio(pixelRatio); renderer.setSize(renderSize.x, renderSize.y, false);
        renderer.shadowMap.needsUpdate = false;
        renderer.render(scene, camera); shadowsDirty = true; invalidate.current();
      }
      return new Promise((resolve, reject) => canvas.toBlob(blob => blob ? resolve(blob) : reject(new Error("Couldn't export the character PNG.")), "image/png"));
    };
    capture.current = snapshotCompanionPng;
    (renderer.domElement as CompanionCanvas).snapshotCompanionPng = snapshotCompanionPng;
    (renderer.domElement as CompanionCanvas).pickCharacter = (x, y) => pickCharacter.current(x, y);
    (renderer.domElement as CompanionCanvas).pickWalkDestination = (x, y) => pickDestination.current(x, y);
    (renderer.domElement as CompanionCanvas).walkDirection = direction => directionDestination.current(direction);
    let hasViewport = false;
    const resize = () => {
      const width = element.clientWidth, height = element.clientHeight;
      hasViewport = width > 0 && height > 0;
      if (!hasViewport) { presentedRevision = undefined; publishRendererState("loading"); return; }
      renderer.setSize(width, height, false); camera.aspect = width / height;
      // Keep the full body in frame on either phone. The canvas uses the whole scene.
      camera.updateProjectionMatrix();
      settleUntil = time + .5;
      invalidate.current();
    };
    const observer = new ResizeObserver(resize);observer.observe(element);resize();
    const contextLost = (event: Event) => {
      event.preventDefault(); rendererUnavailable = true;
      soundClock.dispose(); sound.dispose(); setWorldSound(null);
      loop.dispose();
      rigActivity?.cancel(latest.current.activity?.kind === "care"); activityMoving = false;
      expressionRig?.cancel("renderer-unavailable", true);
      shopEquipment?.dispose(); soloRig?.dispose(); characterSlot.dispose();
      element.dataset.expressionActive = "false";
      publishExpressionCapabilities(unavailableExpressionCapabilities); setError(true);
      publishRendererState("unavailable");
    };
    renderer.domElement.addEventListener("webglcontextlost", contextLost);
    const expressionFrame = (blocked?: string) => ({ characterId: latest.current.characterId, command: latest.current.expression,
      targets: latest.current.expressionTargets, animated: latest.current.animated, now: Date.now(), blocked });
    const visibilityChanged = () => {
      sound.stop();
      rigActivity?.cancel(latest.current.activity?.kind === "care"); activityMoving = false;
      expressionRig?.beginFrame(expressionFrame("visibility-changed"));
      settleUntil = time + 1;
    };
    document.addEventListener("visibilitychange", visibilityChanged);
    let sampleTime = 0, samples = 0, sampleCost = 0;
    let furnitureGroup: THREE.Group | undefined;
    const furniture = new Map<string, THREE.Object3D>();
    let utilityReceipt: { scope: string; sampledAt: number; receivedAt: number } | null = null;
    let soundUtilities: ReturnType<typeof projectCompanionWorldUtilities> | null = null;
    const syncRoomUtilities = (animated: boolean, framing: string) => {
      soundUtilities = null;
      const admitted = framing !== "appearance" ? latest.current.roomUtilities : null;
      if (!admitted) {
        if (!utilityReceipt) return false;
        utilityReceipt = null;
        return roomSlot.clearUtilities();
      }
      const { sample, observation } = admitted;
      const scope = JSON.stringify([admitted.roomKey, sample.scope]);
      if (JSON.stringify(sample.scope) !== JSON.stringify(observation.scope)) return roomSlot.clearUtilities();
      const now = performance.now();
      let restored = false;
      if (utilityReceipt && utilityReceipt.scope !== scope) restored = roomSlot.clearUtilities();
      if (!utilityReceipt || utilityReceipt.scope !== scope || sample.sampledAtRealMs > utilityReceipt.sampledAt) {
        utilityReceipt = { scope, sampledAt: sample.sampledAtRealMs, receivedAt: now };
      }
      if (sample.sampledAtRealMs < utilityReceipt.sampledAt || now < utilityReceipt.receivedAt) return roomSlot.clearUtilities();
      try {
        soundUtilities = projectCompanionWorldUtilities(observation,
          utilityReceipt.sampledAt + Math.floor(now - utilityReceipt.receivedAt));
        roomSlot.setUtilities(soundUtilities);
      } catch { return roomSlot.clearUtilities() || restored; }
      return roomSlot.updateUtilities(!animated, !document.hidden && latest.current.renderActive !== false) || restored;
    };
    const tick = (dt: number, _now: number, viewportChanged: boolean) => {
      if (disposed || rendererUnavailable || !hasViewport) return false;
      time += dt;
      rigActivity?.prepareFrame();
      const { look, light, animated, rotation, state = "idle", framing = "home", speechLevel, reaction } = latest.current;
      // Reset clip ownership before a fresh semantic greeting can start.
      if (animated !== previousAnimated) {
        expressionRig?.prepareAnimationReset();
        mixer?.stopAllAction();
        if (animated) idle?.reset().play();
        previousAnimated = animated; greetingEnd = 0;
      }
      const expressionNow = Date.now();
      if (Math.abs(rotation - previousRotation) > .0001) expressionDragUntil = expressionNow + 450;
      previousRotation = rotation;
      const userReaction = latest.current.greeting !== previousGreeting || !!reaction && reaction.id !== previousReaction;
      characterSlot.request(latestDelivery.current);
      roomSlot.request(latest.current.environment, latestDelivery.current);
      if (roomSlot.pending) sound.stop();
      const utilityChanged = syncRoomUtilities(animated, framing);
      expressionRig?.beginFrame(expressionFrame(roomSlot.pending ? "room-loading" : latest.current.interactionActive || expressionNow < expressionDragUntil ? "user-control"
        : userReaction || greetingEnd > time || time - reactionStart < 1.15 ? "user-reaction" : activityMoving && !!latest.current.activity || latest.current.activity?.kind === "care" ? "character-activity" : framing === "appearance" ? "appearance-editor" : undefined));
      const world = roomSlot.world;
      townSquareLighting.setActive(world?.id === "town-square");
      if (world?.id !== environmentId || environment !== roomSlot.group) {
        environmentId = world?.id;
        environment = roomSlot.group;
        scene.background = world ? new THREE.Color(world.background) : null;
        floor.visible = !world;
        const instance = world?.instanceId ?? world?.id;
        if (instance !== environmentInstance) { followAfterCare = false; rigActivity?.reset(world?.spawn); if (model && world?.spawnYaw !== undefined) model.rotation.y = world.spawnYaw; }
        environmentInstance = instance;
        if (model && !world) model.position.set(0, 0, 0);
        settleUntil = time + 1;
      }
      if (model) {
        model.scale.setScalar(world?.characterScale ?? 1);
        baseRig?.apply(latest.current.baseId);
        wardrobe?.apply(latest.current);
      }
      if (world?.navigation && rigActivity?.reconcileNavigation(world.navigation,
        [...(world.spawn ? [world.spawn] : []), ...world.walkingPoints], latest.current.activity?.id)) {
        activityMoving = false; settleUntil = time + 1;
        expressionRig?.cancel("room-position-changed", true);
      }
      // Audio samples must not keep reduced-motion rendering alive.
      if (appearanceInput !== latest.current) {
        appearanceInput = latest.current;
        const appearanceKey = JSON.stringify({...appearanceInput, speechLevel: undefined, speechText: undefined, expression: undefined, expressionTargets: undefined,
          roomUtilities: appearanceInput.roomUtilities ? [appearanceInput.roomUtilities.roomKey, appearanceInput.roomUtilities.sample.scope] : undefined});
        if (appearanceKey !== previousAppearance) { previousAppearance = appearanceKey; settleUntil = time + 1; shadowsDirty = true; }
      }
      const speech = state === "speaking" && !latest.current.interactionActive && !roomSlot.pending
        && framing !== "appearance" && !latest.current.furnitureItems ? latest.current.speechText ?? "" : "";
      if (speech !== previousSpeech) { previousSpeech = speech; settleUntil = time + .1; }
      if (utilityChanged) { settleUntil = time + (animated ? 1 : 0); shadowsDirty = true; }
      if (!animated && !viewportChanged && time > settleUntil && !latest.current.onSceneFrame) {
        element.dataset.renderStats = JSON.stringify({drawCalls:renderer.info.render.calls,triangles:renderer.info.render.triangles,framesPerSecond:0,pixelRatio:renderer.getPixelRatio(),state:"still",yaw:Math.round((model?.rotation.y??0)*180/Math.PI)});
        return false;
      }
      const lerp = 1 - Math.exp(-dt * 9);
      const conversation = framing === "conversation";
      const appearanceView = framing === "appearance";
      const faceView = appearanceView && latest.current.appearanceFocus === "face";
      desiredCamera.set(0, faceView ? 3.8 : conversation ? 3.35 : 3.25, faceView ? 3.7 : conversation ? 7.0 : appearanceView ? 8.0 : 8.8);
      desiredTarget.set(0, faceView ? 3.55 : conversation ? 2.5 : appearanceView ? 2.22 : 2.05, 0);
      let controlledFov: number | undefined;
      if (world && latest.current.cameraControl) {
        const activity = latest.current.activity;
        const approaching = activity?.kind === "care" && activity.activity.phase === "approaching";
        const caring = activity?.kind === "care" && !approaching && activity.activity.endsAt > Date.now();
        if (approaching && model) followAfterCare = true;
        if (caring) {
          followAfterCare = true;
          careFocus.set(...activity.station.position);
          careFocus.y += activity.activity.kind === "sleep" ? .15 : .4;
          if (activity.activity.kind === "sleep") careFocus.z += .6;
        } else if (followAfterCare && model) {
          careFocus.copy(model.position);
          careFocus.y += world.characterScale * 1.9;
        }
        controlledFov = roomCamera.frame(world, camera.aspect, !speech && latest.current.wideView !== false,
          latest.current.cameraControl, caring ? careFocus : model?.position, desiredCamera, desiredTarget, caring || followAfterCare ? careFocus : undefined);
      } else if (world) {
        const portraitWide = camera.aspect < .8 ? world.portraitWide : undefined;
        const view = latest.current.wideView === false ? world.close : portraitWide ?? world.wide;
        desiredTarget.set(...view.target);
        if (portraitWide && model && latest.current.wideView !== false) {
          desiredTarget.x = model.position.x; desiredTarget.z = model.position.z;
        }
        const orbit = world.cameraOrbitLimit === undefined ? latest.current.cameraOrbit ?? 0
          : THREE.MathUtils.clamp(latest.current.cameraOrbit ?? 0, -world.cameraOrbitLimit, world.cameraOrbitLimit);
        desiredCamera.set(...view.eye);
        if (usesBoundedCamera(world)) desiredTarget.sub(desiredCamera).applyAxisAngle(new THREE.Vector3(0, 1, 0), orbit).add(desiredCamera);
        else desiredCamera.sub(desiredTarget).applyAxisAngle(new THREE.Vector3(0, 1, 0), orbit).add(desiredTarget);
        if (model && latest.current.wideView === false) {
          const offset = new THREE.Vector3(model.position.x, 0, model.position.z).sub(new THREE.Vector3(...(world.spawn ?? [0, 0, .6])));
          desiredCamera.add(offset); desiredTarget.add(offset);
          if (usesBoundedCamera(world) && world.navigation) {
            const bounds = world.navigation.bounds;
            desiredCamera.x = THREE.MathUtils.clamp(desiredCamera.x, bounds.minX + .15, bounds.maxX - .15);
            desiredCamera.z = THREE.MathUtils.clamp(desiredCamera.z, bounds.minZ + .15, bounds.maxZ - .15);
          }
        }
      }
      if (speech && model && speechFraming.focus(model, previousAppearance, speechFocus)) {
        speechOffset.subVectors(desiredCamera, desiredTarget);
        desiredTarget.copy(speechFocus); desiredCamera.copy(speechFocus).add(speechOffset);
        if (world) roomCamera.contain(world, desiredCamera, desiredTarget);
      }
      const cameraFov = controlledFov ?? (world ? world.fov ?? 42 : 29);
      if (Math.abs(camera.fov - cameraFov) > .001) {
        camera.fov = animated && controlledFov !== undefined ? THREE.MathUtils.lerp(camera.fov, cameraFov, 1 - Math.exp(-dt * 7)) : cameraFov;
        camera.updateProjectionMatrix();
      }
      camera.position.lerp(desiredCamera, animated ? 1-Math.exp(-dt*5) : 1);
      cameraTarget.lerp(desiredTarget, animated ? 1-Math.exp(-dt*5) : 1);
      if (world && latest.current.cameraControl && (speech || latest.current.wideView === false || followAfterCare)) roomCamera.contain(world, camera.position, cameraTarget);
      camera.lookAt(cameraTarget);
      if (!speech || !model || !speechFraming.frame(camera, model, latest.current.speechInsets ?? { top: 0, bottom: 0 })) speechFraming.reset(camera);
      viewVolume.update();
      const characterVisible = !!model && viewVolume.character(model, 5);
      const poseActive = animated && characterVisible;
      if (model) model.visible = characterVisible;
      if (!characterVisible) { greeting?.stop(); greetingEnd = 0; reactionStart = -10; }
      if (!characterVisible) expressionRig?.beginFrame(expressionFrame("offscreen"));
      if (model) {
        shopEquipment?.apply(latest.current.shopEquipment);
        if ((latest.current.interactionActive && !latest.current.cameraControl) || expressionNow < expressionDragUntil || (!world && !activityMoving)) {
          model.rotation.y = THREE.MathUtils.lerp(model.rotation.y, rotation, lerp);
        }
        const customTop = latest.current.topColor && /^#[0-9a-f]{6}$/i.test(latest.current.topColor);
        if (!customTop) {
          fabric.forEach(material => material.color.lerp(look === "rose" ? rose : ivory, lerp));
          ribbing.forEach(material => material.color.lerp(look === "rose" ? roseRib : ivoryRib, lerp));
        }
        appearance.forEach(({material,original,slot}) => {
          const value = slot === "bodyColor" ? companionBodyColor(latest.current) : latest.current[slot];
          if (value && /^#[0-9a-f]{6}$/i.test(value)) material.color.lerp(targetColor.set(value),lerp);
          else if (slot !== "topColor") material.color.lerp(original,lerp);
        });
        soloRig?.faceRig.apply(latest.current);
      }
      const sceneLight = world?.lighting ?? light;
      key.color.lerp(sceneLight === "golden" ? warm : sceneLight === "evening" ? evening : daylight, lerp);
      key.intensity = THREE.MathUtils.lerp(key.intensity, sceneLight === "evening" ? 1.1 : 2.6, lerp);
      ambient.intensity = THREE.MathUtils.lerp(ambient.intensity, sceneLight === "evening" ? .85 : 1.6, lerp);
      if (latest.current.greeting !== previousGreeting) {
        previousGreeting = latest.current.greeting;
        if (poseActive) { greeting?.reset().setEffectiveTimeScale(1).setEffectiveWeight(1).fadeIn(.18).play(); greetingEnd = time + 2.05; }
      }
      if (reaction?.id !== previousReaction) {
        previousReaction = reaction?.id;
        if (!reaction) {
          reactionStart = -10;
          if (reactionKind === "greeting") { greeting?.stop(); greetingEnd = 0; }
        }
        if (reaction && poseActive) {
          reactionKind = reaction.kind;reactionStart = time;
          if (reaction.kind === "greeting") { greeting?.reset().setEffectiveTimeScale(1).setEffectiveWeight(1).fadeIn(.18).play();greetingEnd = time+2.05; }
        }
      }
      if (greetingEnd && time >= greetingEnd) { greeting?.fadeOut(.25); greetingEnd = 0; }
      if (poseActive) {
        if (time >= nextBlink) { blink?.reset().play(); nextBlink = time + 3.2 + Math.random() * 2.7; }
        mixer?.update(dt);
      }
      const bodyGesture = expressionRig?.applyBody() || undefined;
      rigActivity?.setGround(roomSlot.group);
      const manualActivityBlocked = latest.current.interactionActive && latest.current.activity?.kind !== "care" && !rigActivity?.hasCarePose;
      if (roomSlot.pending || manualActivityBlocked || framing === "appearance" || userReaction || greetingEnd > time) {
        rigActivity?.cancel(latest.current.activity?.kind === "care"); activityMoving = false;
        if (world) rigActivity?.ground(world.navigation, world.layout);
      } else if (world && rigActivity) activityMoving = rigActivity.update(dt, animated, latest.current.activity, world.navigation, world.layout, poseActive, bodyGesture);
      const phase = Math.max(0, Math.min(1, (time-reactionStart)/1.15));
      const pulse = animated ? Math.sin(phase*Math.PI)**2 : 0;
      const expression = expressionRig?.update(dt, camera.position);
      if (headResponse) {
        const nod = reactionKind === "nod" ? pulse*Math.sin(phase*Math.PI*2)*.14 : 0;
        const tilt = animated ? state === "listening" ? -.065 : state === "processing" ? .04 : 0 : 0;
        const automatic = 1 - (expression?.semanticWeight ?? 0);
        headResponse.rotation.x = THREE.MathUtils.lerp(headResponse.rotation.x, THREE.MathUtils.clamp(nod + (animated && state === "listening" ? .035 * automatic : 0) + (expression?.offset.x ?? 0), -.22, .22), animated ? lerp : 1);
        headResponse.rotation.y = THREE.MathUtils.lerp(headResponse.rotation.y, THREE.MathUtils.clamp(expression?.offset.y ?? 0, -.42, .42), animated ? lerp : 1);
        headResponse.rotation.z = THREE.MathUtils.lerp(headResponse.rotation.z, THREE.MathUtils.clamp(tilt * automatic + (reactionKind === "delight" ? pulse*-.09 : 0) + (expression?.offset.z ?? 0), -.18, .18), animated ? lerp : 1);
      }
      const level = poseActive && state === "speaking" && Number.isFinite(speechLevel)
        ? THREE.MathUtils.clamp(speechLevel ?? 0,0,1) : 0;
      mouthLevel = poseActive ? THREE.MathUtils.lerp(mouthLevel,level,1-Math.exp(-dt*(level>mouthLevel?25:15))) : 0;
      if (mouth) {mouth.visible = mouthLevel>.025;mouth.scale.y = mouthRestScaleY*(.12+mouthLevel*.88);}
      if (smile) smile.visible = mouthLevel<=.025;
      if (element.dataset.characterState !== state) element.dataset.characterState = state;
      if (element.dataset.framing !== framing) element.dataset.framing = framing;
      const currentEnv = world?.id ?? "portrait";
      if (element.dataset.environment !== currentEnv) element.dataset.environment = currentEnv;
      const currentActivity = activityMoving ? latest.current.activity?.kind ?? "idle" : "idle";
      if (element.dataset.activity !== currentActivity) element.dataset.activity = currentActivity;
      const mouthLevelStr = mouthLevel.toFixed(3);
      if (element.dataset.mouthLevel !== mouthLevelStr) element.dataset.mouthLevel = mouthLevelStr;
      const exprActiveStr = expression?.active ? "true" : "false";
      if (element.dataset.expressionActive !== exprActiveStr) element.dataset.expressionActive = exprActiveStr;
      const roomGroup = roomSlot.group;
      if (roomGroup !== furnitureGroup) {
        furnitureGroup = roomGroup; furniture.clear();
        roomGroup?.traverse(node => {
          const id = node.userData.companionFurnitureId;
          if (typeof id === "string" && latest.current.furnitureItems?.some(item => item.id === id)) furniture.set(id, node);
        });
      }
      latest.current.onSceneFrame?.(!roomSlot.pending && roomGroup ? { scene, camera, roomGroup, items: furniture, actorPosition: model?.position } : null, dt);
      const sceneryChanged = roomSlot.update(dt, model ? [{ x: model.position.x, z: model.position.z, radius: world?.navigation?.radius ?? .3 }] : null,
        animated && !document.hidden && latest.current.renderActive !== false && framing !== "appearance",
        { reducedMotion: !animated, visible: !document.hidden && latest.current.renderActive !== false });
      renderer.shadowMap.needsUpdate = false;
      const started = performance.now();renderer.render(scene, camera); shadowsDirty = false;
      if (model && !roomSlot.pending && latest.current.presentationRevision !== undefined)
        latest.current.onRenderedFrame?.(renderer.domElement, latest.current.presentationRevision);
      soundClock.frame();
      if (model && (!latest.current.environment || roomSlot.group)) publishRendererState("ready");
      if (model && !roomSlot.pending && latest.current.presentationRevision !== undefined && presentedRevision !== latest.current.presentationRevision) {
        presentedRevision = latest.current.presentationRevision;
        try { latest.current.onPresentationReady?.(presentedRevision); } catch { /* Presentation callbacks never own graphics. */ }
      }
      const walk = latest.current.activity;
      if (walk?.kind === "walk" && !roomSlot.pending && !manualActivityBlocked && framing !== "appearance" && rigActivity?.captureProgress().completed === walk.id && element.dataset.walkReceipt !== String(walk.id)) {
        element.dataset.walkReceipt = String(walk.id);
        const arrived = model && Math.hypot(model.position.x - walk.destination[0], model.position.z - walk.destination[2]) < .12;
        latest.current.onWalkEvent?.(walk.id, arrived ? "completed" : "blocked");
      }
      const careReceipt = rigActivity?.takeCareReceipt();
      if (careReceipt && !roomSlot.pending && latest.current.activity?.kind === "care" && latest.current.activity.activity.id === careReceipt.activityId) {
        latest.current.onCareEvent?.(careReceipt);
      }
      if (pendingExpressionReceipt && characterVisible && !rendererUnavailable && expression?.active === pendingExpressionReceipt.command.requestId) {
        const receipt = pendingExpressionReceipt; pendingExpressionReceipt = undefined;
        element.dataset.expressionStatus = receipt.event.status;
        latest.current.onExpressionEvent?.(receipt.event, receipt.command);
      }
      sampleCost += performance.now() - started;samples++;sampleTime += dt;
      if (sampleTime >= 2) {
        element.dataset.renderStats = JSON.stringify({drawCalls:renderer.info.render.calls,triangles:renderer.info.render.triangles,meanSubmitMs:Math.round(sampleCost/samples*100)/100,framesPerSecond:Math.round(samples/sampleTime),pixelRatio:renderer.getPixelRatio(),state:animated?(greetingEnd?"greeting":state):"still",yaw:Math.round((model?.rotation.y??0)*180/Math.PI)});
        sampleTime = 0;samples = 0;sampleCost = 0;
      }
      const cameraSettling = camera.position.distanceToSquared(desiredCamera) > .000001 || cameraTarget.distanceToSquared(desiredTarget) > .000001 || Math.abs(camera.fov - cameraFov) > .001;
      return poseActive || activityMoving || cameraSettling || time <= settleUntil || !!latest.current.onSceneFrame;
    };
    const loop = createSceneLoop({ container: element, renderer, draw: tick,
      active: () => latest.current.renderActive !== false && !rendererUnavailable,
      adaptiveQuality: () => !!model && !characterSlot.state.loading && !roomSlot.pending,
      onSuspend: visibilityChanged });
    const soundClock = createWorldSoundClock(() => {
      const current = latest.current, world = roomSlot.world;
      if (!model || !world || !loop.visible || rendererUnavailable) { sound.stop(); return; }
      const activity = current.activity;
      const care = activity?.kind === "care" ? { activity: activity.activity,
        point: { x: activity.station.position[0], y: activity.station.position[1], z: activity.station.position[2] } } : null;
      sound.frame({ scope: `${current.characterId ?? "solo"}:${world.instanceId ?? world.id}`,
        place: { environmentId: current.environment?.id === "terrace" || world.instanceId?.includes(":terrace:") ? "terrace" : world.id,
          roomLayout: world.layout, powerOn: soundUtilities ? soundUtilities.power.status === "on" : null,
          night: (world.lighting ?? current.light) === "evening" },
        listener: { x: model.position.x, y: model.position.y + 1, z: model.position.z,
            yaw: Math.atan2(camera.matrixWorld.elements[8], camera.matrixWorld.elements[10]) },
        camera: world.id === "town-square" ? camera : undefined,
        actors: [{ id: current.play?.snapshot.playerId ?? current.characterId ?? "solo", point: model.position,
          moving: activityMoving, connected: true }], root: roomSlot.group, now: Date.now(),
        speechLevel: current.state === "speaking" ? current.speechLevel ?? 0 : 0,
        active: !roomSlot.pending && current.worldSoundActive !== false && current.renderActive !== false && current.framing !== "appearance"
          && !current.furnitureItems, care, utilities: soundUtilities, play: current.play });
    }, () => loop.visible);
    invalidate.current = () => loop.invalidate(); loop.invalidate();
    return () => {
      latest.current.onSceneFrame?.(null, 0);
      disposed = true;loop.dispose();invalidate.current = () => {};observer.disconnect();
      soundClock.dispose(); releaseSoundInput(); sound.dispose(); setWorldSound(current => current === sound ? null : current);
      publishRendererState("unavailable");
      retryShopAssets.current = () => {};
      retryRoomAssets.current = () => {};
      retryCharacterAssets.current = () => {};
      capture.current = () => Promise.reject(new Error("The character preview is closed."));
      pickDestination.current = () => null;
      pickCharacter.current = () => false;
      directionDestination.current = () => null;
      renderer.domElement.removeEventListener("webglcontextlost", contextLost);
      document.removeEventListener("visibilitychange", visibilityChanged);
      shopEquipment?.dispose(); soloRig?.dispose(); characterSlot.dispose();
      townSquareLighting.dispose();
      renderer.forceContextLoss();renderer.dispose();renderer.domElement.remove();
      floor.geometry.dispose();floor.material.dispose();
      roomSlot.dispose();
      rendererAssets.dispose();
    };
  }, [rendererRetry]);
  return <div className="character-renderer" ref={host} data-ready={ready} data-resource-downloading={downloading}>
    {ready && !error && props.renderActive !== false && props.state === "speaking" && props.speechText
      && !props.interactionActive && !props.furnitureItems && props.framing !== "appearance" && !roomAssets.loading
      ? <div ref={speechBubble} className="companion-speech" role="note" aria-label="Companion reply">
        <span className="companion-speech-text" dir="auto">{companionSpeechExcerpt(props.speechText)}</span>
      </div> : null}
    <ResourceDownloadState />
    {ready && !error && !downloading && worldSound && props.environment && props.framing !== "appearance"
      && !props.furnitureItems && !props.interactionActive && <WorldSoundControl sound={worldSound} enabled={soundPreference.enabled}
        onChange={error => { soundPreference.publish(error); invalidate.current(); }} />}
    {!ready && !error && !characterAssets.error && <span className="character-loading" role="status" aria-label="Loading Bestie..."><span className="character-spinner" aria-hidden="true" /><span>Loading Bestie...</span></span>}
    {error && <div className="character-error" role="alert">The 3D view is unavailable.<button onClick={() => setRendererRetry(value => value + 1)}>Retry 3D view</button></div>}
    {!error && !ready && characterAssets.error && <div className="character-error" role="alert">{characterAssets.error}<button disabled={characterAssets.loading} onClick={() => retryCharacterAssets.current()}>Retry character</button></div>}
    {!error && (roomAssets.error || ready && (characterAssets.error || roomAssets.loading || shopAssets.loading || shopAssets.errors.length > 0)) && <div className="character-shop-status" role={characterAssets.error || roomAssets.error || shopAssets.errors.length ? "alert" : "status"}>
      {ready && characterAssets.error && <><span>{characterAssets.error}</span><button disabled={characterAssets.loading} onClick={() => retryCharacterAssets.current()}>Retry character</button></>}
      {roomAssets.loading && <span><span className="character-spinner" aria-hidden="true" />Loading room...</span>}
      {shopAssets.loading && <span><span className="character-spinner" aria-hidden="true" />Loading item...</span>}
      {roomAssets.error && <><span>{roomAssets.error}</span><button onClick={() => retryRoomAssets.current()}>Retry room</button></>}
      {shopAssets.errors.length > 0 && <>{shopAssets.errors.map(value => <span key={value.id}>{value.message}</span>)}<button onClick={() => retryShopAssets.current()}>Retry items</button></>}
    </div>}
  </div>;
}

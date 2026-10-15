import type { CompanionCombatInput } from "../../../../../packages/utils/src/companionCombatInput";

export type CombatBrowserInput = ReturnType<typeof createCombatBrowserInput>;
type PointerState = { kind: "look" | "move" | "attack" | "block"; x: number; y: number; element: HTMLElement };
const typing = (target: EventTarget | null) => target instanceof Element
  && !!target.closest("input,textarea,select,[contenteditable]:not([contenteditable='false']),[role='textbox']");

/** Capture starts only from start(). The host must also block input before opening chat or an overlay. */
export function createCombatBrowserInput(element: HTMLElement, input: CompanionCombatInput, options: {
  onCaptureChange?(captured: boolean): void;
  onPointerLockUnavailable?(): void;
} = {}) {
  const doc = element.ownerDocument, win = doc.defaultView;
  const pointers = new Map<number, PointerState>();
  const keys = new Set<string>();
  const cleanups: (() => void)[] = [];
  const previousTabIndex = element.getAttribute("tabindex");
  let disposed = false, lockRequested = false, captureGeneration = 0;
  element.tabIndex = 0;
  function listen(target: EventTarget, name: string, handler: EventListener, capture = false) {
    target.addEventListener(name, handler, capture);
    cleanups.push(() => target.removeEventListener(name, handler, capture));
  }
  function locked() { return doc.pointerLockElement === element; }
  function eligible(target?: EventTarget | null) {
    const state = input.getSnapshot();
    if (typing(target ?? doc.activeElement)) { release(); return false; }
    return !disposed && state.active && state.eligible && !state.blocked
      && (locked() || element.contains(doc.activeElement) || doc.activeElement === doc.body || !doc.activeElement);
  }
  function releasePointers() {
    const releasing = Array.from(pointers);
    pointers.clear();
    for (const [id, pointer] of releasing) {
      input.releaseSource(`pointer:${id}`);
      try { if (pointer.element.hasPointerCapture(id)) pointer.element.releasePointerCapture(id); } catch { /* Already released. */ }
    }
  }
  function clearCapture() {
    captureGeneration++; keys.clear(); releasePointers(); lockRequested = false;
    if (locked()) doc.exitPointerLock();
    options.onCaptureChange?.(false);
  }
  function release() { input.release(); clearCapture(); }
  const unsubscribe = input.subscribe(() => { if (!input.getSnapshot().active) clearCapture(); });
  function start(pointerLock = true) {
    if (disposed || doc.visibilityState === "hidden" || !doc.hasFocus() || !input.start()) return false;
    element.focus({ preventScroll: true });
    if (pointerLock && element.requestPointerLock) {
      lockRequested = true;
      const stamp = ++captureGeneration;
      try {
        const pending: Promise<void> | void = element.requestPointerLock();
        if (pending) void pending.catch(() => {
          if (disposed || stamp !== captureGeneration) return;
          lockRequested = false; options.onPointerLockUnavailable?.();
        });
      } catch { lockRequested = false; options.onPointerLockUnavailable?.(); }
    }
    return true;
  }
  function movement() {
    const right = Number(keys.has("KeyD") || keys.has("ArrowRight")) - Number(keys.has("KeyA") || keys.has("ArrowLeft"));
    const forward = Number(keys.has("KeyW") || keys.has("ArrowUp")) - Number(keys.has("KeyS") || keys.has("ArrowDown"));
    input.move("keyboard", right, forward);
  }
  listen(doc, "keydown", ((event: KeyboardEvent) => {
    if (event.code === "Escape") { if (input.getSnapshot().active) { event.preventDefault(); release(); } return; }
    if (!eligible(event.target) || event.altKey || event.ctrlKey || event.metaKey) return;
    if (["KeyW", "KeyA", "KeyS", "KeyD", "ArrowUp", "ArrowLeft", "ArrowDown", "ArrowRight"].includes(event.code)) {
      event.preventDefault(); if (!event.repeat) { keys.add(event.code); movement(); }
    } else if (event.code === "Space") {
      event.preventDefault(); if (!event.repeat) input.hold("keyboard:attack", "attack", true);
    } else if (event.code === "ShiftLeft" || event.code === "ShiftRight") {
      event.preventDefault(); if (!event.repeat) input.hold(`keyboard:${event.code}`, "block", true);
    } else if (event.code === "KeyR" && !event.repeat) { event.preventDefault(); input.reload(); }
  }) as EventListener);
  listen(doc, "keyup", ((event: KeyboardEvent) => {
    if (keys.delete(event.code)) { event.preventDefault(); movement(); }
    if (event.code === "Space") input.releaseSource("keyboard:attack");
    if (event.code === "ShiftLeft" || event.code === "ShiftRight") input.releaseSource(`keyboard:${event.code}`);
  }) as EventListener);
  listen(doc, "pointerlockchange", (() => {
    if (locked()) {
      if (!input.getSnapshot().active || disposed || !lockRequested) { doc.exitPointerLock(); return; }
      lockRequested = false; options.onCaptureChange?.(true);
    } else if (input.getSnapshot().active) release();
  }) as EventListener);
  listen(doc, "pointerlockerror", (() => { lockRequested = false; options.onPointerLockUnavailable?.(); }) as EventListener);
  listen(doc, "mousemove", ((event: MouseEvent) => {
    if (locked() && eligible()) input.aim(event.movementX, event.movementY);
  }) as EventListener);
  function down(event: PointerEvent) {
    if (!eligible(event.target)) return;
    const target = event.target instanceof Element ? event.target.closest<HTMLElement>("[data-combat-control]") : null;
    const control = target?.dataset.combatControl;
    if (!control && event.target instanceof Element && event.target.closest("button,a,input,textarea,select,[role='button']")) return;
    if (!control && !locked() && event.target instanceof Element && !event.target.closest("canvas,[data-combat-look]")) return;
    if (control === "start" || control === "release" || control === "reload" || control === "leave") return;
    event.preventDefault();
    const kind: PointerState["kind"] = control === "move" ? "move" : control === "attack" ? "attack" : control === "block" ? "block"
      : locked() && event.pointerType === "mouse" ? event.button === 2 ? "block" : "attack" : "look";
    if (event.pointerType === "mouse" && event.button !== 0 && event.button !== 2) return;
    const owner = target ?? element;
    pointers.set(event.pointerId, { kind, x: event.clientX, y: event.clientY, element: owner });
    if (kind === "attack" || kind === "block") input.hold(`pointer:${event.pointerId}`, kind, true);
    if (kind === "move") input.move(`pointer:${event.pointerId}`, 0, 0);
    try { if (!locked()) owner.setPointerCapture(event.pointerId); } catch { /* Capture is optional in WebViews. */ }
  }
  listen(element, "pointerdown", down as EventListener);
  listen(doc, "pointermove", ((event: PointerEvent) => {
    const pointer = pointers.get(event.pointerId);
    if (!pointer || !eligible(event.target)) return;
    event.preventDefault();
    if (pointer.kind === "look" && !locked()) {
      input.aim(event.clientX - pointer.x, event.clientY - pointer.y); pointer.x = event.clientX; pointer.y = event.clientY;
    } else if (pointer.kind === "move") {
      const dx = event.clientX - pointer.x, dy = event.clientY - pointer.y;
      const dist = Math.hypot(dx, dy);
      const clamped = Math.min(dist, 36);
      const nx = dist > 0 ? (dx / dist) * clamped : 0;
      const ny = dist > 0 ? (dy / dist) * clamped : 0;
      pointer.element.style.setProperty("--combat-stick-x", `${Math.round(nx)}px`);
      pointer.element.style.setProperty("--combat-stick-y", `${Math.round(ny)}px`);
      input.move(`pointer:${event.pointerId}`, dx / 42, (pointer.y - event.clientY) / 42);
    }
  }) as EventListener);
  const up = ((event: PointerEvent) => {
    const pointer = pointers.get(event.pointerId);
    if (!pointer) return;
    if (pointer.kind === "move") {
      pointer.element.style.setProperty("--combat-stick-x", "0px");
      pointer.element.style.setProperty("--combat-stick-y", "0px");
    }
    pointers.delete(event.pointerId); input.releaseSource(`pointer:${event.pointerId}`);
    try { if (pointer.element.hasPointerCapture(event.pointerId)) pointer.element.releasePointerCapture(event.pointerId); } catch { /* Already released. */ }
  }) as EventListener;
  const cancel = ((event: PointerEvent) => { if (pointers.has(event.pointerId)) release(); }) as EventListener;
  listen(doc, "pointerup", up); listen(doc, "pointercancel", cancel);
  listen(element, "lostpointercapture", cancel, true);
  listen(element, "contextmenu", ((event: Event) => { if (input.getSnapshot().active) event.preventDefault(); }) as EventListener);
  listen(doc, "focusin", ((event: FocusEvent) => {
    if (input.getSnapshot().active && (typing(event.target) || event.target instanceof Node && !element.contains(event.target))) release();
  }) as EventListener);
  listen(doc, "visibilitychange", (() => { if (doc.visibilityState !== "visible") release(); }) as EventListener);
  if (win) { listen(win, "blur", release as EventListener); listen(win, "pagehide", release as EventListener); }
  return {
    start,
    release,
    setBlocked(value: boolean) { input.setBlocked(value); if (value) clearCapture(); },
    isPointerLocked: locked,
    dispose() {
      if (disposed) return;
      release(); disposed = true; unsubscribe(); cleanups.forEach(cleanup => cleanup());
      if (previousTabIndex === null) element.removeAttribute("tabindex"); else element.setAttribute("tabindex", previousTabIndex);
    },
  };
}

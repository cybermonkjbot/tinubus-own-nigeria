import { useEffect, useLayoutEffect, useRef, useState } from "react";
import type { CompanionWorldSnapshot } from "../../../../../packages/utils/src/companionWorld";
import { sampleCompanionMovement } from "../../../../../packages/utils/src/companionMovement";

export function acceptedCityMapMovement(world: CompanionWorldSnapshot) {
  const player = world.player, movement = player?.movement;
  return player && movement && player.status === "present" && movement.sequence === player.moveSequence && movement.roomRevision === player.revision ? movement : null;
}

/** Local presentation only. Uses the scene renderer's accepted route and server-adjusted clock. */
export function useCityMapPosition(world: CompanionWorldSnapshot, surface: HTMLElement | null) {
  const latest = useRef(world); latest.current = world;
  const read = () => {
    const current = latest.current, movement = acceptedCityMapMovement(current);
    return movement ? sampleCompanionMovement(movement, Date.now() + current.serverOffset)
      : { position: current.player!.position, moving: false };
  };
  const [position, setPosition] = useState(() => read().position);
  const wake = useRef(() => {});

  useEffect(() => {
    const element = surface;
    if (!element) return;
    let timer: ReturnType<typeof setTimeout> | undefined, disposed = false, intersecting = true, focused = document.hasFocus(), lastAt = -Infinity;
    const available = () => !disposed && focused && !document.hidden && intersecting && element.clientWidth > 0 && element.clientHeight > 0
      && getComputedStyle(element).visibility !== "hidden" && !element.closest("[hidden], [inert], [data-render-active='false']");
    const cancel = () => { clearTimeout(timer); timer = undefined; };
    const sample = () => {
      timer = undefined;
      if (!available()) return;
      lastAt = performance.now();
      const value = read();
      setPosition(previous => previous.x === value.position.x && previous.z === value.position.z && previous.yaw === value.position.yaw ? previous : value.position);
      if (value.moving) schedule();
    };
    const schedule = () => {
      if (timer !== undefined || !available()) return;
      timer = setTimeout(sample, Math.max(0, 100 - (performance.now() - lastAt)));
    };
    const visibility = () => { if (available()) schedule(); else cancel(); };
    const focus = () => { focused = true; visibility(); };
    const blur = () => { focused = false; cancel(); };
    const intersection = typeof IntersectionObserver === "undefined" ? undefined : new IntersectionObserver(entries => {
      intersecting = entries.some(entry => entry.isIntersecting); visibility();
    });
    intersection?.observe(element);
    const resize = new ResizeObserver(visibility); resize.observe(element);
    const attributes = new MutationObserver(visibility);
    for (let ancestor: HTMLElement | null = element; ancestor; ancestor = ancestor.parentElement) {
      attributes.observe(ancestor, { attributes: true, attributeFilter: ["inert", "hidden", "data-render-active", "style", "class"] });
    }
    document.addEventListener("visibilitychange", visibility);
    window.addEventListener("focus", focus); window.addEventListener("blur", blur);
    wake.current = schedule; schedule();
    return () => {
      disposed = true; cancel(); wake.current = () => {};
      intersection?.disconnect(); resize.disconnect(); attributes.disconnect();
      document.removeEventListener("visibilitychange", visibility);
      window.removeEventListener("focus", focus); window.removeEventListener("blur", blur);
    };
  }, [surface]);

  useLayoutEffect(() => { wake.current(); }, [world]);
  return position;
}

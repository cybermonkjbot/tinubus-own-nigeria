import { useLayoutEffect, useState, type ReactNode, type RefObject } from "react";
import * as Dialog from "@radix-ui/react-dialog";
import { BottomSheet } from "../../mobile";

/** Studio has phone/keyboard providers. The shared native scene deliberately does not. */
export function CityMapOverlay({ open, onOpenChange, trigger, children, title = "City map", className }: {
  open: boolean; onOpenChange(open: boolean): void; trigger: RefObject<HTMLButtonElement | null>;
  children(native: boolean): ReactNode; title?: string; className?: string;
}) {
  const [nativeRoot, setNativeRoot] = useState<HTMLElement | null | undefined>(undefined);
  useLayoutEffect(() => { setNativeRoot(trigger.current?.closest<HTMLElement>(".native-scene") ?? null); }, [trigger]);
  if (nativeRoot === undefined) return null;
  if (!nativeRoot) return <BottomSheet open={open} onOpenChange={onOpenChange} title={title} snap={.84}>
    {open ? children(false) : null}
  </BottomSheet>;

  return <Dialog.Root open={open} onOpenChange={onOpenChange}>
    <Dialog.Portal container={nativeRoot}>
      <Dialog.Overlay className="city-map-native-backdrop" />
      <Dialog.Content className={["city-map-native-dialog", className].filter(Boolean).join(" ")} aria-describedby={undefined}
        onCloseAutoFocus={event => { event.preventDefault(); trigger.current?.focus({ preventScroll: true }); }}>
        <Dialog.Title className="city-map-native-title">{title}</Dialog.Title>
        {children(true)}
      </Dialog.Content>
    </Dialog.Portal>
  </Dialog.Root>;
}

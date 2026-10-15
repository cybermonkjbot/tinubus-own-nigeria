import { useLayoutEffect, useRef, useState, type ReactNode, type RefObject } from "react";
import * as Dialog from "@radix-ui/react-dialog";
import { BottomSheet } from "../../mobile";
import type { CompanionWorldSnapshot } from "../../../../../packages/utils/src/companionWorld";
import { validCompanionGameEconomyQuote } from "../../../../../packages/utils/src/companionGameEconomy";
import { companionTransitFares, currentCompanionTransitFare, type CompanionTransitFare } from "../../../../../packages/utils/src/companionTransitFares";
import "./transit.css";

/** The account store admits the quote against recovery life and clears it on life changes. */
export function matchingTransitFare(world: CompanionWorldSnapshot | undefined): CompanionTransitFare | null {
  const fare=world?.transitFare,player=world?.player,instance=world?.instance;
  if(!fare||!player||!instance||player.status!=="present"||player.health<=0||player.transitRide
    ||player.instanceId!==instance.id||fare.instanceId!==instance.id||fare.visitRevision!==player.revision
    ||!fare.lifeId||fare.lifeId!==world?.transitLifeId||!validCompanionGameEconomyQuote(fare.quote))return null;
  return fare;
}

function FareOverlay({ anchor, locked, onCancel, children }: {
  anchor: RefObject<HTMLDivElement | null>; locked: boolean; onCancel(): void; children: ReactNode;
}) {
  const [nativeRoot,setNativeRoot]=useState<HTMLElement|null|undefined>(undefined);
  const returnFocus=useRef<HTMLElement|null>(null);
  useLayoutEffect(()=>{
    returnFocus.current=document.activeElement instanceof HTMLElement?document.activeElement:null;
    setNativeRoot(anchor.current?.closest<HTMLElement>(".native-scene")??null);
  },[anchor]);
  const close=(open:boolean)=>{if(!open&&!locked)onCancel();};
  if(nativeRoot===undefined)return null;
  if(!nativeRoot)return <BottomSheet open onOpenChange={close} title="Confirm fare" snap={.52}>
    {children}
  </BottomSheet>;
  return <Dialog.Root open onOpenChange={close}>
    <Dialog.Portal container={nativeRoot}>
      <Dialog.Overlay className="city-transit-fare-backdrop" />
      <Dialog.Content className="city-transit-fare-dialog" aria-describedby={undefined}
        onEscapeKeyDown={event=>{if(locked)event.preventDefault();}}
        onPointerDownOutside={event=>{if(locked)event.preventDefault();}}
        onCloseAutoFocus={event=>{
          event.preventDefault();
          const target=returnFocus.current?.isConnected?returnFocus.current:anchor.current;
          target?.focus({preventScroll:true});
        }}>
        <Dialog.Title className="city-transit-fare-title">Confirm fare</Dialog.Title>
        {children}
      </Dialog.Content>
    </Dialog.Portal>
  </Dialog.Root>;
}

/** Local attempt labels do not decide payment outcomes. The host retains the same action/ID for retry. */
export function TransitFareDialog({ fare, world, now, anchor, onService }: {
  fare: CompanionTransitFare; world: CompanionWorldSnapshot; now: number;
  anchor: RefObject<HTMLDivElement | null>; onService(serviceId:string,fareQuoteId?:string):void;
}) {
  const [attempt,setAttempt]=useState<{serviceId:string;pending:boolean}|null>(null);
  const [localError,setLocalError]=useState<string|null>(null);
  const inFlight=useRef(false),sawBusy=useRef(false),lastWorld=useRef(world);
  useLayoutEffect(()=>{
    if(!inFlight.current){lastWorld.current=world;return;}
    if(world.busy)sawBusy.current=true;
    if(!world.busy&&(sawBusy.current||world.transitFareRetryable||world.error&&lastWorld.current!==world)){
      inFlight.current=false;sawBusy.current=false;setAttempt(value=>value?{...value,pending:false}:null);
    }
    lastWorld.current=world;
  },[world]);
  const boardService=`transit:board:${fare.vehicleId}`;
  const uncertain=world.transitFareRetryable===true;
  const retryService=uncertain?world.transitFareRetryAction==="board"?boardService
    :world.transitFareRetryAction==="cancel"?"transit:cancel":null:null;
  const validRetry=retryService===boardService||retryService==="transit:cancel";
  const busy=world.busy||attempt?.pending===true;
  const available=world.ready&&world.renderedInstanceId===fare.instanceId
    &&world.transitAvailability?.boarding==="available"&&world.transitAvailability.farePolicy===companionTransitFares.version;
  const quoteValid=fare.quote.status==="quoted"&&fare.quote.service.available&&currentCompanionTransitFare(fare);
  const expired=!Number.isFinite(now)||now>=Math.min(fare.quote.expiresAt,fare.boardingEndsAt);
  const locked=busy||uncertain;
  const send=(serviceId:string,quoted=true)=>{
    if(inFlight.current||busy||!world.ready)return;
    if(uncertain&&(!validRetry||serviceId!==retryService||!quoted))return;
    inFlight.current=true;sawBusy.current=false;setLocalError(null);setAttempt({serviceId,pending:true});
    try {onService(serviceId,quoted?fare.quote.id:undefined);}
    catch (error) {inFlight.current=false;setAttempt({serviceId,pending:false});
      setLocalError(error instanceof Error?error.message:"The fare action couldn't be sent. Try again.");}
  };
  const cancel=()=>{if(!locked)send("transit:cancel");};
  const kind=fare.kind==="brt"?"BRT":fare.kind;
  const amount=fare.quote.service.price.amount;
  return <FareOverlay anchor={anchor} locked={locked} onCancel={cancel}>
    <div className="city-transit-fare-body" aria-busy={busy}
      onPointerDown={event=>event.stopPropagation()} onPointerMove={event=>event.stopPropagation()}
      onPointerUp={event=>event.stopPropagation()}
      onKeyDown={event=>{if(event.key!=="Escape"&&event.key!=="Tab")event.stopPropagation();}}
      onKeyUp={event=>event.stopPropagation()}>
      <p className="city-transit-fare-amount">{kind} · {amount} Lagos credits</p>
      <p>From banked Lagos credits. Operator fuel is included.</p>
      {localError||world.error?<p role="alert">{localError??world.error}</p>:null}
      {uncertain?<p role="status">The previous fare action is unresolved. Retry that action to check its outcome.</p>
        :expired?<p role="status">This boarding quote expired. Request a new quote.</p>
        :!quoteValid?<p role="status">This fare is no longer available. Request a new quote.</p>
        :!available?<p role="status">Waiting for the current visit.</p>:null}
      <div className="city-transit-fare-buttons">
        <button type="button" disabled={locked} onClick={cancel}>{busy&&attempt?.serviceId==="transit:cancel"?"Cancelling...":"Cancel"}</button>
        {uncertain?<button type="button" disabled={busy||!validRetry||!world.ready}
          onClick={()=>{if(validRetry&&retryService)send(retryService);}}>
          {busy?"Checking fare...":retryService==="transit:cancel"?"Retry cancellation":"Retry payment and boarding"}
        </button>:expired||!quoteValid?<button type="button" disabled={busy||!available}
          onClick={()=>send(boardService,false)}>{busy?"Requesting quote...":"Request new quote"}</button>
          :<button type="button" disabled={busy||!available} onClick={()=>send(boardService)}>
            {busy?"Confirming fare...":`Pay ${amount} credits and board`}
          </button>}
      </div>
    </div>
  </FareOverlay>;
}

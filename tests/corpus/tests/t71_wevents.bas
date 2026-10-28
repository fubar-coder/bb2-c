; t71_wevents - windowslib events: IntuiMessages put to the windows' port
; by the program itself (PutMsg), then Event/WaitEvent, EventCode,
; EventQualifier, EventWindow, EMouseX/Y, GadgetHit (string gadget
; kludge), MenuHit/ItemHit/SubHit (with the extra NextSelect event),
; RawKey/Qualifier/Inkey$, MButtons, FlushEvents.  Nothing waits for
; events the program has not put there itself.
; (NPrint evaluates its arguments from the right: events are fetched
; into variables first.)
DEFTYPE .l
WbToScreen 0
DefaultIDCMP $400|$100|8|$40|$20
Window 0,20,30,200,100,$000E,"Events 0",1,2
Window 1,240,30,200,100,$000E,"Events 1",1,2
WindowInput 0 : DefaultOutput
w0=Peek.l(Addr Window(0)) : w1=Peek.l(Addr Window(1))
port=Peek.l(w0+86)
NPrint "same port ",port=Peek.l(w1+86)
rport=CreateMsgPort("")
Dim mb.l(16*40) : mi=0
e=Event : NPrint "empty ",e," ",EventWindow," ",GadgetHit," ",MenuHit," ",ItemHit," ",SubHit
; any class: just reported
ecls=$4 : ecode=7 : equal=$8001 : eia=0 : emx=11 : emy=22 : ewin=w1 : Gosub post
e=Event : Print "event ",Hex$(e)," code ",EventCode," qual ",Hex$(EventQualifier)," win ",EventWindow
NPrint " mouse ",EMouseX," ",EMouseY
e=Event : NPrint "none ",e
; gadgets: a fake Gadget (ID at 38, type at 16)
Dim gd.l(16) : g=&gd(0) : Poke.w g+38,77 : Poke.w g+16,1
ecls=$40 : ecode=0 : equal=0 : eia=g : emx=1 : emy=2 : ewin=w0 : Gosub post
e=Event : NPrint "gadget up ",Hex$(e)," ",GadgetHit," ",EventWindow
Poke.w g+38,78 : Poke.w g+16,4
ecls=$20 : emx=3 : emy=4 : Gosub post
ecls=$4 : emx=5 : emy=6 : Gosub post
e=Event : NPrint "string down ",Hex$(e)," ",GadgetHit," ",EMouseX
e=Event : NPrint "kludge up ",Hex$(e)," ",GadgetHit," ",EMouseX," ",EventCode
e=Event : NPrint "then ",Hex$(e)," ",EMouseX
e=Event : NPrint "then ",e
; keys: converted with rawkeylib, key-up dropped
ecls=$400 : ecode=$20 : equal=0 : eia=0 : ewin=w0 : Gosub post
ecode=$A0 : Gosub post
ecode=$35 : equal=1 : Gosub post
; Qualifier sets only the low word (BUG(orig)), the upper word is whatever
; was in d0 (memory layout) - masked
e=Event : r=RawKey : NPrint "key ",Hex$(e)," ",r," ",Qualifier AND $FFFF
a$=Inkey$ : NPrint "inkey [",a$,"] ",RawKey," ",Qualifier AND $FFFF
e=Event : NPrint "key2 ",Hex$(e)
a$=Inkey$ : NPrint "inkey [",a$,"] ",RawKey," ",Qualifier AND $FFFF
a$=Inkey$ : e=Event : NPrint "nokey [",a$,"] ",e
; Inkey$ only from the input window: other windows' keys are thrown away
ecode=$33 : equal=0 : ewin=w1 : Gosub post
ecode=$22 : ewin=w0 : Gosub post
a$=Inkey$ : b$=Inkey$ : NPrint "inkey [",a$,"] [",b$,"] ",EventWindow
WindowInput 1
ecode=$21 : ewin=w1 : Gosub post
a$=Inkey$ : NPrint "inkey1 [",a$,"] ",EventWindow
WindowInput 0
; mouse buttons of the input window, others thrown away
ecls=8 : ecode=$68 : ewin=w0 : Gosub post
ecode=$E8 : Gosub post
ecode=$69 : ewin=w1 : Gosub post
ecode=$69 : ewin=w0 : Gosub post
ecode=$E9 : Gosub post
a=MButtons : b=MButtons : c=MButtons : d=MButtons : f=MButtons
NPrint "mbuttons ",a," ",b," ",c," ",d," ",f
ecode=$68 : Gosub post
e=Event : a=MButtons : b=MButtons : NPrint "event mb ",Hex$(e)," ",a," ",b
; menus
MenuTitle 0,0,"Project"
MenuItem 0,0,0,0,"Open"
MenuItem 0,0,0,1,"Save"
SubItem 0,0,0,1,0,"As"
SubItem 0,0,0,1,1,"Text"
MenuTitle 0,1,"Edit"
MenuItem 0,0,1,0,"Cut"
Use Window 0 : SetMenu 0
DefaultOutput
m=Peek.l(Addr MenuList(0))
i0=Peek.l(m+18) : i1=Peek.l(i0) : s0=Peek.l(i1+28) : s1=Peek.l(s0)
Poke.w i0+32,-1 : Poke.w s1+32,-1 : Poke.w i1+32,-1
c1=Peek.l(Peek.l(m)+18) : Poke.w c1+32,-1
ecls=$100 : ecode=0|(0 LSL 5)|(31 LSL 11) : ewin=w0 : Gosub post
e=Event : NPrint "menu ",Hex$(e)," ",MenuHit," ",ItemHit," ",SubHit," ",Hex$(EventCode)
ecode=0|(1 LSL 5)|(1 LSL 11) : Gosub post
e=Event : NPrint "sub ",Hex$(e)," ",MenuHit," ",ItemHit," ",SubHit
ecode=1|(0 LSL 5)|(31 LSL 11) : Gosub post
e=Event : NPrint "menu1 ",Hex$(e)," ",MenuHit," ",ItemHit," ",SubHit
ecode=$FFFF : ewin=w1 : Gosub post
e=Event : NPrint "menunull ",Hex$(e)," ",MenuHit," ",EventWindow
; NextSelect: one more MENUPICK
Poke.w i0+32,1|(0 LSL 5)|(31 LSL 11)
ecode=0|(0 LSL 5)|(31 LSL 11) : ewin=w0 : Gosub post
e=Event : NPrint "multi ",Hex$(e)," ",MenuHit," ",ItemHit
e=Event : NPrint "multi2 ",Hex$(e)," ",MenuHit," ",ItemHit," ",SubHit," ",Hex$(EventCode)," ",EventWindow
e=Event : NPrint "after ",e
; FlushEvents
ecls=8 : ecode=$68 : Gosub post
ecls=$4 : Gosub post
ecls=8 : Gosub post
FlushEvents 8
e=Event : f=Event : a=MButtons : NPrint "flush8 ",Hex$(e)," ",f," ",a
ecls=$400 : ecode=$20 : Gosub post
e=Event
ecls=$4 : Gosub post
FlushEvents
a$=Inkey$ : f=Event : NPrint "flush ",Hex$(e)," ",f," [",a$,"]"
; WaitEvent only for events put to the port before (cannot block)
ecls=$4 : ecode=9 : ewin=w1 : Gosub post
e=WaitEvent : NPrint "waitevent ",Hex$(e)," ",EventWindow," ",EventCode
ecls=$40000 : ecode=0 : ewin=w0 : Gosub post
WaitEvent : NPrint "waitevent2 ",EventWindow
; activating a window: only the call (its event is not waited for)
Activate 1 : NPrint "activate ok"
FlushEvents
CloseWindow 1 : CloseWindow 0
End

; put a message with ecls/ecode/equal/eia/emx/emy/ewin to the port
post:
  pm=&mb(0)+mi*64 : mi=mi+1
  For k=0 To 15 : Poke.l pm+k*4,0 : Next
  Poke.l pm+14,rport : Poke.w pm+18,52
  Poke.l pm+20,ecls : Poke.w pm+24,ecode : Poke.w pm+26,equal : Poke.l pm+28,eia
  Poke.w pm+32,emx : Poke.w pm+34,emy : Poke.l pm+44,ewin
  GetReg d0,pm : MOVE.l d0,a1 : GetReg d0,port : MOVE.l d0,a0
  MOVE.l 4,a6 : JSR -366(a6)
Return

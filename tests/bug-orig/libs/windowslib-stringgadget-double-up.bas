; windowslib-stringgadget-double-up (#311) - after a gadget down of a
; string gadget, Event gives an artificial gadget up ($40) unless the
; next message is the real gadget up; when another message comes in
; between, the real gadget up comes later as well: two gadget ups for
; one entry.
; IntuiMessages put to the window's port by the program (as the corpus
; test t71_wevents): gadget down of a fake string gadget, an event of
; class 4, the real gadget up of the same gadget; five Events.
; Correct: down, the artificial up, class 4, then nothing (the real up of
; the same gadget is taken without a second report).
; @fixed #311
; @configs clib
DEFTYPE .l
WbToScreen 0
DefaultIDCMP $400|$100|8|$40|$20
Window 0,20,30,200,100,$000E,"Events",1,2
DefaultOutput
w0=Peek.l(Addr Window(0))
port=Peek.l(w0+86)
rport=CreateMsgPort("")
Dim mb.l(16*8) : mi=0
; a fake Gadget (ID at 38, type at 16: 4 = string gadget)
Dim gd.l(16) : g=&gd(0) : Poke.w g+38,78 : Poke.w g+16,4
ecls=$20 : ecode=0 : equal=0 : eia=g : emx=3 : emy=4 : ewin=w0 : Gosub post
ecls=$4 : emx=5 : emy=6 : Gosub post
ecls=$40 : emx=7 : emy=8 : Gosub post
e1=Event : h1=GadgetHit
e2=Event : h2=GadgetHit
e3=Event
e4=Event
e5=Event
NPrint "events ",Hex$(e1)," ",Hex$(e2)," ",Hex$(e3)," ",Hex$(e4)," ",Hex$(e5)
NPrint "gadgets ",h1," ",h2
CloseWindow 0
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

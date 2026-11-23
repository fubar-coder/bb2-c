; windowslib-stringgadget-double-up (#311) - after a gadget down of a
; string gadget, Event gives an artificial gadget up ($40) unless the
; next message is the real gadget up; when another message comes in
; between, the real gadget up comes later as well: two gadget ups for
; one entry.
; Here: IntuiMessages put to the window's port by the program (as the
; corpus test t71_wevents): gadget down of a fake string gadget, an event
; of class 4, the real gadget up of the same gadget; five Events.
; Correct: down, the artificial up, class 4, then nothing (the real up of
; the same gadget is taken without a second report).
DEFTYPE .l

; put a message with ecls/eia/emx/emy to the port of window w0, from the
; message buffer pm
Statement post{pm,ecls,eia,emx,emy}
  SHARED rport,port,w0
  For k=0 To 15 : Poke.l pm+k*4,0 : Next
  Poke.l pm+14,rport : Poke.w pm+18,52
  Poke.l pm+20,ecls : Poke.l pm+28,eia
  Poke.w pm+32,emx : Poke.w pm+34,emy : Poke.l pm+44,w0
  GetReg d0,pm : MOVE.l d0,a1 : GetReg d0,port : MOVE.l d0,a0
  MOVE.l 4,a6 : JSR -366(a6)
End Statement

!TestCase{down_up_up}
  WbToScreen 0
  DefaultIDCMP $400|$100|8|$40|$20
  Window 0,20,30,200,100,$000E,"Events",1,2
  w0=Peek.l(Addr Window(0))
  port=Peek.l(w0+86)
  rport=CreateMsgPort("")
  Dim mb.l(16*8)
  ; a fake Gadget (ID at 38, type at 16: 4 = string gadget)
  Dim gd.l(16) : g=&gd(0) : Poke.w g+38,78 : Poke.w g+16,4
  post{&mb(0),$20,g,3,4}
  post{&mb(0)+64,$4,g,5,6}
  post{&mb(0)+128,$40,g,7,8}
  e1=Event : h1=GadgetHit
  e2=Event : h2=GadgetHit
  e3=Event
  e4=Event
  e5=Event
  !CheckEq{e1,$20,"event_1"}
  !CheckEq{e2,$40,"event_2"}
  !CheckEq{e3,$4,"event_3"}
  !CheckEq{e4,0,"event_4"}
  !CheckEq{e5,0,"event_5"}
  !CheckEq{h1,78,"gadget_1"}
  !CheckEq{h2,78,"gadget_2"}
  CloseWindow 0
!EndTestCase

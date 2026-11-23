; windowslib-nextselect-qualifier (#312) - for a MENUPICK whose item has a
; NextSelect (multiple selection), Event appends one more MENUPICK node
; for it, but does not set its qualifier and IAddress: the node is fresh
; memory of memlib (not cleared), EventQualifier of that event is what
; the memory held.
; Here: run under inject with fill=a5 (every AllocMem block filled with
; $A5), so the memory is known. A MENUPICK of item 0 with the qualifier
; $8001 put to the window's port by the program (as the corpus test
; t71_wevents); item 0 has item 1 as its NextSelect.
; Correct: both events have the qualifier $8001 of the menu pick.
DEFTYPE .l

!TestCase{two_picks}
  WbToScreen 0
  DefaultIDCMP $400|$100|8|$40|$20
  Window 0,20,30,200,100,$000E,"Menus",1,2
  w0=Peek.l(Addr Window(0))
  port=Peek.l(w0+86)
  rport=CreateMsgPort("")
  Dim mb.l(16)
  MenuTitle 0,0,"Project"
  MenuItem 0,0,0,0,"Open"
  MenuItem 0,0,0,1,"Save"
  Use Window 0 : SetMenu 0
  m=Peek.l(Addr MenuList(0))
  i0=Peek.l(m+18) : i1=Peek.l(i0)
  Poke.w i1+32,-1
  Poke.w i0+32,0|(1 LSL 5)|(31 LSL 11)
  ; the MENUPICK message of item 0 with the qualifier $8001
  pm=&mb(0)
  For k=0 To 15 : Poke.l pm+k*4,0 : Next
  Poke.l pm+14,rport : Poke.w pm+18,52
  Poke.l pm+20,$100 : Poke.w pm+24,0|(0 LSL 5)|(31 LSL 11) : Poke.w pm+26,$8001
  Poke.w pm+32,1 : Poke.w pm+34,2 : Poke.l pm+44,w0
  GetReg d0,pm : MOVE.l d0,a1 : GetReg d0,port : MOVE.l d0,a0
  MOVE.l 4,a6 : JSR -366(a6)
  e1=Event : q1=EventQualifier : n1=ItemHit
  e2=Event : q2=EventQualifier : n2=ItemHit
  !CheckEq{e1,$100,"event_1"}
  !CheckEq{e2,$100,"event_2"}
  !CheckEq{n1,0,"item_1"}
  !CheckEq{n2,1,"item_2"}
  !CheckEq{q1&$FFFF,$8001,"qualifier_1"}
  !CheckEq{q2&$FFFF,$8001,"qualifier_2"}
  CloseWindow 0
!EndTestCase

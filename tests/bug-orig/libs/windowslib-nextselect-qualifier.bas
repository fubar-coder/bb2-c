; windowslib-nextselect-qualifier (#312) - for a MENUPICK whose item has a
; NextSelect (multiple selection), Event appends one more MENUPICK node
; for it, but does not set its qualifier and IAddress: the node is fresh
; memory of memlib (not cleared), EventQualifier of that event is what
; the memory held.
; Run under inject with fill=a5 (every AllocMem block filled with $A5),
; so the memory is known. A MENUPICK of item 0 with the qualifier $8001
; put to the window's port by the program (as the corpus test
; t71_wevents); item 0 has item 1 as its NextSelect.
; Correct: both events have the qualifier $8001 of the menu pick.
; @pass runwrap=@INJECT@|fill=a5
; @fixed #312
; @configs clib
DEFTYPE .l
WbToScreen 0
DefaultIDCMP $400|$100|8|$40|$20
Window 0,20,30,200,100,$000E,"Menus",1,2
DefaultOutput
w0=Peek.l(Addr Window(0))
port=Peek.l(w0+86)
rport=CreateMsgPort("")
Dim mb.l(16*8) : mi=0
MenuTitle 0,0,"Project"
MenuItem 0,0,0,0,"Open"
MenuItem 0,0,0,1,"Save"
Use Window 0 : SetMenu 0
DefaultOutput
m=Peek.l(Addr MenuList(0))
i0=Peek.l(m+18) : i1=Peek.l(i0)
Poke.w i1+32,-1
Poke.w i0+32,0|(1 LSL 5)|(31 LSL 11)
ecls=$100 : ecode=0|(0 LSL 5)|(31 LSL 11) : equal=$8001 : eia=0 : emx=1 : emy=2 : ewin=w0 : Gosub post
e1=Event : q1=EventQualifier : n1=ItemHit
e2=Event : q2=EventQualifier : n2=ItemHit
NPrint "events ",Hex$(e1)," ",Hex$(e2)," items ",n1," ",n2
NPrint "qualifiers ",Hex$(q1&$FFFF)," ",Hex$(q2&$FFFF)
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

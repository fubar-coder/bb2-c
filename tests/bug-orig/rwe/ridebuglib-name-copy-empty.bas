; ridebuglib-name-copy-empty (#612) - the commands work in their error
; check routines (only in a program compiled with runtime error checking)
; and send a message to the debugger through debuglib's docall ($D509):
; command.b, data1.b, data2.b, pad.b, size.l, the name from +8.
; AddVarTrace copies the name with a loop that tests the byte after the
; one copied: an empty name copies its 0 and then the bytes behind it up
; to the next 0.  The name here is Chr$(0)+"behind": empty up to its
; first byte, with bytes behind it that the test chooses.
; The program installs its own debugger handlers (debuglib's
; dosethandler, $D502): the docall handler remembers the message and
; counts it.  AddVarTrace b,"bb",1, then AddVarTrace b with the empty
; name; the bytes at +8 and from +9 of the second message.
; Correct: only the 0 of the empty name is copied; +9 keeps the "b" of
; the previous message.
; @rwe ridebuglib
; @pass runtime_errors=on
DEFTYPE .l
a=1 : b=2
Poke.l ?tab,?hok
Poke.l ?tab+4,?hok
Poke.l ?tab+8,?hok
Poke.l ?tab+12,?hok
Poke.l ?tab+16,?hrts
Poke.l ?tab+20,?hrts
Poke.l ?tab+24,?hcall
Poke.l ?tab+28,?hok
MOVEM.l d0-d7/a0-a6,-(a7)
LEA tab,a0
MOVE.l #ar+4,d0
SysJsr $D502
MOVEM.l (a7)+,d0-d7/a0-a6
n$=Chr$(0)+"behind"
NPrint "set"
AddVarTrace b,"bb",1
m=Peek.l(?ar)
AddVarTrace b,n$,1
NPrint "messages ",Peek.l(?ar+8)
NPrint "+8 ",Peek.b(m+8) AND 255
NPrint "+9 ",Peek$(m+9)
End

; the handlers: state, super error, user error, wait (Z: go on), eval,
; (unused), docall (remembers the message, counts it), procedure
hok:
  CMP.w d0,d0
hrts:
  RTS
hcall:
  MOVE.l a1,-(a7)
  LEA ar(pc),a1
  MOVE.l a0,(a1)
  ADDQ.l #1,8(a1)
  MOVE.l (a7)+,a1
  RTS

; the last message, the procedure flag word (0: handler 7 not called),
; the number of messages
ar:
  Dc.l 0,0,0
tab:
  Dc.l 0,0,0,0,0,0,0,0

; ridebuglib-delvartrace-offset-12 (#610) - the commands work in their
; error check routines (only in a program compiled with runtime error
; checking) and send a message to the debugger through debuglib's docall
; ($D509): command.b, data1.b, data2.b, pad.b, size.l, the name from +8.
; DelVarTrace skips 11 bytes after the command byte and writes the name
; at +12; +8 keeps the name of the previous AddVarTrace, which the
; debugger then reads as the name of the trace to delete.
; Here the program installs its own debugger handlers (debuglib's
; dosethandler, $D502): the docall handler remembers the message and
; counts it.  AddVarTrace b,"bb",1 then DelVarTrace "a"; the command byte
; and the name at +8 of each message.
; Correct: the DelVarTrace message has the name "a" at +8.
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
NPrint "set"
AddVarTrace b,"bb",1
m=Peek.l(?ar)
NPrint "messages ",Peek.l(?ar+8)
NPrint "add ",Peek.b(m) AND 255," ",Peek$(m+8)
DelVarTrace "a"
NPrint "messages ",Peek.l(?ar+8)
NPrint "del ",Peek.b(m) AND 255," ",Peek$(m+8)
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

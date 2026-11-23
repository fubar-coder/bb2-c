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
DEFTYPE .l

!TestCase{messages}
  a=1 : b=2
  Poke.l ?dbgtab,?dbgok
  Poke.l ?dbgtab+4,?dbgok
  Poke.l ?dbgtab+8,?dbgok
  Poke.l ?dbgtab+12,?dbgok
  Poke.l ?dbgtab+16,?dbgrts
  Poke.l ?dbgtab+20,?dbgrts
  Poke.l ?dbgtab+24,?dbgcall
  Poke.l ?dbgtab+28,?dbgok
  MOVEM.l d0-d7/a0-a6,-(a7)
  LEA dbgtab,a0
  MOVE.l #dbgmsg+4,d0
  SysJsr $D502
  MOVEM.l (a7)+,d0-d7/a0-a6
  AddVarTrace b,"bb",1
  m=Peek.l(?dbgmsg)
  n1=Peek.l(?dbgmsg+8)
  c1=Peek.b(m) AND 255
  s1$=Peek$(m+8)
  DelVarTrace "a"
  n2=Peek.l(?dbgmsg+8)
  c2=Peek.b(m) AND 255
  s2$=Peek$(m+8)
  !CheckEq{n1,1,"messages_after_add"}
  !CheckEq{c1,255,"add_command"}
  !CheckStrEq{s1$,"bb","add_name"}
  !CheckEq{n2,2,"messages_after_del"}
  !CheckEq{c2,254,"del_command"}
  !CheckStrEq{s2$,"a","del_name"}
  Goto dbgend

  ; the handlers: state, super error, user error, wait (Z: go on), eval,
  ; (unused), docall (remembers the message, counts it), procedure
  .dbgok
    CMP.w d0,d0
  .dbgrts
    RTS
  .dbgcall
    MOVE.l a1,-(a7)
    LEA dbgmsg(pc),a1
    MOVE.l a0,(a1)
    ADDQ.l #1,8(a1)
    MOVE.l (a7)+,a1
    RTS

  ; the last message, the procedure flag word (0: handler 7 not called),
  ; the number of messages
  .dbgmsg
    Dc.l 0,0,0
  .dbgtab
    Dc.l 0,0,0,0,0,0,0,0
  .dbgend
!EndTestCase

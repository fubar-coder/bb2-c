; riarexxlib-defaultport-no-limit (#529) - RXDefaultPort copies the name
; without a limit into its 65 bytes; behind them lie rexxbase and the
; other variables.  A name of 70 characters overwrites rexxbase: the next
; CreateArgString (the message of RXRunCommand, here to the program's own
; port) jumps through it.
; To give that jump a defined end the bytes 66-69 of the name are not
; "PPPP" but the address of a library base the program builds in a block
; it allocates (Z3 memory: the address must have a non-zero top byte, the
; program checks that; the block is placed so that the base lies in it):
; every vector below the base is an RTS, the CreateArgString vector
; (-126) a jump to rec.  rec puts the stack pointer back from sv (the call
; chain is abandoned, as it would be by the crash) and sets w=1.  This
; relies on details outside the bug: that the Z3 load address of
; AllocMem_ has a top byte (without it the check address_bytes fails
; first), that the program may execute the code it writes, and that the
; stack pointer saved before the RXDefaultPort call is the right one at rec.
; Correct: the name is refused or cut, the message is sent and arrives
; (w stays 0, received is -1).
DEFTYPE .l

!TestCase{long_name}
  b=AllocMem_(140000,$10001)
  !Require{b<>0,"memory"}
  x=(b+66000) AND $FFFF0000
  If ((x LSR 16) AND 255)=0 Then x+$10000
  x+$0180
  !Check{(x LSR 24)<>0,"address_bytes"}
  For i=x-400 To x-2 Step 2 : Poke.w i,$4E75 : Next
  Poke.w x-126,$4EF9 : Poke.l x-124,?rec
  n$=String$("P",66)+Chr$(x LSR 24)+Chr$((x LSR 16) AND 255)+Chr$((x LSR 8) AND 255)+Chr$(x AND 255)
  p=RXCreatePort("BUGORIG")
  RXTokenise On
  w=0
  LEA sv(pc),a0 : MOVE.l a7,(a0)
  RXDefaultPort n$
  !Check{-1,"name_set"}
  RXRunCommand "BUGORIG","hello"
  m=RXReadPort
  received=(m<>0)
  !CheckEq{received,-1,"received"}
  Goto ok
rec:
  LEA sv(pc),a0 : MOVE.l (a0),a7
  w=1
ok:
  !CheckEq{w,0,"library_base"}
  Goto fin
sv:
  Dc.l 0
fin:
!EndTestCase

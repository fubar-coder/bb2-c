; audiolib-setperiod-err5 (#70) - SetPeriod without a sound branches
; (BEQ err5) from the normal code into the error check area, and the
; messages of err1-err4 (normal code) lie there too: without runtime
; error checking that area is not part of the program, the branch lands
; behind the library and d0 points behind it.  Here SetPeriod on an empty
; sound ("Uninitialized Sound") and LoadSound of a missing file
; ("Illegal Sound file"), runtime error checking off.  The program sets
; its own trap handler (tc_TrapCode of its task): a TRAP #0 (exception
; 32) continues at a label of the program with the message in d0; an
; exception with a short stack frame also continues there, without a
; message; any other goes on to the old handler.
; Correct: both runtime errors with their messages.  The original runs
; into other code for SetPeriod (it may return, fault or loop, depending on
; the memory layout: the words behind the library are a variable of the
; next library, a heap address) and passes an address behind the library
; as the message of LoadSound.  The program therefore reports only whether
; the message of SetPeriod is "Uninitialized Sound" (any other outcome, also
; a fault of the garbage code, which the handler turns into a return to
; the program for exceptions with a short stack frame, prints the same
; line), and prints the text at the message address of LoadSound in full
; (code bytes of the library behind: fixed in the executable).
; @fixed #70
; @configs clib
; @pass timeout=60
DEFTYPE .l
t=Peek.l(Peek.l(4)+276)
o=Peek.l(t+50)
Poke.l ?ar+4,o
Poke.l t+50,?th
NPrint "before"
k=1
LEA ar(pc),a0 : MOVE.l a5,8(a0) : MOVE.l a7,12(a0)
SetPeriod 0,428
Goto rec
s2:
k=2
LEA ar(pc),a0 : MOVE.l a5,8(a0) : MOVE.l a7,12(a0)
LoadSound 0,"RAM:audiolib-setperiod-err5.none"
Goto rec
s3:
Poke.l t+50,o
NPrint "after"
End

rec:
  d=Peek.l(?ar)
  Poke.l ?ar,0
  If k=1
    ok=0
    If d<>0 Then If Peek$(d)="Uninitialized Sound" Then ok=1
    If ok
      NPrint "SetPeriod: runtime error Uninitialized Sound"
    Else
      NPrint "SetPeriod: no runtime error Uninitialized Sound"
    EndIf
    Goto s2
  EndIf
  If d=0 Then NPrint "LoadSound: no runtime error" Else NPrint "LoadSound: runtime error ",Peek$(d)
  Goto s3

th:
  CMP.l #32,(a7) : BNE other
  ADDQ.l #4,a7
  LEA ar(pc),a0 : MOVE.l d0,(a0)
  LEA rec(pc),a0 : MOVE.l a0,2(a7)
  RTE
other:
  MOVE.w 10(a7),d0 : AND.w #$F000,d0 : BNE thold
  LEA ar(pc),a0 : MOVE.l (a7),16(a0) : ADDQ.l #4,a7
  LEA ar(pc),a0 : CLR.l (a0) : MOVE.l 8(a0),a5 : MOVE.l 12(a0),a0 : MOVE a0,USP
  LEA rec(pc),a0 : MOVE.l a0,2(a7)
  RTE
thold:
  MOVE.l ar+4(pc),-(a7)
  RTS

ar:
  Dc.l 0,0,0,0,0

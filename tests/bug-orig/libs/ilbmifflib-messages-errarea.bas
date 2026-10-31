; ilbmifflib-messages-errarea (#192) - the error routines of ilbmifflib
; (MOVE.l #message,d0:TRAP #0) are in the normal code, but their messages
; lie in the error check area: without runtime error checking that area
; is not part of the program, and d0 points behind the library.  Here
; DecodeILBM with memory that is no FORM ("Can't Find IFF 'BMHD'") and
; with a FORM ILBM without BODY ("Can't Find IFF 'BODY'"), runtime error
; checking off.  The program sets its own trap handler (tc_TrapCode of
; its task): a TRAP #0 (exception 32) continues at a label of the
; program with the message in d0; any other exception goes on to the old
; handler.
; Correct: both runtime errors with their messages.  The original passes
; an address behind the library as the message.
; @configs clib
; @pass timeout=60
DEFTYPE .l
t=Peek.l(Peek.l(4)+276)
o=Peek.l(t+50)
Poke.l ?ar+4,o
Poke.l t+50,?th
BitMap 0,16,16,1
NPrint "before"
k=1
DecodeILBM 0,?noform
NPrint "no error"
s2:
k=2
DecodeILBM 0,?nobody
NPrint "no error"
s3:
Poke.l t+50,o
NPrint "after"
End

rec:
  d=Peek.l(?ar)
  NPrint "runtime error ",Peek$(d)
  If k=1 Then Goto s2
  Goto s3

th:
  CMP.l #32,(a7) : BNE thold
  ADDQ.l #4,a7
  LEA ar(pc),a0 : MOVE.l d0,(a0)
  LEA rec(pc),a0 : MOVE.l a0,2(a7)
  RTE
thold:
  MOVE.l ar+4(pc),-(a7)
  RTS

ar:
  Dc.l 0,0
noform:
  Dc.l 0,0,0,0
nobody:
  Dc.l $464f524d,40,$494c424d
  Dc.l $424d4844,20
  Dc.w 16,1,0,0
  Dc.b 1,0,0,0
  Dc.w 0
  Dc.b 10,11
  Dc.w 320,200
  Dc.l $414e4e4f,0

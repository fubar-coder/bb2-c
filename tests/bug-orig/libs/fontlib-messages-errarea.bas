; fontlib-messages-errarea (#955) - the runtime errors of fontlib trap in
; the normal code (err1 "Can't Open Font" of LoadBlitzFont, err2/err3
; "Uninitialized BitMap"/"Uninitialized Font" of the bitmap output), but
; their messages lie in the error check area.  Without runtime error
; checking that area is not part of the program: d0 points behind the
; library.
; Here, runtime error checking off: LoadBlitzFont of a font that does not
; exist (err1; BlitzFont 1 stays without a font and becomes the output
; font), Print on an uninitialized bitmap (err2), then on bitmap 0 with
; that font (err3).  The program sets its own trap handler (tc_TrapCode
; of its task): a TRAP #0 (exception 32) continues at the label in ar+8
; with the message in d0; any other exception goes on to the old handler.
; Correct: the three runtime errors with their messages.  The original
; passes pointers behind the library (garbage texts).
; @fixed #955
; @configs clib
; @pass timeout=60
DEFTYPE .l
BitMap 0,320,16,1
t=Peek.l(Peek.l(4)+276)
o=Peek.l(t+50)
Poke.l ?ar+4,o
Poke.l ?ar+8,?rec1
Poke.l t+50,?th
NPrint "before"
LoadBlitzFont 1,"bb2-c-no-such.font"
Poke.l t+50,o
NPrint "no error 1"
End

rec1:
  NPrint "runtime error ",Peek$(Peek.l(?ar))
  Poke.l ?ar+8,?rec2
  BitMapOutput 1
  Print "x"
  DefaultOutput
  Poke.l t+50,o
  NPrint "no error 2"
  End

rec2:
  DefaultOutput
  NPrint "runtime error ",Peek$(Peek.l(?ar))
  Poke.l ?ar+8,?rec3
  BitMapOutput 0
  Print "x"
  DefaultOutput
  Poke.l t+50,o
  NPrint "no error 3"
  End

rec3:
  DefaultOutput
  Poke.l t+50,o
  NPrint "runtime error ",Peek$(Peek.l(?ar))
  End

th:
  CMP.l #32,(a7) : BNE thold
  ADDQ.l #4,a7
  LEA ar(pc),a0 : MOVE.l d0,(a0)
  MOVE.l ar+8(pc),2(a7)
  RTE
thold:
  MOVE.l ar+4(pc),-(a7)
  RTS

ar:
  Dc.l 0,0,0

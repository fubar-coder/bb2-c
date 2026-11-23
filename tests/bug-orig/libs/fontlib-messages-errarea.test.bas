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
; of its task): a TRAP #0 (exception 32) continues at the label in tl_ar+8
; with the message in d0; any other exception goes on to the old handler.
; The handler code and its data stand in the case, behind a jump; the
; handler path ends the program (no end entry of the case).
; Correct: the three runtime errors with their messages.  The original
; passes pointers behind the library (garbage texts).
DEFTYPE .l

!TestCase{three_errors}
  Goto tl_start
tl_th:
  CMP.l #32,(a7) : BNE tl_thold
  ADDQ.l #4,a7
  LEA tl_ar(pc),a0 : MOVE.l d0,(a0)
  MOVE.l tl_ar+8(pc),2(a7)
  RTE
tl_thold:
  MOVE.l tl_ar+4(pc),-(a7)
  RTS
tl_ar:
  Dc.l 0,0,0
tl_start:
  BitMap 0,320,16,1
  t=Peek.l(Peek.l(4)+276)
  o=Peek.l(t+50)
  Poke.l ?tl_ar+4,o
  Poke.l ?tl_ar+8,?tl_rec1
  Poke.l t+50,?tl_th
  LoadBlitzFont 1,"bb2-c-no-such.font"
  Poke.l t+50,o
  !Check{0,"error_1_raised"}
  End
tl_rec1:
  m1$=Peek$(Peek.l(?tl_ar))
  Poke.l ?tl_ar+8,?tl_rec2
  BitMapOutput 1
  Print "x"
  DefaultOutput
  Poke.l t+50,o
  !Check{0,"error_2_raised"}
  End
tl_rec2:
  DefaultOutput
  m2$=Peek$(Peek.l(?tl_ar))
  Poke.l ?tl_ar+8,?tl_rec3
  BitMapOutput 0
  Print "x"
  DefaultOutput
  Poke.l t+50,o
  !Check{0,"error_3_raised"}
  End
tl_rec3:
  DefaultOutput
  Poke.l t+50,o
  m3$=Peek$(Peek.l(?tl_ar))
  !CheckStrEq{m1$,"Can't Open Font","message_1"}
  !CheckStrEq{m2$,"Uninitialized BitMap","message_2"}
  !CheckStrEq{m3$,"Uninitialized Font","message_3"}
  End
!EndTestCase

; riseriallib-close-intena (#790) - SerialClose switches every interrupt
; off (INTENA $7FFF) and then on those that were on at SerialOpen (with
; the master bit): an interrupt switched on since SerialOpen is lost, one
; switched off since is switched on again.  The program switches the
; vertical blank interrupt (VERTB, bit 5) off between SerialOpen and
; SerialClose and reads it after SerialClose (then switches it on again).
; Correct: SerialClose changes only the library's two interrupts (RBF,
; TBE); VERTB stays off.
DEFTYPE .l

!TestCase{close_intena}
  SerialOpen 19200
  Poke.w $DFF09A,$0020
  SerialClose
  v=Peek.w($DFF01C) AND $0020
  Poke.w $DFF09A,$8020
  !CheckEq{v,0,"vertb_off"}
!EndTestCase

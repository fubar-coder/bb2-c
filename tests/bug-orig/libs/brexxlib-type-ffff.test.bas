; brexxlib-type-ffff (#102) - Type takes rawkeylib unconvert's result
; unchecked: a character without a raw key code gives a RAWKEY event
; with code $FFFF and qualifier 1 (left shift).
; Here: Type "a"+Chr$(200)+"b" to a tape (QuietTrap On).
; Correct: the character without a code is skipped, the tape holds the
; two events of "a" and "b".  The original has three, the middle one
; with code $FFFF.
DEFTYPE .l

!TestCase{unconvertible_char}
  QuietTrap On
  TapeTrap 0
  Type "a"+Chr$(200)+"b"
  e=Peek.l(Addr Tape(0))
  !CheckEq{Peek.b(e+4),1,"event_1_class"}
  !CheckEq{Peek.w(e+6) AND $FFFF,$20,"event_1_code"}
  !CheckEq{Peek.w(e+8) AND $FFFF,0,"event_1_qualifier"}
  e=Peek.l(e)
  !CheckEq{Peek.b(e+4),1,"event_2_class"}
  !CheckEq{Peek.w(e+6) AND $FFFF,$35,"event_2_code"}
  !CheckEq{Peek.w(e+8) AND $FFFF,0,"event_2_qualifier"}
  !CheckEq{Peek.l(e),0,"event_3_none"}
!EndTestCase

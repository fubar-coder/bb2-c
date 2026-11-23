; brexxlib-clickbutton-d0 (#97) - ClickButton calls MouseButton for the
; press and falls into it for the release with the d0 the first call
; left: with the event going to a tape and QuietTrap On, that is the
; QuietTrap value 1, so ClickButton 0 records "left down, RIGHT up".
; Correct: the tape holds RAWMOUSE $68 (left down) and $E8 (left up).
; The original records $68 and $E9.
DEFTYPE .l

!TestCase{tape_events}
  QuietTrap On
  TapeTrap 0
  ClickButton 0
  e=Peek.l(Addr Tape(0))
  !CheckEq{Peek.b(e+4),2,"event_1_class"}
  !CheckEq{Peek.w(e+6) AND $FFFF,$68,"event_1_code"}
  e=Peek.l(e)
  !CheckEq{Peek.b(e+4),2,"event_2_class"}
  !CheckEq{Peek.w(e+6) AND $FFFF,$E8,"event_2_code"}
  !CheckEq{Peek.l(e),0,"event_3"}
!EndTestCase

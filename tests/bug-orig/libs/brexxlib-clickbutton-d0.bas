; brexxlib-clickbutton-d0 (#97) - ClickButton calls MouseButton for the
; press and falls into it for the release with the d0 the first call
; left: with the event going to a tape and QuietTrap On, that is the
; QuietTrap value 1, so ClickButton 0 records "left down, RIGHT up".
; Correct: the tape holds RAWMOUSE $68 (left down) and $E8 (left up).
; The original records $68 and $E9.
; @fixed #97
; @configs clib
DEFTYPE .l
QuietTrap On
TapeTrap 0
ClickButton 0
e=Peek.l(Addr Tape(0))
While e<>0
  NPrint "class ",Peek.b(e+4)," code ",Right$(Hex$(Peek.w(e+6)),4)
  e=Peek.l(e)
Wend
End

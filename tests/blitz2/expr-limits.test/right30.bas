; expr-limits.test/right30.bas: 30 nested Right$ around
; Hex$(Peek.b(...)), more pending elements than the compiler takes
Macro r
Right$(Right$(Right$(Right$(Right$(Right$(
End Macro
Macro rc
, 2), 2), 2), 2), 2), 2)
End Macro
b.l = 3
For i.w = 0 To 3
  Print !r!r!r!r!r Hex$(Peek.b(&b + i) & $FF)!rc!rc!rc!rc!rc, " "
Next
NPrint ""
End

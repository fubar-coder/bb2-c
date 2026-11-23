; expr-limits.test/calls32.bas: 32 nested function calls, 33 pending
; elements, one more than the compiler takes
Macro a
Abs(Abs(Abs(Abs(Abs(Abs(Abs(Abs(
End Macro
Macro c
))))))))
End Macro
b.l = 3
b = !a!a!a!a b!c!c!c!c
NPrint b
End

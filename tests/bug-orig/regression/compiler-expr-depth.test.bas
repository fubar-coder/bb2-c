; compiler-expr-depth (#1080) - expressions as deep as the compiler's
; tables allow, compiled with the shell's default stack (4000 bytes) on
; Kickstart 1.3: 63 nested parentheses (the most the precedence stack
; takes), 31 nested function calls (the most) with 32 parentheses inside -
; the deepest recursion of the expression evaluator - and 28 nested Right$
; around Hex$(Peek.b(...)).  The lines are built with macros (a TED line
; holds 128 characters).
; Correct: the program compiles and gives 3, 3 and "00 00 00 03 ".
; The port's evaluator needs about 25 KB of stack for the second
; expression (blitz2.s about 40-50 bytes per level): before the fix of
; #1080 the compile overran the 4000 bytes.
Macro o
((((((((
End Macro
Macro c
))))))))
End Macro
Macro a
Abs(Abs(Abs(Abs(Abs(Abs(Abs(Abs(
End Macro
Macro r
Right$(Right$(Right$(Right$(
End Macro
Macro rc
, 2), 2), 2), 2)
End Macro

!TestCase{deep_expressions}
  b.l = !o!o!o!o!o!o!o(((((((3)))))))!c!c!c!c!c!c!c
  !CheckEq{b,3,"parentheses_63"}
  b = !a!a!a Abs(Abs(Abs(Abs(Abs(Abs(Abs(!o!o!o!o b!c!c!c!c)))))))))))))))))))))))))))))))
  !CheckEq{b,3,"calls_31_parentheses_32"}
  s$ = ""
  For i.w = 0 To 3
    h$ = !r!r!r!r!r!r!r Hex$(Peek.b(&b + i) & $FF)!rc!rc!rc!rc!rc!rc!rc
    s$ = s$ + h$ + " "
  Next
  !CheckStrEq{s$,"00 00 00 03 ","right_hex_28"}
!EndTestCase

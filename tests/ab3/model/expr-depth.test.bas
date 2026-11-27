; expr-depth - expressions as deep as the compiler's tables allow,
; compiled with the shell's default stack (#1080): 63 nested parentheses
; (the most the precedence stack takes), 31 nested function calls (the
; most) with 32 parentheses inside (the deepest recursion of the expression
; evaluator), 28 nested Right$ around Hex$(Peek.b(...)). The long lines are
; the subject.
!TestCase{expr_depth}
b.l = (((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((3)))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))
  !CheckEq{b,3,"l1"}
b = Abs(Abs(Abs(Abs(Abs(Abs(Abs(Abs(Abs(Abs(Abs(Abs(Abs(Abs(Abs(Abs(Abs(Abs(Abs(Abs(Abs(Abs(Abs(Abs(Abs(Abs(Abs(Abs(Abs(Abs(Abs(((((((((((((((((((((((((((((((((b)))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))
  !CheckEq{b,3,"l2"}
  zs$=""
  For i.w = 0 To 3
    t$=Right$(Right$(Right$(Right$(Right$(Right$(Right$(Right$(Right$(Right$(Right$(Right$(Right$(Right$(Right$(Right$(Right$(Right$(Right$(Right$(Right$(Right$(Right$(Right$(Right$(Right$(Right$(Right$(Hex$(Peek.b(&b + i) & $FF), 2), 2), 2), 2), 2), 2), 2), 2), 2), 2), 2), 2), 2), 2), 2), 2), 2), 2), 2), 2), 2), 2), 2), 2), 2), 2), 2), 2)
    zs$=zs$+t$+" "
  Next
  !CheckStrEq{zs$,"00 00 00 03 ","l3"}
!EndTestCase

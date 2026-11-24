; expr-limits-minus-run - a very long run of minus signs in an expression
; compiles with the stack use of a single sign (#1205): the port's first
; evaluator recursed once per sign (about 123 bytes of stack each), so a
; run of about 400 signs overran the compiler's 48 KB stack.  The runs
; here have 768 and 769 signs (and 1536 in front of a bracket), built
; with macros (a TED line holds 128 characters; a macro ends in a sign,
; not in a call of another macro).
; Correct: the sign of the operand is flipped once per minus sign.
Macro m16
----------------
End Macro
Macro m128
!m16!m16!m16!m16!m16!m16!m16----------------
End Macro

!TestCase{minus_768}
  b.l = 3 : f.f = 1.5
  r.l = !m128!m128!m128!m128!m128!m128 b
  !CheckEq{r,3,"even"}
  g.f = !m128!m128!m128!m128!m128!m128 f
  !CheckFltNear{g,1.5,0.001,"even_float"}
!EndTestCase

!TestCase{minus_769}
  b.l = 3 : f.f = 1.5
  r.l = !m128!m128!m128!m128!m128!m128- b
  !CheckEq{r,-3,"odd"}
  g.f = !m128!m128!m128!m128!m128!m128- f
  !CheckFltNear{g,-1.5,0.001,"odd_float"}
  r = 10 - !m128!m128!m128!m128!m128!m128- b
  !CheckEq{r,13,"binary"}
!EndTestCase

!TestCase{minus_1536_bracket}
  b.l = 3
  r.l = !m128!m128!m128!m128!m128!m128!m128!m128!m128!m128!m128!m128 (b)
  !CheckEq{r,3,"bracket"}
!EndTestCase

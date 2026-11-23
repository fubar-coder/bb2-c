; expr-limits - the limits of the expression evaluator, at the edge: what
; still compiles is here, what is one step beyond is in the data sources
; of expr-limits.test/ (their compile errors, message and line, are
; asserted in expr-limits.test.toml).  The limits are those of the
; original's tables: 64 entries of the precedence stack (one for the
; expression, one per parenthesis, per NOT and per pending operator:
; "Precedence Stack Overflow"), 32 pending elements ("Expression too
; Complex") and the registers d0-d7 for the operands (an operand beyond
; d7: "Expression too Complex", #1185).  A chain of one precedence and a
; run of minus signs have no limit of their own, and a minus sign is not
; counted (64 times "-(" fail at the 64th parenthesis).  The long lines
; are built with macros (a TED line holds 128 characters; a NOT in a
; macro gives "Syntax Error", so NOT stands in the line itself).
; Correct: every case compiles and gives the values of its checks.
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
Macro t10
kk+kk+kk+kk+kk+kk+kk+kk+kk+kk
End Macro
Macro t100
!t10+!t10+!t10+!t10+!t10+!t10+!t10+!t10+!t10+!t10
End Macro
Macro m16
----------------
End Macro
Macro mb
-(-(-(-(-(-(-(-(
End Macro

!TestCase{brackets_63}
  b.l = 3
  b = !o!o!o!o!o!o!o(((((((b)))))))!c!c!c!c!c!c!c
  !CheckEq{b,3,"brackets"}
!EndTestCase

!TestCase{pending_64}
  ; the expression, + and *, a parenthesis, NOT and 59 parentheses
  v1.l = 1 : v2.l = 2 : b.l = 3
  r.l = v1 + (v2 * NOT !o!o!o!o!o!o!o(((b)))!c!c!c!c!c!c!c)
  !CheckEq{r,-7,"pending"}
  r = NOT NOT NOT NOT NOT NOT NOT NOT NOT NOT NOT NOT NOT NOT NOT NOT NOT NOT NOT NOT NOT NOT NOT NOT b
  !CheckEq{r,3,"not_24"}
  r = NOT NOT NOT NOT NOT NOT NOT NOT NOT NOT NOT NOT NOT NOT NOT NOT NOT NOT NOT NOT NOT NOT NOT b
  !CheckEq{r,-4,"not_23"}
!EndTestCase

!TestCase{calls_31}
  b.l = -3
  b = !a!a!a Abs(Abs(Abs(Abs(Abs(Abs(Abs(b)))))))!c!c!c
  !CheckEq{b,3,"calls"}
!EndTestCase

!TestCase{operand_d7}
  v1.l=1 : v2.l=2 : v3.l=3 : v4.l=4 : v5.l=5 : v6.l=6 : v7.l=-1 : v8.l=3 : v9.l=2
  r.l=v1+(v2+(v3+(v4+(v5+(v6+(v7+v8+v9))))))
  !CheckEq{r,25,"d7"}
!EndTestCase

!TestCase{chain_1000}
  kk.l = 1
  sum.l = !t100+!t100+!t100+!t100+!t100+!t100+!t100+!t100+!t100+!t100
  !CheckEq{sum,1000,"chain"}
!EndTestCase

!TestCase{right_16}
  b.l = 3
  s$ = ""
  For i.w = 0 To 3
    h$ = !r!r!r!r Hex$(Peek.b(&b + i) & $FF)!rc!rc!rc!rc
    s$ = s$ + h$ + " "
  Next
  !CheckStrEq{s$,"00 00 00 03 ","right"}
!EndTestCase

!TestCase{minus_run}
  b.l = 3 : f.f = 1.5
  r.l = !m16!m16!m16!m16 b
  !CheckEq{r,3,"even"}
  r = !m16!m16!m16--------------- b
  !CheckEq{r,-3,"odd"}
  g.f = !m16!m16!m16--------------- f
  !CheckFltNear{g,-1.5,0.001,"float"}
  r = -(-(-(-(-(-(-(b)))))))
  !CheckEq{r,-3,"brackets"}
  r = !mb!mb!mb!mb!mb!mb!mb-(-(-(-(-(-(-(b)))))))!c!c!c!c!c!c!c
  !CheckEq{r,-3,"brackets_63"}
  r = 10 - -b * - -2
  !CheckEq{r,16,"operands"}
!EndTestCase

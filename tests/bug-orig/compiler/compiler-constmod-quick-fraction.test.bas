; compiler-constmod-quick-fraction - a Quick Mod of two constants by a
; divisor whose whole part is 0 (5 Mod 0.5).
; When both operands of Mod are constants, blitz2 folds the operation by
; running the code it has just generated in its own process (eval5,
; blitz2.s:9007); a Quick Mod is domodquick, DIVS.W of the whole parts.
; The check of a constant divisor 0 (#1044, divzerochk) looks at the whole
; Quick (0.5 is not 0), so the fold divides by the whole part 0: the CPU's
; division trap in the compiler (exec's alert "Software Failure",
; 8000 0005), no result.  The port keeps that: its fold in C leaves this
; case to the run (exprfold.c, the trap of a Quick Mod).
; Correct: the line compiles; a Mod with a Quick operand is exact (rule P+
; of expr-tree-5-operand-typing, design decision 11, point 2): 5 Mod 0.5
; is 0.
!TestCase{constmod_quick_fraction}
  rq.q=1
  rq=5 Mod 0.5
  !CheckQEq{rq,0,"half"}
!EndTestCase

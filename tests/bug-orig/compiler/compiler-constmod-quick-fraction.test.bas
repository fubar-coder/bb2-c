; compiler-constmod-quick-fraction (#1219) - a Quick Mod of two constants
; by a divisor whose whole part is 0 (5 Mod 0.5).
; Fixed (#1219): when both operands of Mod are constants, blitz2 folds
; the operation by running the code it has just generated in its own
; process (eval5, blitz2.s:9007); a Quick Mod is domodquick, DIVS.W of the
; whole parts.  The check of a constant divisor 0 (#1044, divzerochk)
; looks at the whole Quick (0.5 is not 0), so the fold divides by the
; whole part 0: the CPU's division trap in the compiler (exec's alert
; "Software Failure", 8000 0005), no result.  The port folds the Mod exactly:
; the typing pass (exprtyping.c const_flow, eval.c p5constapply) and the
; fold in C (exprfold.c) no longer leave it to the run.
; Correct: the lines compile; a Mod with a Quick operand is exact, its
; result a Quick (rule P+ of expr-tree-5-operand-typing, design decision
; 11, point 2): 5 Mod 0.5 is 0, 5.25 Mod 0.5 is 0.25.
!TestCase{constmod_quick_fraction}
  rq.q=1
  rq=5 Mod 0.5
  !CheckQEq{rq,0,"half"}
  rq=5.25 Mod 0.5
  !CheckQEq{rq,0.25,"quarter"}
  rq=-5.75 Mod 0.5
  !CheckQEq{rq,0.25,"negative"}
!EndTestCase

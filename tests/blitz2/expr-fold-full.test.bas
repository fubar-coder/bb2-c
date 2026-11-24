; expr-fold-full - the constant fold while the object buffer is full: a
; fold is not made while nomemleft is set (the pass is compiled again with
; a buffer of the size the first pass needed, #1190).  The child
; (expr-fold-full.test/child.bas) fills the object buffer of 2992 bytes
; (jobline objbuf=) with folds, then folds a Mod (the first use of modlib),
; a sum after a variable, a negated bracket, a Float and a Quick product
; and Not of a sum with a division; it is compiled without the driver,
; whose code would change where the buffer ends, and pins the code of
; these forms for the restructuring of the evaluator (#1180, change
; expr-tree-4a-emit-after-build).
; Correct: the child compiles and prints -9 and 525.
DEFTYPE .l

!TestCase{child}
  *r.TLRun=!RunProgram{"@TESTDIR@/child","child"}
  !CheckEq{*r\rc,0,"rc"}
  !CheckStrEq{*r\printed,"-9"+Chr$(10)+"525"+Chr$(10),"printed"}
!EndTestCase

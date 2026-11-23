; compiler-macro-end-line-include (#968) - the include case of
; compiler-macro-end-line: a macro whose last body line ends in End.  In an
; include the bytes behind the stored 0 are the next line, and readinc
; hands the same line out a second time: it is End Macro, the macro ends on
; the glued copy, the second copy is compiled as a statement and doend has
; no case for Macro: "Syntax Error" at the Include line.
; The included macro (the test's data) sets x_seen and ends in
; "If 0 Then End", so that its End never ends the test program.
; Correct (AmiBlitz3): the macro is defined and runs.
DEFTYPE .l

!TestCase{macro_in_include}
  Include "CORPUS:src/inc.bb2"
  x_seen=0
  !m
  !CheckEq{x_seen,1,"macro_ran"}
!EndTestCase

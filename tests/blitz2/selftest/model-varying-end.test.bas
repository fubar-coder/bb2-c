; model-varying-end: a cell whose end varies (the table ends of change
; basic-test-runner-extension, task 3.3; ctest
; blitz2_selftest_model_varying_end): the case returns or hangs by the
; argument its cell gives the program (runargs return or hang), as an
; original whose end depends on the memory behind a table returns or hangs.
; Both ends are declared as alternatives, so both cells pass and the case
; comes out as declared (REPRODUCED: the form of an open bug).
; model-varying-end-one.test.bas is the same source with only one of the
; alternatives: the cell that hangs fails, naming the alternative.
!TestCase{varies}
  !Check{-1,"before"}
  If Par$(1)="hang"
    s.l=Wait_($1000)
  EndIf
  !Check{-1,"after"}
!EndTestCase

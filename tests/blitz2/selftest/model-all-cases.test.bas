; model-all-cases: the run of all cases of a source (corpusrun case without
; --case; ctest blitz2_selftest_model_all_cases, change
; basic-test-runner-extension task 1.5). The cases share the run of the
; driver of all of them, but prints, which asserts the printed output (an
; observation of the whole program) and runs on its own from the start.
; ends ends the program with End: the shared run does not end cleanly (the
; library writes incomplete before the end entry), ends runs on its own,
; where it passes (its configuration expects no end entry of the case), and
; the others run again without it. In that second shared run fails fails its check and is
; confirmed on its own (FAILED again), and victim fails since pass1 set the
; global it checks: on its own it passes, and the report names the verdict
; of the shared run.
!TestCase{pass1}
  mark.l=1
  !CheckEq{1+1,2,"sum"}
!EndTestCase

!TestCase{fails}
  !CheckEq{1+1,3,"wrong"}
!EndTestCase

!TestCase{ends}
  !Check{-1,"before"}
  End
!EndTestCase

!TestCase{behind}
  !CheckEq{2*3,6,"product"}
!EndTestCase

!TestCase{victim}
  !CheckEq{mark,0,"untouched"}
!EndTestCase

!TestCase{prints}
  NPrint "hello"
  !Check{-1,"printed"}
!EndTestCase

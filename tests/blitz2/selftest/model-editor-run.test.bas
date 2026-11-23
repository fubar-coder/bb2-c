; model-editor-run: the program runs from the editor (editor-run = "on",
; change basic-test-editor-run task 2.4; ctest
; blitz2_selftest_model_editor_run and _hang): tedstub compiles the driver
; and runs it with COMPILE/RUN, runtime error checking on, so the run has
; the debugger. prints passes from its result file and its console output
; (the query output); hangs prints a line and loops, the hang check armed
; on the console ends the session, and the case never reaches its end.
!TestCase{prints}
  NPrint "start"
  !CheckEq{6*7,42,"product"}
!EndTestCase

!TestCase{hangs}
  NPrint "before the hang"
  Repeat
    n.l+1
  Forever
  !Check{0,"behind"}
!EndTestCase

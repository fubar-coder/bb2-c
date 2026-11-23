; model-compile-inject: inject's report of the compiler (the query
; compile.inject, change basic-test-editor-run task 2.4; ctest
; blitz2_selftest_model_compile_inject): the compiler runs through inject
; with track, which reports the compiler's allocations left behind its run;
; the program runs from the shell as in any cell.
!TestCase{tracked}
  !CheckEq{1+2,3,"sum"}
!EndTestCase

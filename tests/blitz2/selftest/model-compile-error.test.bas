; model-compile-error: a test of the BASIC test model whose text does not
; compile on purpose (ctest blitz2_selftest_model_compile_error): the
; runner must report the error at its line of this file, not at the line of
; the driver that holds the text (basic-test-runner task 2.3)
!TestCase{broken}
  !CheckEq{1,1,"one"}
  x.l=(1+
!EndTestCase

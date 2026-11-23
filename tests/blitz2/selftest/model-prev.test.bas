; model-prev: the source of the self tests of the comparison of a test of
; the BASIC test model with the prev build (tests/blitz2/CMakeLists.txt,
; blitz2_selftest_model_*_prev): one case and no cell with runtime error
; checking, so one compile, named model-prev
!TestCase{one}
  a.l=1
  !CheckEq{a,1,"one"}
!EndTestCase

; model-isolate: the run setting isolate = "on" gives each cell an emulator
; session of its own (ctest blitz2_selftest_model_isolate, change
; basic-test-runner-extension task 2.1): the two cells (runtime error
; checking off and on) would share one session without it, and here each
; has its own, its memory and RAM: shared with no other cell's program.
!TestCase{own}
  !CheckEq{1+1,2,"sum"}
!EndTestCase

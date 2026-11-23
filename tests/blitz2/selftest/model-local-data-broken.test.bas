; model-local-data-broken: a data source that does not compile makes the
; cases of the cell BROKEN, the report naming the line of the data source
; (ctest blitz2_selftest_model_local_data_broken, change
; basic-test-local-data task 2.4)
!TestCase{child}
  !CheckEq{1+1,2,"sum"}
!EndTestCase

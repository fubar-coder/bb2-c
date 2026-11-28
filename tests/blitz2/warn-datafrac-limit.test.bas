; warn-datafrac-limit - the editor shows a type warning (change
; type-system-rework, spec compiler/warnings) for at most 8 places per
; name and compile, then one "datafrac: 2 more": 10 Data.b values with a
; fraction (the console prints all 10: tests/ab3/cli/datafrac_many). The
; values are cut towards zero as without the warning.
!TestCase{datafrac_limit}
  Restore wdl_d
  Read a.b,b.b,c.b
  !CheckEq{a,1,"first"}
  !CheckEq{b,-2,"second"}
  !CheckEq{c,3,"third"}
!EndTestCase
.wdl_d
Data.b 1.5
Data.b -2.5
Data.b 3.5
Data.b 4.5
Data.b 5.5
Data.b 6.5
Data.b 7.5
Data.b 8.5
Data.b 9.5
Data.b 10.5

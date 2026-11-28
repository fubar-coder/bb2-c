; warn-datafrac-many - 10 places of the warning datafrac: on the console
; every place, its line each (the editor shows 8 and "datafrac: 2 more",
; tests/blitz2/warn-datafrac-limit); the executable is written.
!TestCase{datafrac_many}
  Restore dfm_d
  Read a.b
  !CheckEq{a,1,"cut"}
!EndTestCase
.dfm_d
Data.b 1.5
Data.b 2.5
Data.b 3.5
Data.b 4.5
Data.b 5.5
Data.b 6.5
Data.b 7.5
Data.b 8.5
Data.b 9.5
Data.b 10.5

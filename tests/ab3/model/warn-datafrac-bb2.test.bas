; warn-datafrac-bb2 - warn-datafrac in BB2 mode (a text source without
; header): the warning datafrac gives the same entry at the line of the
; Data.b value; the Byte keeps the value cut towards zero (-1.5 gives -1).
!TestCase{datafrac_bb2}
  Restore dfr_d
  Read a.b,b.b
  !CheckEq{a,-1,"cut"}
  !CheckEq{b,1,"whole"}
!EndTestCase
.dfr_d
Data.b -1.5,1.0

; rinonvolatilelib-storenvdata-result (#1085) - StoreNVData is documented
; to return TRUE/FALSE, whether the save completed; it returns the result
; of nonvolatile.library's StoreNV instead: 0 when stored, an NVERR_...
; code when not.  Here the system's nonvolatile.library has no storage
; (no NVRAM, and no nv_location on the read-only system disk), so StoreNV
; fails and nothing is stored: the original's result is not 0, and the
; manual's test "If StoreNVData(...)" says "stored".
; Correct: StoreNVData returns 0 (FALSE) when nothing was stored, -1
; (TRUE) when it was.
DEFTYPE .l

!TestCase{nothing_stored}
  d.l=$12345678
  o=IsNonvolatile
  !Require{o=-1,"open"}
  r.w=StoreNVData("RWETest","Item",&d,4)
  !CheckEq{r,0,"result"}
!EndTestCase

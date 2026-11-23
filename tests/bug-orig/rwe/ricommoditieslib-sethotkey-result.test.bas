; ricommoditieslib-sethotkey-result (#565) - SetHotKey never calls
; CxObjError: a hotkey description that commodities' ParseIX does not
; understand still gives a filter (with COERR_BADFILTER), which is
; attached and reported as success (-1), though it never triggers.  (It
; also tests the d0 that the void AttachCxObj leaves; with Workbench 3.1's
; commodities.library that is not 0.)
; Correct: SetHotKey with an invalid description returns 0; a valid one
; -1.
DEFTYPE .l

!TestCase{descriptions}
  made=MakeCommodity("BugOrigResult","BugOrig","bad hotkey")
  !Require{made<>0,"commodity"}
  bad=SetHotKey(1,"rawkey nonsense_qualifier q")
  good=SetHotKey(2,"lalt f1")
  !CheckEq{bad,0,"invalid"}
  !CheckEq{good,-1,"valid"}
!EndTestCase

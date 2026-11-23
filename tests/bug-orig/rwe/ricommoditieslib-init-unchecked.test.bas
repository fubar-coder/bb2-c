; ricommoditieslib-init-unchecked (#559) - init opens commodities.library
; V37 without checking the result; MakeCommodity creates its message
; port and calls CxBroker through the base without testing it.  The
; program runs through inject with the open failing (runwrap=, inject
; openlibfail=commodities.library): the base is 0, CxBroker jumps to
; address -36 and the system crashes (the session ends in a timeout).
; Correct: MakeCommodity fails (0), the program goes on.
DEFTYPE .l

!TestCase{no_library}
  made=MakeCommodity("BugOrig","BugOrig","no library")
  !CheckEq{made,0,"failed"}
!EndTestCase

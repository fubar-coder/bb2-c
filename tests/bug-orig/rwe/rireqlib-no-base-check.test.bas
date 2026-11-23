; rireqlib-no-base-check (#788) - no command checks the base of
; req.library: when init could not open it, ReqFileRequest and
; ReqColour load the base 0 into a6 and call FileRequester (JSR
; -$54(a6)) and ColorRequester (JSR -$c0(a6)).  The program runs through
; inject with the open failing (runwrap=, inject
; openlibfail=req.library): the jump to address -$54 crashes the system
; (the session ends in a timeout) before ReqColour.
; Correct: without req.library ReqFileRequest returns "" (as on cancel,
; also with a title: the rest of the expression follows at once;
; ReqFileLoc returns 0) and ReqColour 65535 ($FFFF, what the real
; ColorRequester returns on cancel), the program goes on.
DEFTYPE .l

!TestCase{open_fails}
  fl=ReqFileLoc
  !CheckEq{fl,0,"fileloc"}
  f$=ReqFileRequest
  !CheckEq{Len(f$),0,"file_result"}
  g$="<"+ReqFileRequest("Load")+">"
  !CheckStrEq{g$,"<>","file_title"}
  c=ReqColour(5)
  !CheckEq{c,65535,"colour"}
!EndTestCase

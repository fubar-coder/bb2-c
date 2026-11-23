; rireqlib-real-needs-libs - the control of rireqlib-real-cancel: the
; real req.library is staged in CORPUS:libs, but without inject's libs=
; the Workbench's LIBS: does not have it, so the library does not open
; (ReqFileLoc 0) and the requesters return at once: "" and 65535, as
; on cancel (#788).  Shows that rireqlib-real-cancel gets the
; library through libs=.

!TestCase{library_not_found}
  !CheckEq{ReqFileLoc,0,"library_closed"}
  a$="<"+ReqFileRequest("RQ")+">"
  !CheckStrEq{a$,"<>","requester_returns_empty"}
  !CheckEq{ReqColour(5),65535,"colour_returns_ffff"}
!EndTestCase

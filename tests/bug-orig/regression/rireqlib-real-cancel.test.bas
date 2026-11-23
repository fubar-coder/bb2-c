; rireqlib-real-cancel - rireqlib with the real req.library 2.8 (Aminet
; util/libs/ReqLib28.lha, archive/aminet/ReqLib28/; skipped without it):
; the library opens, two file requesters and the colour requester are
; ended by a click on their cancel gadget (inject click=, `*` for the
; title: the window the gadget is in; libs= puts the staged directory
; into LIBS:).  Checks, against the real library, claims of the fixes
; #788 and #789:
;  - ReqFileLoc is not 0 (the library opened);
;  - a cancelled ReqFileRequest returns "" (also in a string expression);
;  - a title is for that call only (#789): the window of the next call
;    without one has the title of the empty title buffer, "", not "RQ"
;    (the title the call before had), asserted on inject's report;
;  - ReqColour on cancel returns what ColorRequester returns, the word
;    $FFFF (also what #788 returns without the library);
;  - the library is closed again (opencount +0), asserted on inject's
;    report.

!TestCase{cancel_requesters}
  !Check{ReqFileLoc<>0,"library_open"}
  a$=ReqFileRequest("RQ")
  !CheckStrEq{a$,"","first_cancelled"}
  b$="<"+ReqFileRequest+">"
  !CheckStrEq{b$,"<>","second_cancelled"}
  !CheckEq{ReqColour(5),65535,"colour_cancelled"}
!EndTestCase

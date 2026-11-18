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
;    (the title the call before had);
;  - ReqColour on cancel returns what ColorRequester returns, the word
;    $FFFF (#788 returns the colour it is given only without the
;    library);
;  - the library is closed again (opencount +0).
; @configs clib
; @rwe rireqlib
; @pass file=libs/req.library|@REQLIB@
; @pass runwrap=@INJECT@|libs=CORPUS:libs opencount=req.library click=*/Forget_it click=*/Forget_it click=*/Cancel
; @pass timeout=60
If ReqFileLoc<>0 Then NPrint "open" Else NPrint "closed"
a$=ReqFileRequest("RQ")
NPrint "first <",a$,">"
b$="<"+ReqFileRequest+">"
NPrint "second ",b$
NPrint "colour ",ReqColour(5)
End

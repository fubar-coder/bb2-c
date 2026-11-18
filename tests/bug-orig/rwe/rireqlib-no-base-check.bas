; rireqlib-no-base-check (#788) - no command checks the base of
; req.library: when init could not open it, ReqFileRequest and
; ReqColour load the base 0 into a6 and call FileRequester (JSR
; -$54(a6)) and ColorRequester (JSR -$c0(a6)).  The program runs through
; inject with the open failing (runwrap=, inject
; openlibfail=req.library): the jump to address -$54 crashes the system
; (the session ends in a timeout) before ReqColour.
; Correct: without req.library ReqFileRequest returns "" (as on cancel,
; also with a title: the rest of the expression follows at once;
; ReqFileLoc returns 0) and ReqColour the colour it is given, the
; program goes on.
; @fixed #788
; @rwe rireqlib
; @pass runwrap=@INJECT@|openlibfail=req.library
; @pass timeout=30
NPrint ReqFileLoc
f$=ReqFileRequest
NPrint "result ",Len(f$)
g$="<"+ReqFileRequest("Load")+">"
NPrint "title ",g$
NPrint "colour ",ReqColour(5)
End

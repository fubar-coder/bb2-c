; rireqlib-no-base-check (#788) - no command checks the base of
; req.library: when init could not open it, ReqFileRequest loads the
; base 0 into a6 and calls FileRequester (JSR -$54(a6)).  The program
; runs through inject with the open failing (runwrap=, inject
; openlibfail=req.library): the jump to address -$54 crashes the system
; (the session ends in a timeout).
; Correct: without req.library ReqFileRequest returns "" (as ReqFileLoc
; returns 0), the program goes on.
; @rwe rireqlib
; @pass runwrap=@INJECT@|openlibfail=req.library
; @pass timeout=30
NPrint ReqFileLoc
f$=ReqFileRequest
NPrint "result ",Len(f$)
End

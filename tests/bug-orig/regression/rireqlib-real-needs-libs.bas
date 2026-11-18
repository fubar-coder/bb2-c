; rireqlib-real-needs-libs - the control of rireqlib-real-cancel: the
; real req.library is staged in CORPUS:libs, but without inject's libs=
; the Workbench's LIBS: does not have it, so the library does not open
; (ReqFileLoc 0) and the requesters return at once: "" and the colour
; they are given (#788).  Shows that rireqlib-real-cancel gets the
; library through libs=.
; @configs clib
; @rwe rireqlib
; @pass file=libs/req.library|@REQLIB@
; @pass runwrap=@INJECT@|opencount=req.library
; @pass timeout=60
If ReqFileLoc<>0 Then NPrint "open" Else NPrint "closed"
NPrint "<"+ReqFileRequest("RQ")+">"
NPrint "colour ",ReqColour(5)
End

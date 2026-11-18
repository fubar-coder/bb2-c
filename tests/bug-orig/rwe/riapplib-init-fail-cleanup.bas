; riapplib-init-fail-cleanup (#526) - init opens workbench, dos and
; icon.library V37 one after the other; if one fails it deletes the
; message port and frees its block, but does not close the libraries
; already opened (finit tests libflag and closes nothing) and keeps the
; deleted port in msgport, which AppMsgPort returns.  The program runs
; through inject, which fails the OpenLibrary of icon.library (inject
; counts two failed opens) and counts the opens minus the closes of
; workbench and dos.library: +0 in the original too, the leak of the
; open counts is not seen this way.
; Correct: AppMsgPort is 0 (the deleted port is not returned).
; @fixed #526
; @rwe riapplib
; The program is silent for about 11 s (more than BUG_ORIG_RUN_HANG_AFTER):
; @pass hang_after=30
; @pass runwrap=@INJECT@|openlibfail=icon.library opencount=workbench.library opencount=dos.library
DEFTYPE .l
If AppMsgPort=0 Then NPrint "port 0" Else NPrint "port set"
End

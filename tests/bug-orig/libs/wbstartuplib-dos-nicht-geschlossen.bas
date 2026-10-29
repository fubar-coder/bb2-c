; wbstartuplib-dos-nicht-geschlossen (#306) - started from the Workbench,
; the init routine of wbstartuplib opens dos.library (OldOpenLibrary) for
; CurrentDir and never closes it.  wbrun starts the program like the
; Workbench and reports the change of the open count of dos.library over
; the run.  The program uses wbstartuplib (FromCLI), otherwise nothing
; would answer the WBStartup message.
; Correct: the open count is the same after the run (+0).
; @configs clib
; @pass runwrap=@WBRUN@|dosopen
a=FromCLI
End

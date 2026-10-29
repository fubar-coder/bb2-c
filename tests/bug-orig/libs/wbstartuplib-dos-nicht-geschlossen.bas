; wbstartuplib-dos-nicht-geschlossen (#306) - started from the Workbench,
; the init routine of wbstartuplib opens dos.library (OldOpenLibrary) for
; CurrentDir and never closes it.  wbrun starts the program like the
; Workbench and counts how often the program opens dos.library minus how
; often it closes it.  The program begins with WBStartup, which a program
; run from the Workbench needs (manual; wbstartuplib answers the WBStartup
; message).
; Correct: the open count is the same after the run (+0).
; @configs clib
; @pass runwrap=@WBRUN@|dosopen
WBStartup
End

; wbstartuplib-currentdir (#307) - started from the Workbench, the init
; routine of wbstartuplib makes the program's directory (the lock of the
; first WBArg) the current directory and drops the old lock; codefinit
; does not restore it.  wbrun starts the program like the Workbench and
; reports which lock is the current directory when the process ends.
; The program begins with WBStartup, which a program run from the
; Workbench needs (manual; wbstartuplib answers the WBStartup message).
; Correct: the process ends in the directory it was started with.
; @configs clib
; @pass runwrap=@WBRUN@|cd
WBStartup
End

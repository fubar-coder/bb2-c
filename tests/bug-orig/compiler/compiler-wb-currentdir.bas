; compiler-wb-currentdir (#21) - started from the Workbench, the startup
; code of blitz2 (wbheader) changes into the directory of blitz2 and,
; with a project icon, into that of the project, drops the old locks and
; does not restore the current directory at the end.  wbrun starts blitz2
; like the Workbench, with the job list as project icon, and reports
; which lock is the current directory when the process ends.
; The port does not reproduce it (DIFF): libnix's startup code restores
; the directory at exit, so port and clib come out FIXED.
; Correct: the process ends in the directory it was started with.
; @pass wrap=@WBRUN@|cd
NPrint "hello"
End

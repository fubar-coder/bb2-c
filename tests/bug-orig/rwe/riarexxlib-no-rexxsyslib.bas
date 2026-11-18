; riarexxlib-no-rexxsyslib (#1094) - init does not check whether
; rexxsyslib.library opened, and the commands that build a message call
; CreateArgString through the base unconditionally.  The program runs
; through inject with the library failing to open (openlibfail=) and
; sends a command to its own port with RXRunCommand and RXSendCommand.
; Correct: nothing is sent without the library - RXRunCommand 0,
; RXSendCommand ""; the original jumps through a base of 0 (a crash).
; @fixed #1094
; @rwe riarexxlib
; @pass timeout=30
; @pass runwrap=@INJECT@|openlibfail=rexxsyslib.library
DEFTYPE .l
p=RXCreatePort("BUGORIG")
NPrint "created ",p<>0
r=RXRunCommand("BUGORIG","hello")
NPrint "run ",r
a$=RXSendCommand("BUGORIG","hello")
NPrint "send <",a$,">"
End

; compiler-panic-remtask (#24) - PANIC in the stop requester of a program
; run from the editor: menu10 removes the program's process with RemTask,
; nothing of the program is cleaned up.  tedstub runs the program with
; COMPILE/RUN and hits PANIC 2 s after it started; the program has opened
; a file and waits.  Then the program runs from the shell (an argument)
; and asks for an exclusive lock on that file: it fails while the file is
; still open.
; The port's runtheprog runs in the new process without a4 (the base of
; its globals, -fbaserel): the program never starts and the session
; hangs - the verdict is DIFFERS until runtheprog loads a4.
; Correct: the aborted program's file is closed - the exclusive lock works.
; @pass jobline=run=r001|CORPUS:src/compiler-panic-remtask.bb2|RAM:con|100
; @pass runargs=check
If NumPars>0
  l.l=Lock_("RAM:panic.dat",-1)
  NPrint "exclusive lock ",l<>0
  If l Then UnLock_ l
  End
EndIf
If WriteFile(0,"RAM:panic.dat") Then NPrint "file open"
NPrint "waiting"
s.l=Wait_($1000)
CloseFile 0
NPrint "ended"
End

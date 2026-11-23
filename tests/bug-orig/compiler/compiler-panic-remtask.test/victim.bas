; compiler-panic-remtask.test/victim.bas: the program run from the editor
; by the job line of the test: it opens a file and waits
If WriteFile(0,"RAM:panic.dat") Then NPrint "file open"
NPrint "waiting"
s.l=Wait_($1000)
CloseFile 0
NPrint "ended"
End

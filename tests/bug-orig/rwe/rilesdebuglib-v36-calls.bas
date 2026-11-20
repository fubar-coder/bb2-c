; rilesdebuglib-v36-calls (#1148) - D_FindVolume calls LockDosList,
; FindDosEntry and UnLockDosList, dos.library V36 functions, on the
; program's dos.library without checking its version; on Kickstart 1.3
; (dos V34) the offsets lie behind dos.library's jump table: the program
; crashes at D_FindVolume (the session ends in a timeout).
; Correct: on Kickstart 1.x the command walks the DOS device list under
; Forbid (from V36 on it stays as it is): the device RAM, "ram" (names
; compare without case) and the assign SYS are found (an entry, not 0),
; a missing name is 0.
; @fixed #1148
; @rwe rilesdebuglib
; @machine a500 a500-204 a1200
; @pass timeout=30
DEFTYPE .l
NPrint "before"
v$="RAM"
NPrint "RAM: ",D_FindVolume(&v$)<>0
v$="ram"
NPrint "ram: ",D_FindVolume(&v$)<>0
v$="SYS"
NPrint "SYS: ",D_FindVolume(&v$)<>0
v$="RWENOSUCHVOLUME"
NPrint "missing: ",D_FindVolume(&v$)
NPrint "after"
End

; riamosfunclib-v36-doslist (#1145) - FindVolume and DeviceName$ call
; LockDosList, FindDosEntry and UnLockDosList, dos.library V36 functions,
; on the dos.library the library opens in any version; on Kickstart 1.3
; (dos V34) the offsets lie behind dos.library's jump table: the program
; crashes at the first FindVolume (the session ends in a timeout).
; Correct: on Kickstart 1.x the commands walk the DOS device list under
; Forbid (from V36 on they stay as they are): FindVolume of the device
; RAM, of "ram" (names compare without case) and of the assign SYS is
; -1, of a missing name 0; DeviceName$ of RAM: is the name of its handler
; task with ":", of a missing name "".
; @fixed #1145
; @rwe riamosfunclib
; @machine a500 a500-204 a1200
; @pass timeout=30
DEFTYPE .l
NPrint "before"
v$="RAM"
NPrint "RAM: ",FindVolume(&v$)
v$="ram"
NPrint "ram: ",FindVolume(&v$)
v$="SYS"
NPrint "SYS: ",FindVolume(&v$)
v$="RWENOSUCHVOLUME"
NPrint "missing: ",FindVolume(&v$)
NPrint "DeviceName$ RAM: ",DeviceName$("RAM:")
NPrint "DeviceName$ missing: [",DeviceName$("RWENOSUCHVOLUME:"),"]"
NPrint "after"
End

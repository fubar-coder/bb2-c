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
DEFTYPE .l

!TestCase{v36_doslist}
  v$="RAM"
  r=FindVolume(&v$)
  !CheckEq{r,-1,"ram"}
  v$="ram"
  r=FindVolume(&v$)
  !CheckEq{r,-1,"ram_lower"}
  v$="SYS"
  r=FindVolume(&v$)
  !CheckEq{r,-1,"sys"}
  v$="RWENOSUCHVOLUME"
  r=FindVolume(&v$)
  !CheckEq{r,0,"missing"}
  !CheckStrEq{DeviceName$("RAM:"),"RAM:","devicename_ram"}
  !CheckStrEq{DeviceName$("RWENOSUCHVOLUME:"),"","devicename_missing"}
!EndTestCase

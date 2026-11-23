; rilesdebuglib-v36-calls (#1148) - D_FindVolume calls LockDosList,
; FindDosEntry and UnLockDosList, dos.library V36 functions, on the
; program's dos.library without checking its version; on Kickstart 1.3
; (dos V34) the offsets lie behind dos.library's jump table: the program
; crashes at D_FindVolume (the session ends in a timeout).
; Correct: on Kickstart 1.x the command walks the DOS device list under
; Forbid (from V36 on it stays as it is): the device RAM, "ram" (names
; compare without case) and the assign SYS are found (not 0),
; a missing name is 0.
DEFTYPE .l

!TestCase{find_volume}
  v$="RAM"
  r1=(D_FindVolume(&v$)<>0)
  v$="ram"
  r2=(D_FindVolume(&v$)<>0)
  v$="SYS"
  r3=(D_FindVolume(&v$)<>0)
  v$="RWENOSUCHVOLUME"
  r4=D_FindVolume(&v$)
  !CheckEq{r1,-1,"ram"}
  !CheckEq{r2,-1,"ram_lower_case"}
  !CheckEq{r3,-1,"sys"}
  !CheckEq{r4,0,"missing"}
!EndTestCase

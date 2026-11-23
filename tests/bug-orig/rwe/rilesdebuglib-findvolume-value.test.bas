; rilesdebuglib-findvolume-value (#1167) - D_FindVolume returns the DOS
; list entry it found, after it has unlocked the list again (below V36:
; after Permit): an address into a list the program no longer holds.
; Correct: as riamosfunclib's FindVolume, -1 when the name is there, 0
; when it is not, on Kickstart 1.x (a500) as from V36 on.
DEFTYPE .l

!TestCase{volumes}
  v$="RAM"
  r=D_FindVolume(&v$)
  v$="RWENOSUCHVOLUME"
  m=D_FindVolume(&v$)
  !CheckEq{r,-1,"found"}
  !CheckEq{m,0,"missing"}
!EndTestCase

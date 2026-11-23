; riamosfunclib-findvolume-result (#1082) - FindVolume is
; documented (RIAmosFuncLib guide) to return 0 if the volume is not
; present and -1 if it is.  The original returns what FindDosEntry gave:
; the address of the DosList entry, taken after UnLockDosList (an entry
; of a list the program no longer holds).  As a truth value it works; a
; program that compares the result with -1 never finds the volume.
; Correct: FindVolume of RAM (always mounted) is -1, of a missing name 0.
DEFTYPE .l

!TestCase{findvolume_result}
  v$="RAM"
  r=FindVolume(&v$)
  !CheckEq{r,-1,"present"}
  v$="RWENOSUCHVOLUME"
  m=FindVolume(&v$)
  !CheckEq{m,0,"missing"}
!EndTestCase

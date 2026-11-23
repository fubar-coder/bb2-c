; ristonecrackerlib-dos-never-closed (#834) - the library's init opens
; dos.library and keeps the base; finit is empty, so it is never closed.
; The program runs through inject, which counts the opens minus the
; closes of dos.library by the process (opencount=).
; Correct: dos.library is closed again (+0); the original leaves it +1.
DEFTYPE .l

!TestCase{dos_never_closed}
  r=STCLength(0)
  !CheckEq{r,0,"length"}
!EndTestCase

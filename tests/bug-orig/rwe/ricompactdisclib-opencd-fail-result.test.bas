; ricompactdisclib-opencd-fail-result (#575) - OpenCD returns -1 when the
; device opened; when OpenDevice fails it deletes the request and returns
; what DeleteIORequest leaves in d0.
; Correct: 0 for a device that does not exist.
DEFTYPE .l

!TestCase{no_device}
  r=OpenCD("nonexistent.device",0)
  !CheckEq{r,0,"result"}
!EndTestCase

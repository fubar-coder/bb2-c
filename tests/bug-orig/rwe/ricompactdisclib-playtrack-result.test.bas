; ricompactdisclib-playtrack-result (#1157) - the manual: CDPlayTrack "A
; return value of true means that the command succeeded, else false means
; failure".  Without scsiflag it sends CD_PLAYTRACK with SendIO, which has
; no result: CDPlayTrack returns whatever SendIO leaves in d0.  The
; machine is the A1200 with uaescsi.device and the library's CD image.
; Correct: -1 when the play was sent.
DEFTYPE .l

!TestCase{play}
  r=OpenCD("uaescsi.device",0)
  !CheckEq{r,-1,"open"}
  !Require{r<>0,"opened"}
  p=CDPlayTrack(1,1)
  !CheckEq{p,-1,"play"}
  CDStop
  CloseCD
!EndTestCase

; ricompactdisclib-tracklength-range (#583, the check) - the manual: "The
; track# should be checked to make sure that it exists on the compact
; disc"; CDTrackLength of a track behind the last reads TOC entries that
; do not exist (here, with scsiflag, the other TOC buffer: address 0 and
; the exception vectors).  The track is an argument the programmer
; controls; the original has no error check area.  Here with runtime
; error checking.  The machine is the A1200 with uaescsi.device and the
; library's CD image (two tracks).
; Correct: the error check refuses track 3 after "before".
DEFTYPE .l

!TestCase{track3}
  r=OpenCD("uaescsi.device",0,-1)
  !Require{r<>0,"opened_scsi"}
  t=CDReadTOC
  !ExpectError{"Track number out of range","refused"}
  trklen3=CDTrackLength(3)
  CloseCD
!EndTestCase

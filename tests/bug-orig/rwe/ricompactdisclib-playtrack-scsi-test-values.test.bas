; ricompactdisclib-playtrack-scsi-test-values (#580) - CDPlayTrack with
; scsiflag ignores its arguments: it sends PLAY AUDIO(12) for the blocks
; 14000 to 15000 (the author's test values), behind the end of the
; library's CD image (750 sectors), and returns DoIO's error code.  The
; machine is the A1200 with uaescsi.device and the library's CD image.
; Correct: PLAY AUDIO(12) from the SCSI TOC (the start of the track to
; the start of the track behind the last one played, or the lead-out);
; -1 when it worked, 0 otherwise: 0 before CDReadTOC, -1 for tracks 1
; and 2, 0 for track 3 (there is none).
DEFTYPE .l

!TestCase{tracks}
  r=OpenCD("uaescsi.device",0,-1)
  !CheckEq{r,-1,"open_scsi"}
  !Require{r<>0,"opened_scsi"}
  p0=CDPlayTrack(1,1)
  !CheckEq{p0,0,"before_toc"}
  t=CDReadTOC
  p1=CDPlayTrack(1,1)
  !CheckEq{p1,-1,"track1"}
  p2=CDPlayTrack(2,1)
  !CheckEq{p2,-1,"track2"}
  p3=CDPlayTrack(3,1)
  !CheckEq{p3,0,"track3"}
  CloseCD
!EndTestCase

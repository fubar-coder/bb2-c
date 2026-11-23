; ricompactdisclib-playtrack-scsi-odd (#579) - CDPlayTrack on the SCSI path
; (OpenCD with scsiflag) fills the 12-byte command block with
; MOVE.b ...,(a0)+ and then two MOVE.l and a CLR.w through the now odd a0:
; on a 68000 an address error (the session ends in a timeout).  The
; machine is the A600 (68000) with uaescsi.device and a CD image
; (generated: two audio tracks).
; Correct: CDPlayTrack works on a 68000, the program prints "played".
DEFTYPE .l

!TestCase{play_scsi}
  r=OpenCD("uaescsi.device",0,-1)
  !CheckEq{r,-1,"open_scsi"}
  !Require{r<>0,"opened_scsi"}
  CDReadTOC
  ntr=CDNumTracks
  !CheckEq{ntr,2,"tracks_scsi"}
  CDPlayTrack 1,1
  CloseCD
!EndTestCase

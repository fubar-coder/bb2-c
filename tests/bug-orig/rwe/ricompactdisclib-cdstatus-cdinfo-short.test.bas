; ricompactdisclib-cdstatus-cdinfo-short (#948) - CDStatus asks for 26
; bytes of CD_INFO into MyCDInfo (13 words), the last data of the
; library; a struct CDInfo has 34 bytes, and uaescsi.device writes all of
; them: 8 bytes go behind the library, into the memory behind the
; executable.  On Workbench 3.1 they hit the free chunk that follows it:
; the program's unloading after End stops with the alert 8100 0005
; (corrupt memory list), the session ends in a timeout.  After CDStatus
; the program opens the SCSI path and reads its TOC (the port before #946
; kept its SCSI buffer pointers behind MyCDInfo and hung there).  The
; machine is the A1200 with uaescsi.device and the library's CD image.
; Correct: the program ends normally after "played".
DEFTYPE .l

!TestCase{cdinfo}
  r=OpenCD("uaescsi.device",0)
  !CheckEq{r,-1,"open"}
  !Require{r<>0,"opened"}
  CDReadTOC
  sts=CDStatus
  !CheckEq{sts,7,"status"}
  CloseCD
  r=OpenCD("uaescsi.device",0,-1)
  !CheckEq{r,-1,"open_scsi"}
  !Require{r<>0,"opened_scsi"}
  CDReadTOC
  ntr=CDNumTracks
  !CheckEq{ntr,2,"tracks_scsi"}
  CDPlayTrack 1,1
  CloseCD
!EndTestCase

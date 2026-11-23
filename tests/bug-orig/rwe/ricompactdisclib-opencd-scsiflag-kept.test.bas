; ricompactdisclib-opencd-scsiflag-kept (#577) - only OpenCD with three
; arguments sets scsiflag, and only the library's init clears it: OpenCD
; name$,unit after OpenCD name$,unit,-1 still takes the SCSI path
; (CDReadTOC reads the SCSI TOC, CDNumTracks counts its tracks).  OpenCD
; without arguments opens the name and unit of the OpenCD before; the
; manual: "ommiting the optional parameter of this command will cause the
; default cd.device unit 0 to be opened".  The machine is the A1200 with
; uaescsi.device and the library's CD image; it has no cd.device.
; Correct: after OpenCD name$,unit the cd.device path (uaescsi.device
; does not answer its TOC command: no TOC, 0 tracks); OpenCD without
; arguments opens cd.device unit 0 (fails here: 0).
DEFTYPE .l

!TestCase{flag_kept}
  r=OpenCD("uaescsi.device",0,-1)
  !CheckEq{r,-1,"open_scsi"}
  !Require{r<>0,"opened_scsi"}
  t=CDReadTOC
  ntr=CDNumTracks
  !CheckEq{ntr,2,"tracks_scsi"}
  CloseCD
  r=OpenCD("uaescsi.device",0)
  !CheckEq{r,-1,"open"}
  !Require{r<>0,"opened"}
  t=CDReadTOC
  ntr=CDNumTracks
  !CheckEq{ntr,0,"tracks"}
  CloseCD
  r=OpenCD
  !CheckEq{r,0,"open_default"}
  If r Then CloseCD
!EndTestCase

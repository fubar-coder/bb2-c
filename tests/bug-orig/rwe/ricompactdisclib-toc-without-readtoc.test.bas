; ricompactdisclib-toc-without-readtoc (#582) - CDNumTracks, CDFirstTrack,
; CDLastTrack and CDTrackLength read the TOC buffer of CDReadTOC without a
; test: before any CDReadTOC its pointer is 0, and they read the bytes at
; address 0 and the exception vectors behind it.  With scsiflag CDReadTOC
; reads the TOC into a buffer of its own (READ TOC), but only CDNumTracks
; looks there: CDFirstTrack, CDLastTrack and CDTrackLength still read the
; other buffer (address 0).  The machine is the A1200 with uaescsi.device
; and the library's CD image (two tracks of 300 and 450 sectors).
; Correct: 0 for all four without a TOC; with the SCSI TOC first track 1,
; last track 2, the lengths 4 and 6 seconds.
DEFTYPE .l

!TestCase{no_toc}
  r=OpenCD("uaescsi.device",0)
  !CheckEq{r,-1,"open"}
  !Require{r<>0,"opened"}
  ntr=CDNumTracks
  frs=CDFirstTrack
  lst=CDLastTrack
  trklen1=CDTrackLength(1)
  !CheckEq{ntr,0,"num_tracks"}
  !CheckEq{frs,0,"first_track"}
  !CheckEq{lst,0,"last_track"}
  !CheckEq{trklen1,0,"length1"}
  CloseCD
!EndTestCase

!TestCase{scsi_toc}
  r=OpenCD("uaescsi.device",0,-1)
  !CheckEq{r,-1,"open_scsi"}
  !Require{r<>0,"opened_scsi"}
  t=CDReadTOC
  ntr=CDNumTracks
  frs=CDFirstTrack
  lst=CDLastTrack
  trklen1=CDTrackLength(1)
  trklen2=CDTrackLength(2)
  !CheckEq{ntr,2,"num_tracks"}
  !CheckEq{frs,1,"first_track"}
  !CheckEq{lst,2,"last_track"}
  !CheckEq{trklen1,4,"length1"}
  !CheckEq{trklen2,6,"length2"}
  CloseCD
!EndTestCase

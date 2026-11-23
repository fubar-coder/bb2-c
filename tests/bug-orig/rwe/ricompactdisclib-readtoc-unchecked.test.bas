; ricompactdisclib-readtoc-unchecked (#581) - CDReadTOC reads the summary
; of the TOC (CD_TOCMSF, one entry) without checking the result, allocates
; the TOC buffer from it (not cleared) and reads the entries.  When the
; reads fail (uaescsi.device does not answer CD_TOCMSF) it returns 0, but
; the buffer stays: CDNumTracks, CDFirstTrack and CDLastTrack read the
; uncleared memory.  The machine is the A1200 with uaescsi.device and the
; library's CD image.
; Correct: CDReadTOC returns 0 and leaves no TOC: the TOC functions
; return 0.
DEFTYPE .l

!TestCase{no_answer}
  r=OpenCD("uaescsi.device",0)
  !CheckEq{r,-1,"open"}
  !Require{r<>0,"opened"}
  toc=CDReadTOC
  !CheckEq{toc,0,"toc"}
  ntr=CDNumTracks
  frs=CDFirstTrack
  lst=CDLastTrack
  !CheckEq{ntr,0,"num_tracks"}
  !CheckEq{frs,0,"first_track"}
  !CheckEq{lst,0,"last_track"}
  CloseCD
!EndTestCase

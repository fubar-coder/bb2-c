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
; @rwe ricompactdisclib
; @pass uae=scsi=true
; @pass uae=cdimage0=@TESTDATA@/ricompactdisclib.cue,image
; @pass timeout=60
DEFTYPE .l
r=OpenCD("uaescsi.device",0)
NPrint "open ",r
If r
  NPrint "no toc ",CDNumTracks," ",CDFirstTrack," ",CDLastTrack," ",CDTrackLength(1)
  CloseCD
EndIf
r=OpenCD("uaescsi.device",0,-1)
NPrint "open scsi ",r
If r
  t=CDReadTOC
  NPrint "scsi toc ",CDNumTracks," ",CDFirstTrack," ",CDLastTrack
  NPrint "lengths ",CDTrackLength(1)," ",CDTrackLength(2)
  CloseCD
EndIf
End

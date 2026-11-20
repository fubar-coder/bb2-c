; ricompactdisclib-playtrack-scsi-test-values (#580) - CDPlayTrack with
; scsiflag ignores its arguments: it sends PLAY AUDIO(12) for the blocks
; 14000 to 15000 (the author's test values), behind the end of the
; library's CD image (750 sectors), and returns DoIO's error code.  The
; machine is the A1200 with uaescsi.device and the library's CD image.
; Correct: PLAY AUDIO(12) from the SCSI TOC (the start of the track to
; the start of the track behind the last one played, or the lead-out);
; -1 when it worked, 0 otherwise: 0 before CDReadTOC, -1 for tracks 1
; and 2, 0 for track 3 (there is none).
; @fixed #580
; @rwe ricompactdisclib
; @pass uae=scsi=true
; @pass uae=cdimage0=@TESTDATA@/ricompactdisclib.cue,image
; @pass timeout=60
DEFTYPE .l
r=OpenCD("uaescsi.device",0,-1)
NPrint "open scsi ",r
If r
  NPrint "before toc ",CDPlayTrack(1,1)
  t=CDReadTOC
  NPrint "track 1 ",CDPlayTrack(1,1)
  NPrint "track 2 ",CDPlayTrack(2,1)
  NPrint "track 3 ",CDPlayTrack(3,1)
  CloseCD
EndIf
End

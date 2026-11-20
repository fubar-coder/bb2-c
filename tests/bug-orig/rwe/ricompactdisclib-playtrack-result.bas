; ricompactdisclib-playtrack-result (#1157) - the manual: CDPlayTrack "A
; return value of true means that the command succeeded, else false means
; failure".  Without scsiflag it sends CD_PLAYTRACK with SendIO, which has
; no result: CDPlayTrack returns whatever SendIO leaves in d0.  The
; machine is the A1200 with uaescsi.device and the library's CD image.
; Correct: -1 when the play was sent.
; @fixed #1157
; @rwe ricompactdisclib
; @pass uae=scsi=true
; @pass uae=cdimage0=@TESTDATA@/ricompactdisclib.cue,image
; @pass timeout=60
DEFTYPE .l
r=OpenCD("uaescsi.device",0)
NPrint "open ",r
If r
  NPrint "play ",CDPlayTrack(1,1)
  CDStop
  CloseCD
EndIf
End

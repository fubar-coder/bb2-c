; ricompactdisclib-tracklength-range (#583, the check) - the manual: "The
; track# should be checked to make sure that it exists on the compact
; disc"; CDTrackLength of a track behind the last reads TOC entries that
; do not exist (here, with scsiflag, the other TOC buffer: address 0 and
; the exception vectors).  The track is an argument the programmer
; controls; the original has no error check area.  Here with runtime
; error checking.  The machine is the A1200 with uaescsi.device and the
; library's CD image (two tracks).
; Correct: the error check refuses track 3 after "before".
; @rwe ricompactdisclib
; @pass uae=scsi=true
; @pass uae=cdimage0=@TESTDATA@/ricompactdisclib.cue,image
; @pass timeout=60
; @pass runtime_errors=on
DEFTYPE .l
r=OpenCD("uaescsi.device",0,-1)
If r
  t=CDReadTOC
  NPrint "before"
  NPrint CDTrackLength(3)
  CloseCD
EndIf
End

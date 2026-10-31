; ricompactdisclib-playtrack-scsi-odd (#579) - CDPlayTrack on the SCSI path
; (OpenCD with scsiflag) fills the 12-byte command block with
; MOVE.b ...,(a0)+ and then two MOVE.l and a CLR.w through the now odd a0:
; on a 68000 an address error (the session ends in a timeout).  The
; machine is the A600 (68000) with uaescsi.device and a CD image
; (generated: two audio tracks).
; Correct: CDPlayTrack works on a 68000, the program prints "played".
; @rwe ricompactdisclib
; @machine a600
; @pass uae=scsi=true
; @pass uae=cdimage0=@TESTDATA@/ricompactdisclib-playtrack-scsi-odd.cue,image
; @pass timeout=60
DEFTYPE .l
r=OpenCD("uaescsi.device",0,-1)
NPrint "open scsi ",r
If r
  CDReadTOC
  NPrint "tracks scsi ",CDNumTracks
  CDPlayTrack 1,1
  NPrint "played"
  CloseCD
EndIf
End

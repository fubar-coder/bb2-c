; ricompactdisclib-scsi-after-plain (#946) - a port regression, not a bug
; of the original: a session without scsiflag (CDReadTOC, CDStatus,
; CloseCD), then OpenCD with scsiflag and CDReadTOC.  CDStatus's CD_INFO
; asks for 26 bytes, but uaescsi.device writes the whole CDInfo (34
; bytes): behind the original's MyCDInfo that hit nothing that mattered,
; in the C port it overwrote the pointers of the SCSI buffers, and the
; second CDReadTOC hung.  The machine is the A1200 with uaescsi.device
; and the CD image of the test of #579.
; Expected (what the original does): the SCSI session goes on and plays.
; @rwe ricompactdisclib
; @pass uae=scsi=true
; @pass uae=cdimage0=@TESTDATA@/ricompactdisclib-playtrack-scsi-odd.cue,image
; @pass timeout=60
DEFTYPE .l
r=OpenCD("uaescsi.device",0)
NPrint "open ",r
If r
  CDReadTOC
  NPrint "status ",CDStatus
  CloseCD
EndIf
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

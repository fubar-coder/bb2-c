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
; @fixed #948
; @rwe ricompactdisclib
; @pass uae=scsi=true
; @pass uae=cdimage0=@TESTDATA@/ricompactdisclib.cue,image
; @pass timeout=30
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

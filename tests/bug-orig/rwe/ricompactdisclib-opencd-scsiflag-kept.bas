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
; @rwe ricompactdisclib
; @pass uae=scsi=true
; @pass uae=cdimage0=@TESTDATA@/ricompactdisclib.cue,image
; @pass timeout=60
DEFTYPE .l
r=OpenCD("uaescsi.device",0,-1)
NPrint "open scsi ",r
If r
  t=CDReadTOC
  NPrint "tracks scsi ",CDNumTracks
  CloseCD
EndIf
r=OpenCD("uaescsi.device",0)
NPrint "open ",r
If r
  t=CDReadTOC
  NPrint "tracks ",CDNumTracks
  CloseCD
EndIf
r=OpenCD
NPrint "open default ",r
If r
  CloseCD
EndIf
End

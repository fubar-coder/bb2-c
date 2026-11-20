; ricompactdisclib-readtoc-unchecked (#581) - CDReadTOC reads the summary
; of the TOC (CD_TOCMSF, one entry) without checking the result, allocates
; the TOC buffer from it (not cleared) and reads the entries.  When the
; reads fail (uaescsi.device does not answer CD_TOCMSF) it returns 0, but
; the buffer stays: CDNumTracks, CDFirstTrack and CDLastTrack read the
; uncleared memory.  The machine is the A1200 with uaescsi.device and the
; library's CD image.
; Correct: CDReadTOC returns 0 and leaves no TOC: the TOC functions
; return 0.
; @rwe ricompactdisclib
; @pass uae=scsi=true
; @pass uae=cdimage0=@TESTDATA@/ricompactdisclib.cue,image
; @pass timeout=60
DEFTYPE .l
r=OpenCD("uaescsi.device",0)
NPrint "open ",r
If r
  NPrint "toc ",CDReadTOC
  NPrint "tracks ",CDNumTracks," ",CDFirstTrack," ",CDLastTrack
  CloseCD
EndIf
End

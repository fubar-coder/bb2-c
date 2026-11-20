; ricompactdisclib-opencd-while-open-leak (#578) - the manual: "you must
; call CloseCD between calls to OpenCD"; OpenCD while a device is open
; makes a new request and opens the device again: the open request is
; lost and stays open.  A call the programmer controls; the original has
; no error check area.  Here with runtime error checking.  The machine is
; the A1200 with uaescsi.device and the library's CD image.
; Correct: the error check refuses the second OpenCD after "second".
; @rwe ricompactdisclib
; @pass uae=scsi=true
; @pass uae=cdimage0=@TESTDATA@/ricompactdisclib.cue,image
; @pass timeout=60
; @pass runtime_errors=on
DEFTYPE .l
r=OpenCD("uaescsi.device",0)
NPrint "open ",r
NPrint "second"
r=OpenCD("uaescsi.device",0)
NPrint "open ",r
CloseCD
End

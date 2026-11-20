; ricompactdisclib-updateinfo-no-device (#587) - CDUpdateInfo returns the
; address of the Q-code's minutes (qcodeinfo + 5); without an open device
; it returns at once, with a0 + 5: a0 is whatever the caller left there.
; Correct: 0 without an open device.
; @rwe ricompactdisclib
DEFTYPE .l
a=CDUpdateInfo
NPrint a
End

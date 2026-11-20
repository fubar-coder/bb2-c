; ricompactdisclib-opencd-fail-result (#575) - OpenCD returns -1 when the
; device opened; when OpenDevice fails it deletes the request and returns
; what DeleteIORequest leaves in d0.
; Correct: 0 for a device that does not exist.
; @fixed #575
; @rwe ricompactdisclib
DEFTYPE .l
r=OpenCD("nonexistent.device",0)
NPrint r
End

; rilesdebuglib-extractlabel-space (#704) - ExtractLabel copies the label
; behind the '.' up to a 0 or ':'; the source also ends it at a space
; (CMPI.b #" ",d2), which the CD's blitz2 2.10 assembles as CMPI.b #0
; (compiler bug #343): a space does not end the label.
; Correct: the label of "012345678.a b" is "a".
; @fixed #704
; @rwe rilesdebuglib
DEFTYPE .l
b=AllocMem_(64,$10001)
If b=0 Then NPrint "no memory" : End
Poke$ b,"012345678.a b"
a$=ExtractLabel(b)
FreeMem_ b,64
NPrint "<",a$,">"
End

; ricopperfxlib-getccoffset-resetadr (#593) - GetCCOffset decides whether
; there is a custom copper space by testing coplistaddr, all other
; commands test customaddr.  CopperResetAdr sets customaddr to its
; address and coplistaddr to 0: the commands write their instructions
; and advance the offset, but GetCCOffset returns -1.
; Correct: GetCCOffset returns the offset (1 after one CopperMove).
; @fixed #593
; @rwe ricopperfxlib
DEFTYPE .l
m=AllocMem_(4096,$10000)
If m=0 Then NPrint "no memory" : End
CopperResetAdr m,44
CopperMove $180,$fff
o=GetCCOffset
NPrint "offset ",o
FreeMem_ m,4096
End

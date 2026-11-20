; rilesdebuglib-gethexadr-odd (#1168) - GetHexAdr$ reads the word or long
; at the address with one MOVE.w or MOVE.l: at an odd address an address
; error on a 68000 (the session ends in a timeout).
; Correct: the word and the long at an odd address in hex, read byte by
; byte as GetDataItem does.
; @fixed #1168
; @rwe rilesdebuglib
; @machine a500
; @pass timeout=30
DEFTYPE .l
b=AllocMem_(16,$10001)
If b=0 Then NPrint "no memory" : End
Poke.b b+1,$11 : Poke.b b+2,$22 : Poke.b b+3,$33 : Poke.b b+4,$44
NPrint "before"
NPrint GetHexAdr$(b+1,2)
NPrint GetHexAdr$(b+1,3)
FreeMem_ b,16
End

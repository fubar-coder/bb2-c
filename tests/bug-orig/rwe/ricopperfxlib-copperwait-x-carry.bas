; ricopperfxlib-copperwait-x-carry (#591) - CopperWait builds the first
; word of the WAIT as (y << 8) | (((x >> 1) & $FE) + 6) | 1: the +6 is
; added after the mask, so for x from 500 on the sum carries into bit 8,
; the line.  CopperWait 500,$40 waits for line $41.
; Correct: the line byte is y ($40) for every x; x = 498 as the control.
; @fixed #591
; @rwe ricopperfxlib
DEFTYPE .l
m=AllocMem_(64,$10000)
If m=0 Then NPrint "no memory" : End
CopperResetAdr m,0
CopperWait 498,$40
CopperWait 500,$40
NPrint "x 498: line ",Peek.b(m)&255
NPrint "x 500: line ",Peek.b(m+4)&255
FreeMem_ m,64
End

; rilesdebuglib-copins-odd-digits (#711) - GetCopIns$ shows the
; horizontal position of a WAIT or SKIP and of its mask with both hex
; digits ANDed with $E: an odd high digit loses its bit 0.  (Bit 0 of the
; low digit is the instruction bit and the WAIT/SKIP bit, not a position
; bit.)
; Correct: $2C31,$FFFE is "WAIT (30,2C) MASK (FE,FF) [BFD]", $5BD1,$7FFF
; "SKIP (D0,5B) MASK (FE,7F)".
; @fixed #711
; @rwe rilesdebuglib
DEFTYPE .l
c=AllocMem_(16,$10001)
If c=0 Then NPrint "no memory" : End
Poke.w c,$2C31 : Poke.w c+2,$FFFE
NPrint GetCopIns$(c)
Poke.w c,$5BD1 : Poke.w c+2,$7FFF
NPrint GetCopIns$(c)
FreeMem_ c,16
End

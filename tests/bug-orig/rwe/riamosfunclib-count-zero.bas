; riamosfunclib-count-zero (#492) - CopyByte (like CopyWord, CopyLong and
; FillMem) tests the count only after the first unit (SUBQ.l #1 : BNE):
; a count of 0 copies 2^32 bytes over all memory from the destination on
; - a crash (the session ends in a timeout).
; Correct: a count of 0 copies nothing, the program goes on.
; @rwe riamosfunclib
DEFTYPE .l
a=AllocMem_(16,$10001)
b=AllocMem_(16,$10001)
NPrint "before"
CopyByte a,b,0
NPrint "after"
FreeMem_ a,16 : FreeMem_ b,16
End

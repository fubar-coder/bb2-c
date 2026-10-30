; rifxlib-delay-zero (#669) - ClearBitmap busy-waits after each line with
; SUBQ.l #1 / BNE on the delay: a delay of 0 becomes $FFFFFFFF, 2^32
; turns per line (21 lines for a height of 1) - the program hangs (the
; session ends in a timeout).
; Correct: a delay of 0 does not wait, the program goes on.
; @rwe rifxlib
BitMap 0,320,16,1
NPrint "before"
ClearBitmap 0,0,1,0
NPrint "after"
End

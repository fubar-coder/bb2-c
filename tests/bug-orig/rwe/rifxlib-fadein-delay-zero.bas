; rifxlib-fadein-delay-zero (#669) - FadeInBitmap busy-waits after each
; line with SUBQ.l #1 / BNE on the delay, as ClearBitmap: a delay of 0
; becomes $FFFFFFFF, 2^32 turns per line - the program hangs (the session
; ends in a timeout).  The manual says not to pass 0; the original has no
; error check area.  Here with runtime error checking.
; Correct: a FadeInBitmap with a delay of 1 works; the error check
; refuses the delay of 0: "Delay must not be 0".
; @rwe rifxlib
; @pass timeout=30
; @pass runtime_errors=on
BitMap 0,320,16,1
BitMap 1,320,16,1
FadeInBitmap 0,1,1,1,0,0
NPrint "before"
FadeInBitmap 0,1,0,1,0,0
NPrint "after"
End

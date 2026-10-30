; rigfxlib-cpucls-no-plane-crash (#680) - CPUCls tests the first plane
; pointer before it saves a4-a6 (MOVEM.l a4-a6,-(a7)); without a plane it
; branches to the end of the routine, which restores a4-a6 from the
; stack and returns: it pops the return address and two longs of the
; caller into a4-a6 and returns to the next long - a crash (the session
; ends in a timeout).  Bitmap 5 was never set up: its plane 0 is 0.
; Correct: CPUCls returns without doing anything, the program goes on.
; rigfxlib needs RIAmosFuncLib (SaveCMAP calls its BSave).
; @rwe rigfxlib riamosfunclib
NPrint "before"
CPUCls 5
NPrint "after"
End

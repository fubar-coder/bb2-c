; rifnslib-print-no-bitmap-crash (#665) - FNSPrint saves 13 registers
; (MOVEM.l d0-d6/a1-a6,-(a7)) and only then tests for an output bitmap;
; without one it leaves with a plain RTS: it returns to the pushed d0 (the
; font number 0) - a crash (the session ends in a timeout).
; Correct: FNSPrint without FNSOutput does nothing, the program goes on.
; rifnslib needs RIAmosFuncLib (FNSLoad and FNSUnload use its banks).
; @rwe rifnslib riamosfunclib
NPrint "before"
FNSPrint 0,10,10,"Hello"
NPrint "after"
End

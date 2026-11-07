; compiler-dcb-zero (#37) - dodcb counts the copies down in do-while loops:
; a count of 0 makes 2^32 copies, the compiler runs for hours (here: the
; session's time limit).
; Correct: Dcb with a count of 0 emits nothing.
; @fixed #37
; @pass timeout=30
Goto skip
.la
Dcb.b 0,1
.lb
Dcb.w 0,1
.lc
Dcb.l 0,1
.ld
.skip
NPrint ?lb-?la
NPrint ?lc-?lb
NPrint ?ld-?lc
End

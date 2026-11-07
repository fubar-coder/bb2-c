; compiler-dcb-negative (#37) - dodcb takes the count as unsigned: a count
; of -1 makes 2^32-1 copies, the compiler runs for hours (here: the
; session's time limit).
; Correct: a negative count is an error.
; @pass timeout=30
Goto skip
Dcb.w -1,0
.skip
NPrint "ok"
End

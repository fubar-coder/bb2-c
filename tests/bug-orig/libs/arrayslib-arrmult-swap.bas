; arrayslib-arrmult-swap (#58) - arrmult (size = size * (dimension+1),
; once per dimension) adds the second 16 bit product with SWAP instead of
; shifting it: its high word lands in the low word of the result.  It is
; only <> 0 when the true size is >= 4 GB.  Dim a.l(32767,32767) is 4 GB:
; the second dimension multiplies $20000 by $8000, the high word 1 of
; $10000 is SWAPped to 1 - the array gets 1 byte, the Dim succeeds.
; Correct: a size that does not fit in 32 bits is an error at the Dim.
; Such a size is a programming error: with runtime error checking (here)
; the error check of arrmult reports it, "Array Size out of Range"; the
; program does not go on.  (Without it the allocation fails, memlib's
; "Out of Memory!"; only fixing the SWAP is not enough: the size modulo
; 2^32 is 0, and that Dim succeeds as well.)
; @fixed #58
; @configs clib
; @pass runtime_errors=on
Dim a.l(32767,32767)
NPrint "dimmed"
End

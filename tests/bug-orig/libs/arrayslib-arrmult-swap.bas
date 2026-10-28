; arrayslib-arrmult-swap (#58) - arrmult (size = size * (dimension+1),
; once per dimension) adds the second 16 bit product with SWAP instead of
; shifting it: its high word lands in the low word of the result.  It is
; only <> 0 when the true size is >= 4 GB.  Dim a.l(32767,32767) is 4 GB:
; the second dimension multiplies $20000 by $8000, the high word 1 of
; $10000 is SWAPped to 1 - the array gets 1 byte, the Dim succeeds.
; Correct (fix of the issue: an error when the upper word is <> 0, i.e.
; the size does not fit in 32 bits): a runtime error at the Dim, which
; the debugger shows (runtime error checking on); the program does not
; go on.  Only fixing the SWAP is not enough: the size modulo 2^32 is 0,
; and that Dim succeeds as well.
; @configs clib
; @pass runtime_errors=on
Dim a.l(32767,32767)
NPrint "dimmed"
End

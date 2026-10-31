; rievallib-indirect-odd-long (#636) - the indirection {address}.b of
; PFEvaluate always reads a long at the address and takes its top byte: an
; odd address is an address error on a 68000 (the session ends in a
; timeout).  The machine is the A600 (68000); the byte $5A is put at an
; odd address of a block of its own (the operand in lower case: PFEvaluate
; takes the postfix form, whose hex digits are a-f).  Library 121 is the
; acidlibs' polylib on the CD: ref and port get the acidlibs of the
; variant rwe-1999-eval too (C build, without polylib).
; Correct: {address}.b reads the byte, 90.
; @rwe rievallib
; @machine a600
; @pass timeout=30
; @pass file=blitz2/acidlibs|@CMAKE_BINARY_DIR@/variants/rwe-1999-eval/acidlibs|ref
; @pass file=blitz2/acidlibs|@CMAKE_BINARY_DIR@/variants/rwe-1999-eval/acidlibs|port
DEFTYPE .l
m=AllocMem_(16,$10001)
If m=0 Then NPrint "no memory" : End
Poke.b m+5,$5A
NPrint "before"
NPrint PFEvaluate(LCase$("{$"+Hex$(m+5)+"}.b"))
FreeMem_ m,16
End

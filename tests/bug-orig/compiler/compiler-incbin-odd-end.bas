; compiler-incbin-odd-end (#979) - at the end of the program the compiler
; stores the address of the end code (endop) and emits the library finits
; and "moveq #0,d0 / rts" without aligning pc. An IncBin of odd length as
; the last code-generating statement leaves that code on an odd address:
; End jumps there (address error).
; Correct: the program prints and ends.
; @pass timeout=30
; @pass file=odd.bin|@TESTDATA@/compiler-incbin-odd-end.bin
NPrint "before"
End
IncBin "CORPUS:odd.bin"

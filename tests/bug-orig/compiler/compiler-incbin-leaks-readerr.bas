; compiler-incbin-leaks-readerr (#38) - doincbin reports a read error of
; the IncBin file ("Error Reading File") without closing the file: it
; stays open (until the reboot).  inject makes the first Read on
; CORPUS:incbin.bin fail and checks after blitz2 whether the file is still
; in use (an exclusive Lock fails).  The directory case is
; compiler-incbin-leaks.
; Correct: the error, and the file is not in use.
; @pass file=incbin.bin|@TESTDATA@/compiler-incbin-leaks-readerr.bin
; @pass wrap=@INJECT@|readfail=incbin.bin/1 inuse=CORPUS:incbin.bin
NPrint "hello"
IncBin "CORPUS:incbin.bin"
End

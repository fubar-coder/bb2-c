; rimisclib-openfontprefs-alloc-unchecked (#756) - OpenFontPrefs does not
; check the AllocMem of its 16384 byte buffer: if it fails, ENV:SYS/
; Font.Prefs is read to address 0 and overwrites the low memory (the
; ExecBase pointer at 4) - a crash that halts the CPU some 25 s later (the
; session ends in a halt; the limit leaves room for it).
; inject makes the first AllocMem of that size fail (allocfail=).
; Correct: without its buffer OpenFontPrefs reads nothing and returns 0.
; @rwe rimisclib
; The program is silent for about 11 s (more than BUG_ORIG_RUN_HANG_AFTER):
; @pass hang_after=30
; @pass runwrap=@INJECT@|allocfail=16384-16384/1
; @pass timeout=60
NPrint "before"
x.w=OpenFontPrefs
NPrint x
End

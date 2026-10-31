; compiler-incbin-leaks (#38) - doincbin tests the type of the name before
; it unlocks it: IncBin of a directory reports "Can't open Include" and
; leaves the lock behind (until the reboot).  Here IncBin names the
; directory CORPUS:bindir (dir=); inject checks after blitz2 whether it is
; still in use (an exclusive Lock fails).  The read error is
; compiler-incbin-leaks-readerr.
; Correct: the error, and the directory is not in use.
; @pass dir=bindir
; @pass wrap=@INJECT@|inuse=CORPUS:bindir
NPrint "hello"
IncBin "CORPUS:bindir"
End

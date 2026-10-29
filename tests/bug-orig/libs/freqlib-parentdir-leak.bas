; freqlib-parentdir-leak (#158) - FileRequest$ builds the path of the
; current directory with ParentDir up to the root (with an empty path,
; then PARENT; after CD) and never UnLocks those locks.  The program runs
; through inject in CORPUS:a/b (cd=; dir= makes it) with an empty path;
; inject clicks PARENT, then CANCEL (click=), and afterwards checks if
; CORPUS:a is still in use (inuse=: an exclusive Lock fails).
; Own 320x256 screen (see t93_freq).
; Correct: no lock is left, CORPUS:a is not in use.  The original leaves
; the ParentDir locks of CORPUS:a and CORPUS: behind.
; @configs clib
; @pass dir=a/b
; @pass runwrap=@INJECT@|cd=CORPUS:a/b inuse=CORPUS:a click=FQ/PARENT click=FQ/CANCEL
; @pass timeout=60
Screen 0,0,0,320,256,2,0,"freqlib",1,2
Use Screen 0
MaxLen p$=160
MaxLen f$=64
p$=""
f$="x"
r$=FileRequest$("FQ",p$,f$)
NPrint "cancel ",Len(r$)
End

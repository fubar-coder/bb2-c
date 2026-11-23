; freqlib-cd-currentdir (#157) - the gadget "CD" of FileRequest$ makes the
; path the current directory of the process (CurrentDir) and UnLocks the
; old one, which belongs to whoever started the program (the shell).  The
; program runs through inject: a lock of inject's own is its current
; directory (cd), and inject clicks CD, then CANCEL (click=).  The path is
; a root (RAM:), so building the path takes no ParentDir locks (#158).
; Own 320x256 screen (see t93_freq).
; Correct: the current directory is the one the program was started with
; and its lock is not unlocked.  The original ends in RAM: and has
; unlocked the start lock (inject's report, asserted on `inject`).
DEFTYPE .l

!TestCase{cd_gadget}
  Screen 0,0,0,320,256,2,0,"freqlib",1,2
  Use Screen 0
  MaxLen p$=160
  MaxLen f$=64
  p$="RAM:"
  f$="x"
  r$=FileRequest$("FQ",p$,f$)
  !CheckEq{Len(r$),0,"cancel"}
!EndTestCase

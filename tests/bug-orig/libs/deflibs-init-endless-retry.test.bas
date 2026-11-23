; deflibs-init-endless-retry (#44) - the init of a deflibs library retries
; OpenLibrary forever if the library cannot be opened: a program that uses
; a call of a missing library hangs at startup.
; Here on Kickstart 1.3, which has no gadtools.library: the program uses
; CreateContext_ (gadtools) in a statement it never calls.  This also runs
; the finit of gadtools with base 0, where a CloseLibrary(0) would crash
; on Kickstart 1.x.
; Correct: the program runs and ends normally (the base is 0, the call is
; not made).  The original hangs at its start (the session ends by its
; time limit).
DEFTYPE .l

Statement neverrun{}
  x=CreateContext_(&x)
End Statement

!TestCase{runs}
  !Check{1,"started"}
!EndTestCase

; riregistrylib-unlock-below-stack (#781) - without registry.library (it
; exists nowhere) LockRegistry returns 0 without pushing anything, but
; UnlockRegistry always pops: each Lock/Unlock pair goes one entry below
; the structure stack, into what lies in front of it.  As soon as an
; entry there is not 0 it becomes the current structure, and the next
; UnlockRegistry calls registry.library through a base of 0 - a crash
; (the CPU halts).  By the library file (the end of its token table: 0,
; then $0000FFFF) that is the third pair; the recorded original crashes
; one pair earlier, so the program checks only once, after the loop.
; Correct: UnlockRegistry without a pushed lock does nothing; the
; program goes on.
DEFTYPE .l

!TestCase{three_pairs}
  o=IsRegistryOpen
  !Require{o=0,"registry_closed"}
  For i.w=1 To 3
    a.l=LockRegistry(1)
    UnlockRegistry
  Next
  !Check{1,"after_loop"}
!EndTestCase

; rimapshapeslib-init-allocvec-unchecked (#905) - init stores the result
; of AllocVec(512) for the group without a check; when it fails,
; ResetGroup clears the long at address 0 (and AddGroupMember would write
; its members from there on, over the ExecBase pointer).  The program runs
; through inject, which fails the first allocation of 512-528 bytes (the
; group block); the long at address 0 holds a marker (put back at the
; end).
; Correct: without a group block ResetGroup writes nothing (the marker
; stays).
DEFTYPE .l

!TestCase{address0_kept}
  o=Peek.l(0)
  Poke.l 0,$12345678
  g=ResetGroup(16,16)
  m=Peek.l(0)
  Poke.l 0,o
  !CheckEq{g,0,"group"}
  !CheckEq{m,$12345678,"marker"}
!EndTestCase

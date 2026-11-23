; hangs: a case that waits for a signal that never comes (the self-test of
; the bundles: in the middle of a session the hang check ends it after the
; environment's run-hang-after, the cells behind it run again; not stored)
!TestCase{one}
  !Check{-1,"before"}
  s.l=Wait_($1000)
  !Check{-1,"after"}
!EndTestCase

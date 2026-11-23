; ciatrklib-checkid-address0 (#430) - CheckTrackerID returns -1 for a
; module with the ID "M.K." or "M!K!" and otherwise MOVE.l 0,d0: the long
; at ADDRESS 0 instead of 0 (False).
; Here: the program puts $12345678 at address 0 during the calls (and
; restores it): a slot whose memory has no ID (2200 zero bytes given with
; DecodeModule) and an empty slot.
; Correct: both are False (0); a module with "M.K." is True (-1).
DEFTYPE .l

!TestCase{check_id}
  Dim m.b(2200)
  a=&m(0)
  DecodeModule 0,a
  z=Peek.l(0) : Poke.l 0,$12345678
  r0=CheckTrackerID(0)
  r3=CheckTrackerID(3)
  Poke.l 0,z
  !CheckEq{r0,0,"no_id"}
  !CheckEq{r3,0,"empty_slot"}
  Poke.l GetTrackerLocation(0)+$438,$4D2E4B2E
  !CheckEq{CheckTrackerID(0),-1,"mk_id"}
  FreeTrackerModule 0
!EndTestCase

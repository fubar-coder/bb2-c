; rizonejoylib-newzonetable-allocfail-stale-entry (#874) - NewZoneTable
; frees the old table of that number before it allocates the new one; when
; the allocation fails it returns 0 but leaves the freed block in the
; list entry.  The program runs through inject, which fails the first
; allocation of 164 bytes (table 1 with 20 zones); UseZoneTable 1 then
; selects the freed block and reports success (and finit frees it a
; second time).
; Correct: after the failed NewZoneTable table 1 does not exist,
; UseZoneTable 1 returns 0.
DEFTYPE .l

!TestCase{alloc_fails}
  r1=NewZoneTable(1,10)
  r2=NewZoneTable(1,20)
  u=UseZoneTable(1)
  !CheckEq{r1,-1,"first_table"}
  !CheckEq{r2,0,"second_fails"}
  !CheckEq{u,0,"table_gone"}
!EndTestCase

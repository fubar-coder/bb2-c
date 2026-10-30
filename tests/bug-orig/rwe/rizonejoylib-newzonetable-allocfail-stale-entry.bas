; rizonejoylib-newzonetable-allocfail-stale-entry (#874) - NewZoneTable
; frees the old table of that number before it allocates the new one; when
; the allocation fails it returns 0 but leaves the freed block in the
; list entry.  The program runs through inject, which fails the first
; allocation of 164 bytes (table 1 with 20 zones); UseZoneTable 1 then
; selects the freed block and reports success (and finit frees it a
; second time).
; Correct: after the failed NewZoneTable table 1 does not exist,
; UseZoneTable 1 returns 0.
; @rwe rizonejoylib
; @pass runwrap=@INJECT@|allocfail=164-164/1
r=NewZoneTable(1,10)
NPrint "new 10 zones ",r
r=NewZoneTable(1,20)
NPrint "new 20 zones ",r
NPrint "use ",UseZoneTable(1)
End

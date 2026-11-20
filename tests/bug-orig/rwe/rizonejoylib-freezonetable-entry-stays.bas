; rizonejoylib-freezonetable-entry-stays (#868) - FreeZoneTable frees the
; table but leaves the freed block in its list entry: UseZoneTable of the
; number selects the freed block and reports success, and finit frees it
; a second time at the end of the program.  Made safe: when UseZoneTable
; succeeds, the program takes the freed block back (AllocAbs, under
; Forbid, before anything else can allocate) and writes its zone count
; again (FreeMem put the memory list's header there), so that finit frees
; it once and with the right size.
; Correct: the entry is cleared, UseZoneTable 1 returns 0.
; @fixed #868
; @rwe rizonejoylib
DEFTYPE .l
r=NewZoneTable(1,10)
Forbid_
r=FreeZoneTable(1)
u=UseZoneTable(1)
If u
  a=ZoneTable
  UseZoneTable 0
  b=AllocAbs_(84,a)
  If b Then Poke.l a,10
EndIf
Permit_
NPrint u
End

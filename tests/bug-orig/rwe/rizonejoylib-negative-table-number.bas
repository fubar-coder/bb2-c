; rizonejoylib-negative-table-number (#872) - NewZoneTable, UseZoneTable
; and FreeZoneTable check the table number only against 16 (signed): a
; negative number passes, entry -1 of the list is _zonetable, the pointer
; to the current table in front of it.  UseZoneTable -1 finds the current
; table there, "selects" it and reports success.  (FreeZoneTable -1 and
; NewZoneTable -1 would free the current table and leave it in its own
; entry: a double free at the end, not done here.)
; Correct: a negative table number is refused, the result is 0.
; @rwe rizonejoylib
NPrint UseZoneTable(-1)
End

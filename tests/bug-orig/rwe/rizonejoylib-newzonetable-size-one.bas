; rizonejoylib-newzonetable-size-one (#1136) - NewZoneTable refuses a table
; of one zone (it takes 2 to 65535 zones); the manual says "The new
; zonetables can be from 1 to 65536 in size".
; Correct: table 1 with one zone is made (-1); zone 0 can be set and
; found, the table has 1 zone.
; @fixed #1136
; @rwe rizonejoylib
r=NewZoneTable(1,1)
NPrint r
If r
  UseZoneTable 1
  NPrint SetZone(0,0,0,10,10)," ",Zone(5,5)," ",ZoneTableSize
EndIf
End

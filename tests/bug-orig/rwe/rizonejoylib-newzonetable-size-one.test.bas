; rizonejoylib-newzonetable-size-one (#1136) - NewZoneTable refuses a table
; of one zone (it takes 2 to 65535 zones); the manual says "The new
; zonetables can be from 1 to 65536 in size".
; Correct: table 1 with one zone is made (-1); zone 0 can be set and
; found, the table has 1 zone.
DEFTYPE .l

!TestCase{one_zone}
  r=NewZoneTable(1,1)
  !CheckEq{r,-1,"made"}
  If r
    UseZoneTable 1
    s=SetZone(0,0,0,10,10)
    z=Zone(5,5)
    n=ZoneTableSize
    !CheckEq{s,-1,"set"}
    !CheckEq{z,0,"found"}
    !CheckEq{n,1,"size"}
  EndIf
!EndTestCase

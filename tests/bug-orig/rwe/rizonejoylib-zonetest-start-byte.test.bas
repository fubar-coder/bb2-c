; rizonejoylib-zonetest-start-byte (#881) - ZoneTest finds its start zone
; through the low byte of the zone number, while it counts the number as
; a word: from zone 256 on it tests the wrong zones under the right
; numbers.  ZoneTest(260,5,5) tests zone 4 (a rectangle around the point)
; and returns 260; zone 260 is not set.
; Correct: zone 260 is tested, the result is -1.
DEFTYPE .l

!TestCase{start_zone_260}
  r=NewZoneTable(1,300)
  UseZoneTable 1
  ZoneInit
  SetZone 4,0,0,10,10
  z=ZoneTest(260,5,5)
  !CheckEq{z,-1,"not_found"}
!EndTestCase

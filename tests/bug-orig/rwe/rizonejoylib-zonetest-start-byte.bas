; rizonejoylib-zonetest-start-byte (#881) - ZoneTest finds its start zone
; through the low byte of the zone number, while it counts the number as
; a word: from zone 256 on it tests the wrong zones under the right
; numbers.  ZoneTest(260,5,5) tests zone 4 (a rectangle around the point)
; and returns 260; zone 260 is not set.
; Correct: zone 260 is tested, the result is -1.
; @fixed #881
; @rwe rizonejoylib
r=NewZoneTable(1,300)
UseZoneTable 1
ZoneInit
SetZone 4,0,0,10,10
NPrint ZoneTest(260,5,5)
End

; rizonejoylib-zonetest020-circle-word (#880) - the 68020 mode (on by
; default on a 68020) compares the squared distance of a point from a
; circle's centre with the radius squared as words: the point (256,3) has
; the squared distance 65545 from (0,0), whose low word 9 is below 100,
; so it lies in the circle of radius 10.
; Correct: the point lies in no zone (the 68000 mode compares longs).
; @rwe rizonejoylib
SetZoneMode -1
ZoneInit
SetZone 1,0,0,10
NPrint Zone(256,3)
End

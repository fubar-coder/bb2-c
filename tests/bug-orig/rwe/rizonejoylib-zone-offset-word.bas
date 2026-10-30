; rizonejoylib-zone-offset-word (#878) - NewZoneTable allows 65535 zones,
; but SetZone and ZoneInit compute a zone's offset with ASL.w #3: from
; zone 8192 on it wraps, SetZone 8192 writes zone 0.  Table 1 has 10000
; zones, all cleared with ZoneInit; zone 8192 is set to a rectangle.
; Correct: the point 15,15 lies in zone 8192 (the bug finds it in zone
; 0).
; @rwe rizonejoylib
r=NewZoneTable(1,10000)
UseZoneTable 1
ZoneInit
SetZone 8192,10,10,20,20
NPrint Zone(15,15)
End

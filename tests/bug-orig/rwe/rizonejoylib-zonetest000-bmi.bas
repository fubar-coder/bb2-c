; rizonejoylib-zonetest000-bmi (#879) - the 68000 mode tests x <= x2 and
; y <= y2 by the sign of the word difference x2 - x, y2 - y (BMI): wrong
; when the difference is beyond 32767.  Zone 1 reaches from y -32768 to
; 32767; for the point (5,-1) y2 - y is 32768, negative as a word, and
; the point is not found.
; Correct: the point lies in zone 1.
; @fixed #879
; @rwe rizonejoylib
SetZoneMode 0
ZoneInit
SetZone 1,0,-32768,10,32767
NPrint Zone(5,-1)
End

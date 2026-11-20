; rizonejoylib-circle-marker-y2 (#866) - a circle is a zone with y2 = -1;
; the 68020 mode (on by default on a 68020) takes every zone with a
; negative y2 for a circle, and SetZone with four coordinates stores a
; rectangle whose bottom edge is -1 as it is.  Zone 7, a rectangle above
; the top of the screen (y -200 to -100), is tested as a circle around
; (0,-200) with the "radius squared" 10: the point (5,-150) inside the
; rectangle is not found.
; Correct: only y2 = -1 marks a circle, in both modes: Zone(5,-150) is
; zone 7; a rectangle whose bottom edge is exactly -1 cannot be told
; from a circle and is refused (SetZone returns 0, nothing is stored).
; @fixed #866
; @rwe rizonejoylib
SetZoneMode -1
ZoneInit
SetZone 7,0,-200,10,-100
NPrint Zone(5,-150)
NPrint SetZone(5,-100,-50,50,-1)
End

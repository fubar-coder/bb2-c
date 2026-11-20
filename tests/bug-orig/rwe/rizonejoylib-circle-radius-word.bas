; rizonejoylib-circle-radius-word (#867) - SetZone num,x,y,radius stores
; the radius squared in a word: radius 256 and more wraps (256 gives 0,
; 300 gives 24464).  The circle of radius 256 around (1000,1000) becomes
; a circle of radius 0: SetZone reports it done, and only its centre
; lies in it.
; Correct: a radius outside 0-255 is refused (SetZone returns 0, nothing
; is stored), so the centre lies in no zone.
; @fixed #867
; @rwe rizonejoylib
ZoneInit
NPrint SetZone(1,1000,1000,256)
NPrint Zone(1000,1000)
End

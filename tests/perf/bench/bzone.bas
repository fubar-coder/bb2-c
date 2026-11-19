; bzone - perf benchmark: the RWE userlib rizonejoylib.  Zone(x,y) (a
; game's per-frame test of a point, the mouse or a sprite, against the
; whole zone table) and ZoneTest over a range and of one zone.  Table 0
; (256 zones) holds 12 rectangles and 4 circles in the zones 0-15, the
; other 240 are not set.  The value per call.  The machine is a 68000:
; the library's 68000 mode (the 68020 mode needs a 68020).
; @rwe rizonejoylib
DEFTYPE .l
Dim t(9)
ZoneInit
For i=0 To 11
  SetZone i,i*20,i*10,i*20+15,i*10+30
Next
SetZone 12,250,50,20
SetZone 13,250,120,30
SetZone 14,40,200,10
SetZone 15,150,220,25
VWait 5
BLITZ
Poke.b $bfde00,0 : Poke.b $bfdf00,0
Poke.b $bfd400,99 : Poke.b $bfd500,0
Poke.b $bfd600,$ff : Poke.b $bfd700,$ff
Poke.b $bfdf00,$51 : Poke.b $bfde00,$11
VWait 2
Gosub tod : t0=tt
For i=1 To 20000
Next
Gosub tod : t(0)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 150000
Next
Gosub tod : t(1)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 1000
r=Zone(300,10)
Next
Gosub tod : t(2)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
r=Zone(5,5)
Next
Gosub tod : t(3)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
r=Zone(245,125)
Next
Gosub tod : t(4)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
r=ZoneTest(0,15,300,10)
Next
Gosub tod : t(5)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
r=ZoneTest(3,65,35)
Next
Gosub tod : t(6)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
r=ZoneTest(3,300,10)
Next
Gosub tod : t(7)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
r=ZoneTest(13,245,125)
Next
Gosub tod : t(8)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
r=ZoneTest(13,300,10)
Next
Gosub tod : t(9)=(tt-t0)&65535
VWait 2
AMIGA
NPrint "empty ",t(0)," 20000 1"
NPrint "emptylong ",t(1)," 150000 1"
NPrint "zonemiss ",t(2)," 1000 1"
NPrint "zonerect ",t(3)," 2000 1"
NPrint "zonecircle ",t(4)," 2000 1"
NPrint "rangemiss ",t(5)," 2000 1"
NPrint "rect1hit ",t(6)," 2000 1"
NPrint "rect1miss ",t(7)," 2000 1"
NPrint "circle1hit ",t(8)," 2000 1"
NPrint "circle1miss ",t(9)," 2000 1"
End
tod:
th=Peek.b($bfd700)&255
tl=Peek.b($bfd600)&255
t2=Peek.b($bfd700)&255
If t2<>th Then tl=Peek.b($bfd600)&255 : th=t2
tt=65535-((th LSL 8)+tl)
Return

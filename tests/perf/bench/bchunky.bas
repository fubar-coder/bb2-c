; bchunky - perf benchmark: the RWE userlib richunkylib.  RaycastSky (a
; ray caster's per-column call: a texture column into a chunky column,
; both 320 bytes per row) with the heights 1, 10, 100 and 200, and
; ChunkyToPlanar of a 320 x 256 chunky bitmap into a 320 x 256 x 8 bitmap
; (CludgeBitMap: 8 contiguous planes of 10240 bytes), the value per call.
; @rwe richunkylib
DEFTYPE .l
Dim t(6)
ChunkyBitmap 0,320,256
c=ChunkyData(0)
tx=AllocMem_(81920,$10001)
m=AllocMem_(81920,$10002)
If c=0 OR tx=0 OR m=0 Then NPrint "no memory" : End
For i=0 To 81916 Step 4
  Poke.l c+i,i*$1010101 : Poke.l tx+i,i*$3070103
Next
CludgeBitMap 0,320,256,8,m
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
For i=1 To 2000
RaycastSky 0,100,1,37,tx
Next
Gosub tod : t(2)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
RaycastSky 0,100,10,37,tx
Next
Gosub tod : t(3)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
RaycastSky 0,100,100,37,tx
Next
Gosub tod : t(4)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
RaycastSky 0,100,200,37,tx
Next
Gosub tod : t(5)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 4
ChunkyToPlanar 0,0
Next
Gosub tod : t(6)=(tt-t0)&65535
VWait 2
AMIGA
NPrint "empty ",t(0)," 20000 1"
NPrint "emptylong ",t(1)," 150000 1"
NPrint "raycast1 ",t(2)," 2000 1"
NPrint "raycast10 ",t(3)," 2000 1"
NPrint "raycast100 ",t(4)," 2000 1"
NPrint "raycast200 ",t(5)," 2000 1"
NPrint "c2p320 ",t(6)," 4 1"
End
tod:
th=Peek.b($bfd700)&255
tl=Peek.b($bfd600)&255
t2=Peek.b($bfd700)&255
If t2<>th Then tl=Peek.b($bfd600)&255 : th=t2
tt=65535-((th LSL 8)+tl)
Return

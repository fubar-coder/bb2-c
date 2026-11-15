; bdisp - perf benchmark: DisplayBitMap, DisplaySprite, DisplayPalette (displaylib)
DEFTYPE .l
Dim t(8)
a=123456 : c=0
BitMap 0,320,256,3
BitMap 1,64,64,3
Use BitMap 1
Boxf 0,0,15,15,5 : Boxf 4,4,11,11,2
GetaShape 0,0,0,16,16
Boxf 0,0,31,31,3
GetaShape 1,0,0,32,32
BitMap 2,32,32,2
Use BitMap 2
Boxf 0,0,15,15,3
GetaShape 2,0,0,16,16
GetaSprite 0,2
Buffer 0,16384
Queue 0,64
Dim pts.w(8)
pts(0)=10:pts(1)=10:pts(2)=60:pts(3)=20:pts(4)=30:pts(5)=50

For i=0 To 7 : PalRGB 0,i,i,i,i : Next
InitCopList 0,44,256,$3,8,8,0
VWait 5
BLITZ
Poke.b $bfde00,0 : Poke.b $bfdf00,0
Poke.b $bfd400,99 : Poke.b $bfd500,0
Poke.b $bfd600,$ff : Poke.b $bfd700,$ff
Poke.b $bfdf00,$51 : Poke.b $bfde00,$11
CreateDisplay 0
DisplayBitMap 0,0
DisplayPalette 0,0
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
DisplayBitMap 0,0
Next
Gosub tod : t(2)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
DisplayBitMap 0,0,8,8
Next
Gosub tod : t(3)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
DisplaySprite 0,0,100,100,0
Next
Gosub tod : t(4)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 500
DisplayPalette 0,0
Next
Gosub tod : t(5)=(tt-t0)&65535
VWait 2
AMIGA
NPrint "empty ",t(0)," 20000 1"
NPrint "emptylong ",t(1)," 150000 1"
NPrint "dispbitmap ",t(2)," 2000 1"
NPrint "dispbitmapxy ",t(3)," 2000 1"
NPrint "dispsprite ",t(4)," 2000 1"
NPrint "disppalette ",t(5)," 500 1"
End
tod:
th=Peek.b($bfd700)&255
tl=Peek.b($bfd600)&255
t2=Peek.b($bfd700)&255
If t2<>th Then tl=Peek.b($bfd600)&255 : th=t2
tt=65535-((th LSL 8)+tl)
Return


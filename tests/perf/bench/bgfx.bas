; bgfx - perf benchmark: blits, collisions, drawing, Print on a bitmap, Show, RGB
DEFTYPE .l
Dim t(47)
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
BitMap 3,32,32,1
Use BitMap 3
Boxf 0,0,15,15,1 : Boxf 4,4,11,11,0
GetaShape 3,0,0,16,16
BitMap 4,32,32,6
Use BitMap 4
Boxf 0,0,15,15,45 : Boxf 4,4,11,11,18
GetaShape 4,0,0,16,16
BitMap 5,320,64,6
Buffer 0,16384
Queue 0,64
Dim pts.w(8)
pts(0)=10:pts(1)=10:pts(2)=60:pts(3)=20:pts(4)=30:pts(5)=50
VWait 5
BLITZ
Poke.b $bfde00,0 : Poke.b $bfdf00,0
Poke.b $bfd400,99 : Poke.b $bfd500,0
Poke.b $bfd600,$ff : Poke.b $bfd700,$ff
Poke.b $bfdf00,$51 : Poke.b $bfde00,$11
Slice 0,44,320,256,$fff8,3,8,8,320,320
Use BitMap 0
Show 0
SetColl 1,3
BitMapOutput 0
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
Blit 0,100,100
Next
Gosub tod : t(2)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
Blit 0,105,100
Next
Gosub tod : t(3)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
Blit 1,100,100
Next
Gosub tod : t(4)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
Block 0,96,96
Next
Gosub tod : t(5)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
ClipBlit 0,100,100
Next
Gosub tod : t(6)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
c=BlitColl(0,100,100)
Next
Gosub tod : t(7)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 500
BBlit 0,0,20,20 : BBlit 0,0,60,20 : BBlit 0,0,100,20 : BBlit 0,0,140,20
UnBuffer 0
Next
Gosub tod : t(8)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 500
BBlit 0,0,20,20 : BBlit 0,0,60,20 : BBlit 0,0,100,20 : BBlit 0,0,140,20
BBlit 0,0,20,60 : BBlit 0,0,60,60 : BBlit 0,0,100,60 : BBlit 0,0,140,60
UnBuffer 0
Next
Gosub tod : t(9)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 500
QBlit 0,0,20,20 : QBlit 0,0,60,20 : QBlit 0,0,100,20 : QBlit 0,0,140,20
UnQueue 0
Next
Gosub tod : t(10)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 500
QBlit 0,0,20,20 : QBlit 0,0,60,20 : QBlit 0,0,100,20 : QBlit 0,0,140,20
QBlit 0,0,20,60 : QBlit 0,0,60,60 : QBlit 0,0,100,60 : QBlit 0,0,140,60
UnQueue 0
Next
Gosub tod : t(11)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 500
QBlit 0,0,20,20 : QBlit 0,0,60,20 : QBlit 0,0,100,20 : QBlit 0,0,140,20
FlushQueue 0
Next
Gosub tod : t(12)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 500
QBlit 0,0,20,20 : QBlit 0,0,60,20 : QBlit 0,0,100,20 : QBlit 0,0,140,20
QBlit 0,0,20,60 : QBlit 0,0,60,60 : QBlit 0,0,100,60 : QBlit 0,0,140,60
FlushQueue 0
Next
Gosub tod : t(13)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
ShowSprite 0,100,100,0
Next
Gosub tod : t(14)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
DoColl
Next
Gosub tod : t(15)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
c=SColl(0,2)
Next
Gosub tod : t(16)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
c=PColl(0)
Next
Gosub tod : t(17)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
c=ShapesHit(0,10,10,1,20,20)
Next
Gosub tod : t(18)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
c=SpritesHit(0,10,10,0,20,20)
Next
Gosub tod : t(19)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
c=RectsHit(10,10,16,16,20,20,16,16)
Next
Gosub tod : t(20)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
Plot 10,10,1
Next
Gosub tod : t(21)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
c=Point(10,10)
Next
Gosub tod : t(22)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 500
Line 10,10,110,60,1
Next
Gosub tod : t(23)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 500
Line 10,10,110,10,1
Next
Gosub tod : t(24)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 500
Box 10,10,50,50,1
Next
Gosub tod : t(25)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 500
Boxf 10,10,25,25,2
Next
Gosub tod : t(26)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 200
Circle 100,100,10,1
Next
Gosub tod : t(27)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 200
Circlef 100,100,10,1
Next
Gosub tod : t(28)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 50
Cls
Next
Gosub tod : t(29)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 200
Poly 3,&pts(0),1
Next
Gosub tod : t(30)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 200
Polyf 3,&pts(0),1
Next
Gosub tod : t(31)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 500
Scroll 0,0,32,32,100,100
Next
Gosub tod : t(32)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
Locate 0,0
Next
Gosub tod : t(33)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
Colour 1
Next
Gosub tod : t(34)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 500
Locate 0,0 : Print "SCORE"
Next
Gosub tod : t(35)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 500
Locate 0,0 : Print a
Next
Gosub tod : t(36)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
Show 0
Next
Gosub tod : t(37)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
Show 0,8,8
Next
Gosub tod : t(38)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
RGB 1,15,0,0
Next
Gosub tod : t(39)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
c=Green(1)
Next
Gosub tod : t(40)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
Use BitMap 0
Next
Gosub tod : t(41)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
Use Slice 0
Next
Gosub tod : t(42)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
Blit 3,100,100
Next
Gosub tod : t(43)=(tt-t0)&65535
Use BitMap 5
Gosub tod : t0=tt
For i=1 To 2000
Blit 4,100,20
Next
Gosub tod : t(44)=(tt-t0)&65535
Use BitMap 0
Gosub tod : t0=tt
For i=1 To 1000
ClipBlit 0,-5,100 : ClipBlit 0,310,100 : ClipBlit 0,100,-5 : ClipBlit 0,100,250
Next
Gosub tod : t(45)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
Blit 3,100,100,1
Next
Gosub tod : t(46)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
ClipBlit 3,100,100,1
Next
Gosub tod : t(47)=(tt-t0)&65535
VWait 2
AMIGA
DefaultOutput
NPrint "empty ",t(0)," 20000 1"
NPrint "emptylong ",t(1)," 150000 1"
NPrint "blit16 ",t(2)," 2000 1"
NPrint "blit16s ",t(3)," 2000 1"
NPrint "blit32 ",t(4)," 2000 1"
NPrint "block16 ",t(5)," 2000 1"
NPrint "clipblit16 ",t(6)," 2000 1"
NPrint "blitcoll ",t(7)," 2000 1"
NPrint "bblit4u ",t(8)," 500 1"
NPrint "bblit8u ",t(9)," 500 1"
NPrint "qblit4u ",t(10)," 500 1"
NPrint "qblit8u ",t(11)," 500 1"
NPrint "qblit4f ",t(12)," 500 1"
NPrint "qblit8f ",t(13)," 500 1"
NPrint "showsprite ",t(14)," 2000 1"
NPrint "docoll ",t(15)," 2000 1"
NPrint "scoll ",t(16)," 2000 1"
NPrint "pcoll ",t(17)," 2000 1"
NPrint "shapeshit ",t(18)," 2000 1"
NPrint "spriteshit ",t(19)," 2000 1"
NPrint "rectshit ",t(20)," 2000 1"
NPrint "plot ",t(21)," 2000 1"
NPrint "point ",t(22)," 2000 1"
NPrint "line ",t(23)," 500 1"
NPrint "lineh ",t(24)," 500 1"
NPrint "box ",t(25)," 500 1"
NPrint "boxf ",t(26)," 500 1"
NPrint "circle ",t(27)," 200 1"
NPrint "circlef ",t(28)," 200 1"
NPrint "cls ",t(29)," 50 1"
NPrint "poly3 ",t(30)," 200 1"
NPrint "polyf3 ",t(31)," 200 1"
NPrint "scroll32 ",t(32)," 500 1"
NPrint "locate ",t(33)," 2000 1"
NPrint "colour ",t(34)," 2000 1"
NPrint "print5 ",t(35)," 500 1"
NPrint "printnum ",t(36)," 500 1"
NPrint "show ",t(37)," 2000 1"
NPrint "showxy ",t(38)," 2000 1"
NPrint "rgb ",t(39)," 2000 1"
NPrint "green ",t(40)," 2000 1"
NPrint "usebitmap ",t(41)," 2000 1"
NPrint "useslice ",t(42)," 2000 1"
NPrint "blit16p1 ",t(43)," 2000 1"
NPrint "blit16p6 ",t(44)," 2000 1"
NPrint "clipblitc ",t(45)," 1000 1"
NPrint "blitex ",t(46)," 2000 1"
NPrint "clipblitex ",t(47)," 2000 1"
End
tod:
th=Peek.b($bfd700)&255
tl=Peek.b($bfd600)&255
t2=Peek.b($bfd700)&255
If t2<>th Then tl=Peek.b($bfd600)&255 : th=t2
tt=65535-((th LSL 8)+tl)
Return


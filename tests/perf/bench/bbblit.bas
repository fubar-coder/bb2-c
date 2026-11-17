; bbblit - perf benchmark: every command of the RWE userlib bb_blitlib
; @rwe bb_blitlib
DEFTYPE .l
Dim t(20)
a=123456 : c=0
BitMap 0,320,256,3
BitMap 1,64,64,3
BitMap 2,320,256,3
BitMap 3,320,64,6
Use BitMap 1
Boxf 0,0,15,15,5 : Boxf 4,4,11,11,2
GetaShape 0,0,0,16,16
Boxf 16,0,31,15,3 : Boxf 20,4,27,11,6
GetaShape 1,16,0,16,16
GetaShape 12,32,0,16,16
img=Peek.l(Addr BitMap(1)+8)
mem=Peek.l(Addr BitMap(1)+12)
Buffer 0,16384
Buffer 1,16384
VWait 5
BLITZ
Poke.b $bfde00,0 : Poke.b $bfdf00,0
Poke.b $bfd400,99 : Poke.b $bfd500,0
Poke.b $bfd600,$ff : Poke.b $bfd700,$ff
Poke.b $bfdf00,$51 : Poke.b $bfde00,$11
Slice 0,44,320,256,$fff8,3,8,8,320,320
Use BitMap 0
Show 0
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
SpecialBlock 0,1,96,96
Next
Gosub tod : t(2)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
SpecialBlock 0,1,3852
Next
Gosub tod : t(3)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
TBlock 0,96,96
Next
Gosub tod : t(4)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 500
BPredatorBlit 0,0,20,20 : BPredatorBlit 0,0,60,20 : BPredatorBlit 0,0,100,20 : BPredatorBlit 0,0,140,20
UnBuffer 0
Next
Gosub tod : t(5)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
SpecialBlockBitmap 0,2,96,96
Next
Gosub tod : t(6)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
SpecialBlockBitmap 0,2,3852
Next
Gosub tod : t(7)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
c=PredatorBlit(0,100,96)
Next
Gosub tod : t(8)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 500
BColourBlit 0,0,20,20,5 : BColourBlit 0,0,60,20,2
BColourBlit 0,0,100,20,7 : BColourBlit 0,0,140,20,1
UnBuffer 0
Next
Gosub tod : t(9)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
SetupMapTwo 40
Next
Gosub tod : t(10)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
CreateShape 20,10,mem,0
Next
Gosub tod : t(11)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
SpecialBlockToShape 0,1,12
Next
Gosub tod : t(12)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
BlockToShape 0,12
Next
Gosub tod : t(13)=(tt-t0)&65535
Use BitMap 3
Gosub tod : t0=tt
For i=1 To 2000
TBlockImage img,0,96,32
Next
Gosub tod : t(14)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 500
BTBlockImage 1,img,20,20,0 : BTBlockImage 1,img,60,20,0
BTBlockImage 1,img,100,20,0 : BTBlockImage 1,img,140,20,0
UnBuffer 1
Next
Gosub tod : t(15)=(tt-t0)&65535
Use BitMap 0
VWait 2
AMIGA
NPrint "empty ",t(0)," 20000 1"
NPrint "emptylong ",t(1)," 150000 1"
NPrint "specialblock ",t(2)," 2000 1"
NPrint "specialblocko ",t(3)," 2000 1"
NPrint "tblock ",t(4)," 2000 1"
NPrint "bpredator4u ",t(5)," 500 1"
NPrint "sblockbm ",t(6)," 2000 1"
NPrint "sblockbmo ",t(7)," 2000 1"
NPrint "predator ",t(8)," 2000 1"
NPrint "bcolour4u ",t(9)," 500 1"
NPrint "setupmaptwo ",t(10)," 2000 1"
NPrint "createshape10 ",t(11)," 2000 1"
NPrint "sblocktoshape ",t(12)," 2000 1"
NPrint "blocktoshape ",t(13)," 2000 1"
NPrint "tblockimage ",t(14)," 2000 1"
NPrint "btblockimage4u ",t(15)," 500 1"
End
tod:
th=Peek.b($bfd700)&255
tl=Peek.b($bfd600)&255
t2=Peek.b($bfd700)&255
If t2<>th Then tl=Peek.b($bfd600)&255 : th=t2
tt=65535-((th LSL 8)+tl)
Return

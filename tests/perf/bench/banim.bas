; banim - perf benchmark: RINextAnimFrame of the RWE userlib rianimlib
; (the game path: one delta per call).  anim5: the CD's INTRO.ANIM (SHED,
; 320 x 256 x 3, ANIM5, 13 deltas of 1158 to 2930 bytes), the frames 3 to
; 14 a hundred times (AnimLoop on: behind frame 14 the third again).
; anim7: an ANIM7 built here (320 x 256 x 5, long data, six deltas, every
; plane and column skips 8 to 127 rows, copies 16 longs and runs one long
; over 32 rows: 2400 longs per frame), its frames 3 to 7 sixty times.
; RIPackLib calls RIAmosFuncLib.
; @rwe rianimlib ripacklib riamosfunclib
; @file data/intro.anim|@CD@/BUI9/SHED/IFF/INTRO.ANIM
DEFTYPE .l
Dim t(4)
c=0
f=AllocMem_(43566,1)
a=AllocMem_(8192,$10001)
If f=0 OR a=0 Then NPrint "no memory" : End
If ReadFile(0,"CORPUS:data/intro.anim")
  ReadMem 0,f,43566
  CloseFile 0
EndIf
BitMap 0,320,256,3
BitMap 1,320,256,5
; the ANIM7: FORM ANIM { FORM ILBM { BMHD, BODY (ByteRun1, zeros) },
; six FORM ILBM { ANHD, DLTA } }; the planes of a delta share its op list
; and its data list
Poke.l a,$464f524d : Poke.l a+8,$414e494d
Poke.l a+12,$464f524d : Poke.l a+16,2600 : Poke.l a+20,$494c424d
Poke.l a+24,$424d4844 : Poke.l a+28,20
Poke.w a+32,320 : Poke.w a+34,256
Poke.b a+40,5 : Poke.b a+42,1 : Poke.b a+46,10 : Poke.b a+47,11
Poke.w a+48,320 : Poke.w a+50,256
Poke.l a+52,$424f4459 : Poke.l a+56,2560
For i=0 To 1279
  Poke.w a+60+i*2,$d900
Next
q=a+2620
For k=0 To 5
  Poke.l q,$464f524d : Poke.l q+4,856 : Poke.l q+8,$494c424d
  Poke.l q+12,$414e4844 : Poke.l q+16,40
  Poke.b q+20,7 : Poke.w q+22,320 : Poke.w q+24,256 : Poke.l q+34,1 : Poke.l q+40,1
  Poke.l q+60,$444c5441 : Poke.l q+64,796
  d=q+68
  For p=0 To 4
    Poke.l d+p*4,64 : Poke.l d+32+p*4,116
  Next
  For j=0 To 9
    o=d+64+j*5
    Poke.b o,3 : Poke.b o+1,8+((j*8+k*16) MOD 120)
    Poke.b o+2,$90 : Poke.b o+3,0 : Poke.b o+4,32
  Next
  For j=0 To 169
    Poke.l d+116+j*4,j*$10001+k
  Next
  q=q+864
Next
Poke.l a+4,q-a-8
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
r=RIAnimInit(f,0,0)
c=RINextAnimFrame(0)
Gosub tod : t0=tt
For i=1 To 1200
c=RINextAnimFrame(0)
Next
Gosub tod : t(2)=(tt-t0)&65535
r=RIAnimInit(a,1,0)
c=RINextAnimFrame(1)
Gosub tod : t0=tt
For i=1 To 300
c=RINextAnimFrame(1)
Next
Gosub tod : t(3)=(tt-t0)&65535
VWait 2
AMIGA
NPrint "empty ",t(0)," 20000 1"
NPrint "emptylong ",t(1)," 150000 1"
NPrint "anim5 ",t(2)," 1200 1"
NPrint "anim7 ",t(3)," 300 1"
End
tod:
th=Peek.b($bfd700)&255
tl=Peek.b($bfd600)&255
t2=Peek.b($bfd700)&255
If t2<>th Then tl=Peek.b($bfd600)&255 : th=t2
tt=65535-((th LSL 8)+tl)
Return

; bsnow - perf benchmark: the RWE userlib risnowlib.  LDoSnow, the call
; per frame of a snow field (one step of every flake plus a new one), on
; two fields of 500 points in a 320 x 256 x 3 bitmap each, the value per
; call: snowfall (an empty bitmap: a flake in every row, about 255,
; falling straight down) and snowpile (a ground of plane 0 and plane 1
; in row 200 and obstacles of both planes above it: flakes move aside
; and stop, the piles grow).  The random table (offsets 0-39 of row 0,
; bits 0-7) from a fixed LCG; 300 steps before the loops fill the
; fields.  Each loop runs 200 calls: the timer's 16 bits hold 65.5
; million cycles, 1000 calls of the C port (163 million) wrapped it.
; @rwe risnowlib
DEFTYPE .l
Dim t(3)
BitMap 0,320,256,3
BitMap 1,320,256,3
rt=AllocMem_(1030,$10001)
If rt=0 Then NPrint "no memory" : End
s=12345
For i=2 To 1025 Step 2
  s=(s*1103+12345)&$FFFF
  Poke.b rt+i,s MOD 40 : Poke.b rt+i+1,(s/40)&7
Next
Use BitMap 1
Line 0,200,319,200,3
For i=0 To 30
  Plot 10*i+5,150,3 : Plot 10*i+6,120,3
Next
l0=InitSnowDrop(0,500,0,10240)
l1=InitSnowDrop(1,500,1,10240)
For i=1 To 300 : LDoSnow 0,rt : LDoSnow 1,rt : Next
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
For i=1 To 200
LDoSnow 0,rt
Next
Gosub tod : t(2)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 200
LDoSnow 1,rt
Next
Gosub tod : t(3)=(tt-t0)&65535
VWait 2
AMIGA
NPrint "empty ",t(0)," 20000 1"
NPrint "emptylong ",t(1)," 150000 1"
NPrint "snowfall ",t(2)," 200 1"
NPrint "snowpile ",t(3)," 200 1"
End
tod:
th=Peek.b($bfd700)&255
tl=Peek.b($bfd600)&255
t2=Peek.b($bfd700)&255
If t2<>th Then tl=Peek.b($bfd600)&255 : th=t2
tt=65535-((th LSL 8)+tl)
Return

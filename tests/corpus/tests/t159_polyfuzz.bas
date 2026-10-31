; t159_polyfuzz - polylib with pseudo random arguments (fixed seed):
; BitMapPoly (plain and with a dither bitmap, both point orders), RenderPoly
; after RenderToBitmap (both forms) and RenderToChunky (colours 0-14: #422),
; ChunkyPoly (blitter and CPU form), ChunkyPlot and ChunkyVlin (the CLIP
; forms, the plain forms inside the area and with y1 <= y2: #417) on a
; copper chunky area as in t157.  At most 8 points (no overflow, #395);
; points around the bitmap / chunky area, so that all clip paths are
; taken.  The drawing happens in short BLITZ ... AMIGA sections (polylib
; does not own the blitter, RenderPoly and ChunkyPoly are Blitz mode only;
; as t156); checksums in Amiga mode.
DEFTYPE .l
Function.l rn{lo.l,hi.l}
  SHARED seed.l
  seed=(seed*69069+1) AND $7FFFFFFF
  Function Return lo+((seed/256) MOD (hi-lo+1))
End Function
Statement bsum{t$,bn.l}
  b.l=Addr BitMap(bn)
  Print t$
  For i.l=0 To Peek.b(b+5)-1
    pl.l=Peek.l(b+8+i*4) : s.l=0
    For k.l=0 To Peek.w(b)*Peek.w(b+2)-1 Step 2 : s=(s*5+Peek.w(pl+k))&$FFFFFF : Next
    Print " ",Hex$(s)
  Next
  NPrint ""
End Statement
Statement csum{t$,c.l}
  ch.l=Peek.l(c+96) : n.l=Peek.w(c+104)*Peek.w(c+108) : s.l=0
  For k.l=0 To n-1 Step 2 : s=(s*5+Peek.w(ch+k))&$FFFFFF : Next
  NPrint t$," ",Hex$(s)
End Statement
; x and y of the points all different: where a side leaves the clip width,
; the cut divides by the x difference to the point before (DIVS by 0 if
; both are beyond the edge at the same x - the same in the original; the
; chunky form swaps x and y)
Function.l uniq{a.l,i.l,v.l}
  Repeat
    ok.l=-1
    For j.l=0 To i-1
      If Peek.w(a+j*4)=v Then ok=0 : v=v+1
    Next
  Until ok
  Function Return v
End Function
Statement points{a.l,n.l,x0.l,x1.l,y0.l,y1.l}
  For i.l=0 To n-1
    Poke.w a+i*4,uniq{a,i,rn{x0,x1}}
    Poke.w a+i*4+2,uniq{a+2,i,rn{y0,y1}}
  Next
End Statement
seed=4711
Dim p.w(40)
a=&p(0)
BitMap 0,160,64,3
BitMap 1,160,64,3
BitMap 2,160,32,3
b=Addr BitMap(2)
For i=0 To 2
  pl=Peek.l(b+8+i*4)
  For k=0 To 20*32-1 : Poke.b pl+k,(k*7+i*29+k/5) : Next
Next
Use BitMap 0
VWait
BLITZ
For k=1 To 150
  n=rn{3,8} : points{a,n,-60,220,-40,100}
  If k AND 1 Then BitMapPoly n,a,rn{0,7} Else BitMapPoly -n,a,rn{0,7}
Next
Use BitMap 1
For k=1 To 150
  n=rn{3,8} : points{a,n,-60,220,-40,100}
  If k AND 1 Then BitMapPoly n,a,rn{0,7},2 Else BitMapPoly -n,a,rn{0,7},2
Next
VWait 2
AMIGA
bsum{"bitmappoly",0}
bsum{"dither",1}
BitMap 0,160,64,3
BitMap 1,160,64,3
VWait
BLITZ
RenderToBitmap 0
For k=1 To 100
  n=rn{3,8} : points{a,n,-60,220,-40,100} : RenderPoly n,a,rn{0,7}
Next
RenderToBitmap 1,2
For k=1 To 100
  n=rn{3,8} : points{a,n,-60,220,-40,100} : RenderPoly n,a,rn{0,7}
Next
VWait 2
AMIGA
bsum{"render",0}
bsum{"render dither",2}
cs=CustomChunkySize(64,16)
InitCopList 0,44,64,1,0,2,cs
CustomChunky 0,0,8,64,16,1
c=Addr CopList(0)
ChunkyCLS 0
VWait
BLITZ
For k=1 To 150
  n=rn{3,8} : points{a,n,-30,90,-10,26}
  If k AND 1 Then ChunkyPoly n,a,rn{0,$FFF} Else ChunkyPoly n,a,rn{0,$FFF},1
Next
VWait 2
AMIGA
csum{"chunkypoly",c}
ChunkyCLS $111
VWait
BLITZ
RenderToChunky 0
For k=1 To 150
  n=rn{3,8} : points{a,n,-30,90,-10,26} : RenderPoly n,a,rn{0,14}
Next
VWait 2
AMIGA
csum{"render chunky",c}
For k=1 To 300 : ChunkyPlot rn{-20,80},rn{-8,24},rn{0,$FFF},1 : Next
For k=1 To 300 : ChunkyPlot rn{0,63},rn{0,15},rn{0,$FFF} : Next
csum{"plot",c}
For k=1 To 200 : ChunkyVlin rn{-20,80},rn{-30,40},rn{-30,40},rn{0,$FFF},1 : Next
For k=1 To 200
  y1=rn{0,15} : ChunkyVlin rn{0,63},y1,rn{y1,15},rn{0,$FFF}
Next
csum{"vlin",c}
End

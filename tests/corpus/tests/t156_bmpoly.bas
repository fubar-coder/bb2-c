; t156_bmpoly - polylib on bitmaps: BitMapPoly (plain and with a dither
; bitmap, clockwise and backwards point lists, clipped at single edges),
; RenderToBitmap + RenderPoly (both forms; the two-bitmap form draws on
; the second bitmap with the first as dither bitmap - original).  polylib
; does not own the blitter and RenderPoly is Blitz mode only, so all
; drawing happens in a short BLITZ ... AMIGA section (no display, no
; events, no allocation; VWait before AMIGA); checksums in Amiga mode.
DEFTYPE .l
Statement bsum{bn.l}
  b.l=Addr BitMap(bn)
  For i.l=0 To Peek.b(b+5)-1
    pl.l=Peek.l(b+8+i*4) : s.l=0
    For k.l=0 To Peek.w(b)*Peek.w(b+2)-1 Step 2 : s=(s*5+Peek.w(pl+k))&$FFFFFF : Next
    Print " ",s
  Next
  NPrint ""
End Statement
Statement setp{m.l,a$}
  While a$<>""
    i.l=Instr(a$,",")
    If i=0 Then i=Len(a$)+1
    Poke.w m,Val(Left$(a$,i-1)) : m=m+2
    a$=Mid$(a$,i+1)
  Wend
End Statement
Dim p.w(200)
a=&p(0)
setp{a,"10,10,60,15,40,50,5,40"}
setp{a+20,"80,10,150,10,150,40,80,40"}
setp{a+40,"30,-20,70,10,20,30"}
setp{a+60,"100,30,140,80,90,70"}
setp{a+80,"-30,20,30,10,20,50"}
setp{a+100,"130,5,200,30,140,60"}
setp{a+120,"20,60,40,55,60,58,62,62,40,63,15,62"}
setp{a+160,"40,50,5,40,10,10,60,15"}
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
BitMapPoly 4,a,1 : BitMapPoly 4,a+20,3 : BitMapPoly 3,a+40,5 : BitMapPoly 3,a+60,6
BitMapPoly 3,a+80,7 : BitMapPoly 3,a+100,2 : BitMapPoly 6,a+120,4 : BitMapPoly -4,a+160,6
Use BitMap 1
BitMapPoly 4,a,1,2 : BitMapPoly 4,a+20,3,2 : BitMapPoly 3,a+40,5,2 : BitMapPoly 3,a+60,6,2
BitMapPoly 3,a+80,7,2 : BitMapPoly 3,a+100,2,2 : BitMapPoly 6,a+120,4,2
VWait 2
AMIGA
Print "bitmappoly" : bsum{0}
Print "dither" : bsum{1}
BitMap 0,160,64,3
BitMap 1,160,64,3
BLITZ
RenderToBitmap 0
RenderPoly 4,a,3 : RenderPoly 3,a+40,6
RenderToBitmap 0,2
RenderPoly 4,a+20,1 : RenderPoly 3,a+60,2
VWait 2
AMIGA
Print "render" : bsum{0}
Print "render 2" : bsum{2}
End

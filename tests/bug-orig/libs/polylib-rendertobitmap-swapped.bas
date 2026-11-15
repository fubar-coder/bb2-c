; polylib-rendertobitmap-swapped (#421) - RenderToBitmap BitMap#,
; DitherBitMap# stores the target and the dither bitmap, but RenderPoly
; (rpoly1) loads them swapped: it draws on the dither bitmap with the
; target as the dither pattern, and the target stays unchanged.
; Here a 320x200 target (2 planes, cleared) and a 32x8 dither bitmap (2
; planes, every bit set); RenderToBitmap 0,1 and a triangle in colour 1,
; once in each orientation (BitMapPoly fills only one), in a short
; BLITZ ... AMIGA section (RenderPoly is Blitz mode only).  Counted: the
; bytes set in the target, the bytes of the dither bitmap that are no
; longer $FF.
; Correct: the triangle is drawn on the target (bitmap 0, through the
; pattern of bitmap 1); the dither bitmap is not changed.
; @fixed #421
; @configs clib
DEFTYPE .l
BitMap 0,320,200,2
BitMap 1,32,8,2
d=Addr BitMap(1)
For i=0 To 1
  pl=Peek.l(d+8+i*4)
  For k=0 To 31 : Poke.b pl+k,$FF : Next
Next
Dim p.w(20)
p(0)=2 : p(1)=1 : p(2)=30 : p(3)=2 : p(4)=12 : p(5)=7
a=&p(0)
RenderToBitmap 0,1
VWait
BLITZ
RenderPoly 3,a,1
RenderPoly -3,a,1
VWait 2
AMIGA
b=Addr BitMap(0) : n=0
For i=0 To 1
  pl=Peek.l(b+8+i*4)
  For k=0 To 7999
    If Peek.b(pl+k)<>0 Then n=n+1
  Next
Next
c=0
For i=0 To 1
  pl=Peek.l(d+8+i*4)
  For k=0 To 31
    If (Peek.b(pl+k) AND $FF)<>$FF Then c=c+1
  Next
Next
NPrint "target drawn ",n>0
NPrint "dither changed ",c
End

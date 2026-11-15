; shapetrixlib-xflip-blitwait (#271) - XFlip of a shape whose width is
; not a multiple of 16 pixels first shifts each plane with the blitter
; (right by the pad bits) and then mirrors the bytes of every row with the
; CPU, without waiting for the last blit: the CPU mirrors rows the
; blitter has not shifted yet (in Amiga mode the wait before
; DisownBlitter is BTST twice without a loop).  The result depends on the
; timing; it shows with a tall shape of one plane, whose only blit is the
; last one.
; Here: a 13x200 shape of one plane from a pattern, XFlip twice, in Amiga
; mode and in Blitz mode; the rows (one word each) are compared with
; those before.
; Correct: two XFlips give the shape back ("ok").  The original changes
; pixels in both modes ("wrong", the number of pixels depends on the
; timing).
; @fixed #271
; @configs clib
DEFTYPE .l
Dim o.l(199)
BitMap 0,64,200,1
b=Addr BitMap(0)
For i=0 To 1599
  Poke.b Peek.l(b+8)+i,(i*37+(i/8)*5) AND 255
Next
For k=0 To 1
  Use BitMap 0
  GetaShape 0,3,0,13,200
  d=Peek.l(Addr Shape(0)+14)
  For y=0 To 199 : o(y)=Peek.w(d+y*2) AND $FFFF : Next
  If k=0 Then XFlip 0 : XFlip 0
  If k=1 Then BLITZ : XFlip 0 : XFlip 0 : AMIGA
  d=Peek.l(Addr Shape(0)+14)
  c=0
  For y=0 To 199
    If (Peek.w(d+y*2) AND $FFFF)<>o(y) Then c=c+1
  Next
  If k=0 Then m$="amiga " Else m$="blitz "
  If c=0 Then NPrint m$,"ok" Else NPrint m$,"wrong"
Next
End

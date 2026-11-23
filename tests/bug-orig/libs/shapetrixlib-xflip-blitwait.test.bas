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
DEFTYPE .l
Function.l wrongrows{}
  SHARED ob.l
  d=Peek.l(Addr Shape(0)+14)
  c=0
  For y=0 To 199
    If (Peek.w(d+y*2) AND $FFFF)<>(Peek.w(ob+y*2) AND $FFFF) Then c=c+1
  Next
  Function Return c
End Function
Statement setup{}
  SHARED ob.l
  ob=AllocMem(400,65536)
  b=Addr BitMap(0)
  For i=0 To 1599
    Poke.b Peek.l(b+8)+i,(i*37+(i/8)*5) AND 255
  Next
  Use BitMap 0
  GetaShape 0,3,0,13,200
  d=Peek.l(Addr Shape(0)+14)
  For y=0 To 199 : Poke.w ob+y*2,Peek.w(d+y*2) : Next
End Statement

!TestCase{amiga_mode}
  BitMap 0,64,200,1
  setup{}
  XFlip 0 : XFlip 0
  !CheckEq{wrongrows{},0,"wrong_rows"}
  FreeMem ob,400
!EndTestCase

!TestCase{blitz_mode}
  BitMap 0,64,200,1
  setup{}
  BLITZ : XFlip 0 : XFlip 0 : AMIGA
  !CheckEq{wrongrows{},0,"wrong_rows"}
  FreeMem ob,400
!EndTestCase

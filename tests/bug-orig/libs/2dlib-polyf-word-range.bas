; 2dlib-polyf-word-range (#1000) - Polyf clips and steps the edges with
; word arithmetic: the slope of an edge is one DIVS of dx * 4^k by dy (it
; overflows from a dx of about 8192 on), an edge of 16384 lines or more
; gets the slope 0, and the clipping at the window edges divides with
; DIVS too.  A polygon with vertices far outside the bitmap is not drawn.
; Here, on a 320x200 bitmap of 2 planes with Polyf n,&p,1,2 (colour 1
; clockwise, colour 2 anticlockwise): a triangle around the bitmap with
; vertices at +-1000 (in the word range: the control), the same at
; +-20000, the same anticlockwise; printed are the bytes of plane 0 and
; plane 1 that are $FF.  Then a triangle whose right edge is the vertical
; x = 160 and whose other vertices lie at +-20000, compared with the same
; triangle at +-1000 (the visible part is the same, x = 0 to 160 of every
; line): printed is the number of bytes of plane 0 that differ.
; Correct: the triangles cover the bitmap (8000 bytes of the colour's
; plane, 0 of the other), and the two half triangles are drawn alike.
; @fixed #1000
DEFTYPE .l
Statement setp{m.l,a$}
  While a$<>""
    i.l=Instr(a$,",")
    If i=0 Then i=Len(a$)+1
    Poke.w m,Val(Left$(a$,i-1)) : m=m+2
    a$=Mid$(a$,i+1)
  Wend
End Statement
Function.l full{pl.l}
  a.l=Peek.l(Addr BitMap(0)+8+pl*4) : n.l=0
  For i.l=0 To 7999
    If (Peek.b(a+i) AND 255)=255 Then n=n+1
  Next
  Function Return n
End Function
Statement tri{t$,a$}
  SHARED m
  setp{m,a$}
  Use BitMap 0 : Cls
  Polyf 3,m,1,2
  NPrint t$," ",full{0}," ",full{1}
End Statement
Function.l diff{}
  a.l=Peek.l(Addr BitMap(0)+8) : b.l=Peek.l(Addr BitMap(1)+8) : n.l=0
  For i.l=0 To 7999
    If Peek.b(a+i)<>Peek.b(b+i) Then n=n+1
  Next
  Function Return n
End Function
BitMap 0,320,200,2
BitMap 1,320,200,2
Dim p.w(20)
m=&p(0)
tri{"small","-1000,-1000,1000,-1000,0,1000"}
tri{"wild","-20000,-20000,20000,-20000,0,20000"}
tri{"wild anti","-20000,-20000,0,20000,20000,-20000"}
setp{m,"-1000,-1000,160,-1000,160,1000"}
Use BitMap 1 : Cls : Polyf 3,m,1
setp{m,"-20000,-20000,160,-20000,160,20000"}
Use BitMap 0 : Cls : Polyf 3,m,1
NPrint "half differ ",diff{}
End

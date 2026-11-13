; polylib-word-range (#1019, won't fix) - the polygon code of BitMapPoly and
; ChunkyPoly computes edge slopes, clip crossings and coordinate differences
; in word arithmetic: the slope of an edge is one DIVS of dx * 8^k by dy and
; overflows for an edge of 4096 lines or more that is at least as wide as
; high (the edge is then drawn vertical), and the clipping subtracts
; coordinates as words, which wraps when two points are 32768 or more apart
; (the polygon is not drawn).
; Here, on a 320x200 bitmap of 1 plane, BitMapPoly (Blitz mode) draws the
; triangle (0,0), (2s,s), (0,s): its visible part is the same for every
; s >= 200, so printed is the number of bytes of the plane that differ from
; the drawing for s = 200 (s = 4095 and 4096).  Then the triangle
; (-s,-s), (s,-s), (0,s) around the bitmap (s = 16383 and 16384): printed is
; the number of bytes of the plane that are $FF.  The same for ChunkyPoly on
; a copper chunky area of 64x16 pixels (never displayed; ChunkyPoly swaps x
; and y inside, so the triangles are (0,0), (s,2s), (0,2s) and
; (-s,-s), (s,0), (-s,s)): printed
; are the pixels that differ from the drawing for s = 100, and the pixels
; the cover triangle sets.
; Correct: no byte or pixel differs, the cover triangles fill the bitmap
; (8000 bytes) and the chunky area (1024 pixels).
; @pass timeout=120
DEFTYPE .l
Function.l full{}
  a.l=Peek.l(Addr BitMap(0)+8) : n.l=0
  For i.l=0 To 7999
    If (Peek.b(a+i) AND 255)=255 Then n=n+1
  Next
  Function Return n
End Function
Function.l diff{}
  a.l=Peek.l(Addr BitMap(0)+8) : b.l=Peek.l(Addr BitMap(1)+8) : n.l=0
  For i.l=0 To 7999
    If Peek.b(a+i)<>Peek.b(b+i) Then n=n+1
  Next
  Function Return n
End Function
Statement tri{a$,k.l,s.l}
  SHARED m
  Select k
    Case 0 : Poke.w m,0 : Poke.w m+2,0 : Poke.w m+4,2*s : Poke.w m+6,s : Poke.w m+8,0 : Poke.w m+10,s
    Case 1 : Poke.w m,-s : Poke.w m+2,-s : Poke.w m+4,s : Poke.w m+6,-s : Poke.w m+8,0 : Poke.w m+10,s
    Case 2 : Poke.w m,0 : Poke.w m+2,0 : Poke.w m+4,s : Poke.w m+6,2*s : Poke.w m+8,0 : Poke.w m+10,2*s
    Case 3 : Poke.w m,-s : Poke.w m+2,-s : Poke.w m+4,s : Poke.w m+6,0 : Poke.w m+8,-s : Poke.w m+10,s
  End Select
  If a$="bitmap"
    Use BitMap 0 : Cls
    VWait
    BLITZ
    BitMapPoly 3,m,1
    VWait 2
    AMIGA
  Else
    ChunkyCLS 0
    VWait
    BLITZ
    ChunkyPoly 3,m,$FFF
    VWait 2
    AMIGA
  EndIf
End Statement
Function.l pix{x.l,y.l}
  SHARED c
  Function Return Peek.w(Peek.l(c+96)+(x&31)*4+(x/32)*132+y*Peek.w(c+108))
End Function
Statement csnap{}
  SHARED q
  For y.l=0 To 15
    For x.l=0 To 63 : Poke.w q+(y*64+x)*2,pix{x,y} : Next
  Next
End Statement
Function.l cdiff{}
  SHARED q
  n.l=0
  For y.l=0 To 15
    For x.l=0 To 63
      If Peek.w(q+(y*64+x)*2)<>pix{x,y} Then n=n+1
    Next
  Next
  Function Return n
End Function
Function.l cfull{}
  n.l=0
  For y.l=0 To 15
    For x.l=0 To 63
      If pix{x,y}<>0 Then n=n+1
    Next
  Next
  Function Return n
End Function
BitMap 0,320,200,1
BitMap 1,320,200,1
cs=CustomChunkySize(64,16)
InitCopList 0,44,64,1,0,2,cs
CustomChunky 0,0,8,64,16,1
c=Addr CopList(0)
Dim p.w(8),r.w(1024)
m=&p(0) : q=&r(0)
tri{"bitmap",0,200}
a=Peek.l(Addr BitMap(0)+8) : b=Peek.l(Addr BitMap(1)+8)
For i=0 To 7999 : Poke.b b+i,Peek.b(a+i) : Next
n=0 : For i=0 To 7999 : If Peek.b(a+i)<>0 Then n=n+1
Next
NPrint "bitmap reference bytes set ",n
For k=0 To 1
  s=4095+k
  tri{"bitmap",0,s}
  NPrint "bitmap slope ",s," differ ",diff{}
Next
For k=0 To 1
  s=16383+k
  tri{"bitmap",1,s}
  NPrint "bitmap cover ",s," full ",full{}
Next
tri{"chunky",2,100}
csnap{}
NPrint "chunky reference pixels set ",cfull{}
For k=0 To 1
  s=4095+k
  tri{"chunky",2,s}
  NPrint "chunky slope ",s," differ ",cdiff{}
Next
For k=0 To 1
  s=16383+k
  tri{"chunky",3,s}
  NPrint "chunky cover ",s," pixels ",cfull{}
Next
End

; 2dlib-polyf-overflow (#395) - Polyf keeps the edges of each side in a
; list of 32 words (3 words per edge and a 2 word end marker, room for 10
; edges) and writes it without a limit: an 11th edge on the left side runs
; into the list of the right side, which is written afterwards.
; Here a polygon whose left side is a vertical line cut into pieces is
; compared with the same polygon with an uncut left side (the cut points
; lie on the edge, so both cover the same pixels): with 10 left edges
; (sanity) and with 14 left edges (overflow, still 16 points: no overflow
; of the point buffer).  Counted: bytes of plane 0 that differ.
; Correct: both polygons are drawn alike in both cases.
; @configs clib
DEFTYPE .l
Statement setp{m.l,a$}
  While a$<>""
    i.l=Instr(a$,",")
    If i=0 Then i=Len(a$)+1
    Poke.w m,Val(Left$(a$,i-1)) : m=m+2
    a$=Mid$(a$,i+1)
  Wend
End Statement
Function.l diff{}
  a.l=Peek.l(Addr BitMap(0)+8) : b.l=Peek.l(Addr BitMap(1)+8) : n.l=0
  For i.l=0 To 7999
    If Peek.b(a+i)<>Peek.b(b+i) Then n=n+1
  Next
  Function Return n
End Function
BitMap 0,320,200,1
BitMap 1,320,200,1
Dim p.w(100)
a=&p(0)
setp{a,"55,2,10,10,10,130,100,140,100,10"}
Use BitMap 1 : Polyf 5,a,1
setp{a,"55,2,10,10,10,25,10,40,10,55,10,70,10,85,10,100,10,115,10,130,100,140,100,10"}
Use BitMap 0 : Polyf 12,a,1
NPrint "10 edges differ ",diff{}
Use BitMap 0 : Cls
setp{a,"55,2,10,10,10,20,10,30,10,40,10,50,10,60,10,70,10,80,10,90,10,100,10,110,10,120,10,130,100,140,100,10"}
Polyf 16,a,1
NPrint "14 edges differ ",diff{}
End

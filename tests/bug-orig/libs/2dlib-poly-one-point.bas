; 2dlib-poly-one-point (#396) - Poly stores numpoints-1 in a word counter
; and draws before it counts down (SUB #1,pfc:BNE): with 1 point the
; counter goes from 0 to $FFFF and 65535 more lines are drawn from the
; words behind the coordinates.
; Here the coordinate array has room for 65536 more points, all (0,0) (set
; here: Dim leaves what was in the memory), so the lines stay inside the
; bitmap.  Counted: bytes set in the bitmap's
; plane, for 2 points (sanity: one short line) and for 1 point.
; Correct: a polygon of one point draws no line (nothing set).
; @configs clib
DEFTYPE .l
Function.l nset{}
  pl.l=Peek.l(Addr BitMap(0)+8) : n.l=0
  For i.l=0 To 7999
    If Peek.b(pl+i)<>0 Then n=n+1
  Next
  Function Return n
End Function
BitMap 0,320,200,1
Dim p.w(131100)
For i.l=0 To 131100 : p(i)=0 : Next
p(0)=40 : p(1)=30 : p(2)=47 : p(3)=30
Poly 2,&p(0),1
NPrint "2 points, at most 2 bytes set ",nset{}<=2
Cls
p(2)=0
Poly 1,&p(0),1
NPrint "1 point, bytes set ",nset{}
End

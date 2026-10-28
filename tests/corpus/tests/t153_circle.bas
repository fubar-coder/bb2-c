; t153_circle - 2dlib in Amiga mode: Circle and Circlef (circle and
; ellipse forms, complement colour, clipped at the left, right and bottom
; edges, BitMapOrigin) and FreeFill; checksums of the bitmap planes.
; No circle reaches above the top edge (the original then plots far
; outside the bitmap).
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
BitMap 0,160,100,3
Use BitMap 0
Cls
Circle 40,40,20,1 : Circle 40,40,5,2 : Circle 100,50,30,10,3 : Circle 100,50,8,30,4
Circle 130,20,0,5 : Circle 20,90,3,6 : Circle 80,80,1,1,7 : Circle 60,20,12,-1
Print "circle" : bsum{0}
Circle 5,50,20,2 : Circle 150,40,25,3 : Circle 80,90,30,4 : Circle 155,95,40,10,5
Circle -10,60,20,6 : Circle 200,50,10,7
Print "circle clip" : bsum{0}
Cls
Circlef 40,40,20,1 : Circlef 40,40,5,2 : Circlef 100,50,30,10,3 : Circlef 100,50,8,30,4
Circlef 130,20,0,5 : Circlef 20,90,3,6 : Circlef 80,80,1,1,7 : Circlef 60,20,12,-1
Print "circlef" : bsum{0}
Circlef 5,50,20,2 : Circlef 150,40,25,3 : Circlef 80,90,30,4 : Circlef 155,95,40,10,5
Circlef 64,70,16,-1 : Circlef 64,70,16,-1
Print "circlef clip" : bsum{0}
Cls
BitMapOrigin 0,20,10
Circle 20,20,10,3 : Circlef 60,30,15,5
BitMapOrigin 0,0,0
Print "origin" : bsum{0}
FreeFill
BitMap 1,1040,40,2
Use BitMap 1
Circlef 520,20,500,18,1 : Circlef 30,20,10,3 : Circle 1000,20,30,15,2
Print "wide" : bsum{1}
FreeFill
FreeFill
End

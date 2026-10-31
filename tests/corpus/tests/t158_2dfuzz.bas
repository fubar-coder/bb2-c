; t158_2dfuzz - 2dlib in Amiga mode with pseudo random arguments (fixed
; seed): Poly, Polyf (both colour forms, at most 8 points: no overflow of
; the point buffer or the edge lists, #395; also coordinates far outside),
; Line (all three forms), Box, Boxf (complement and long colours), Circle
; and Circlef (both forms; centres far enough from the top and bottom
; edge: points above the top are written outside the bitmap, #392),
; ReMap, FloodFill (both forms), then BitMapOrigin with Line, Boxf and
; Polyf; checksums of the bitmap planes after each part.
DEFTYPE .l
Function.l rn{lo.l,hi.l}
  SHARED seed.l
  seed=(seed*69069+1) AND $7FFFFFFF
  Function Return lo+((seed/256) MOD (hi-lo+1))
End Function
Statement sums{t$}
  b.l=Addr BitMap(0)
  Print t$
  For pl.l=0 To 2
    a.l=Peek.l(b+8+pl*4) : s.l=0
    For i.l=0 To 7999 Step 2 : s=(s*5+(Peek.w(a+i) AND $FFFF)) AND $FFFFFF : Next
    Print " ",Hex$(s)
  Next
  NPrint ""
End Statement
seed=12345
BitMap 0,320,200,3
Dim p.w(40)
Cls
For k=1 To 300
  n=rn{2,8}
  For i=0 To n*2-1 : p(i)=rn{-200,500} : Next
  Poly n,&p(0),rn{-1,7}
Next
sums{"poly"}
Cls
For k=1 To 300
  n=rn{3,8}
  For i=0 To n*2-1 : p(i)=rn{-150,450} : Next
  If k AND 1 Then Polyf n,&p(0),rn{0,7} Else Polyf n,&p(0),rn{0,7},rn{0,7}
Next
sums{"polyf"}
Cls
For k=1 To 200
  n=rn{3,8}
  For i=0 To n*2-1 : p(i)=rn{-20000,20000} : Next
  Polyf n,&p(0),rn{0,7},rn{0,7}
Next
sums{"polyf wild"}
Cls
For k=1 To 400 : Line rn{-30000,30000},rn{-30000,30000},rn{-30000,30000},rn{-30000,30000},rn{-1,7} : Next
For k=1 To 400 : Line rn{-400,700},rn{-400,600},rn{-400,700},rn{-400,600},rn{-1,7} : Next
For k=1 To 100 : Line rn{-50,350},rn{-50,250},rn{-1,7} : Next
For k=1 To 30 : Line rn{-50,350},rn{-50,250},rn{-50,350},rn{-50,250},rn{-1,7},rn{0,5} : Next
sums{"line"}
Cls
For k=1 To 200 : Box rn{-100,400},rn{-100,300},rn{-100,400},rn{-100,300},rn{-1,7} : Next
sums{"box"}
Cls
For k=1 To 200 : Boxf rn{-100,400},rn{-100,300},rn{-100,400},rn{-100,300},rn{-3,9} : Next
sums{"boxf"}
Cls
For k=1 To 150
  r=rn{0,60} : r2=rn{0,60} : y=rn{60,140}
  If k AND 1 Then Circle rn{-80,400},y,r,rn{-1,7} Else Circle rn{-80,400},y,r,r2,rn{-1,7}
Next
sums{"circle"}
Cls
For k=1 To 150
  r=rn{0,60} : r2=rn{0,60} : y=rn{62,138}
  If k AND 1 Then Circlef rn{-80,400},y,r,rn{-1,7} Else Circlef rn{-80,400},y,r,r2,rn{-1,7}
Next
sums{"circlef"}
For k=1 To 40 : ReMap rn{0,7},rn{0,7} : Next
sums{"remap"}
For k=1 To 30 : FloodFill rn{0,319},rn{0,199},rn{0,7} : Next
sums{"flood"}
For k=1 To 30 : FloodFill rn{0,319},rn{0,199},rn{0,7},rn{0,7} : Next
sums{"flood2"}
BitMapOrigin 0,rn{-50,50},rn{-50,50}
Cls
For k=1 To 100 : Line rn{-100,400},rn{-100,300},rn{-100,400},rn{-100,300},rn{-1,7} : Next
For k=1 To 100 : Boxf rn{-100,400},rn{-100,300},rn{-100,400},rn{-100,300},rn{-1,7} : Next
For k=1 To 100
  n=rn{3,8}
  For i=0 To n*2-1 : p(i)=rn{-150,450} : Next
  Polyf n,&p(0),rn{0,7},rn{0,7}
Next
sums{"origin"}
End

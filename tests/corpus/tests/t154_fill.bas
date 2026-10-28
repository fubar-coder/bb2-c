; t154_fill - 2dlib in Amiga mode: FloodFill (region of the colour under
; the point, and up to a border colour; regions touching the edges),
; ReMap (in place and into another bitmap), BitPlanesBitMap,
; BitMapWindow (drawing through both), FreeFill; a bitmap wider than 64
; words for the column split of the mask blits; checksums of the planes
; and the bitmap object fields.
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
Statement bdump{bn.l}
  b.l=Addr BitMap(bn)
  Print "bm ",bn,":"
  For i.l=0 To 6 Step 2 : Print " ",Peek.w(b+i) : Next
  Print " pl"
  For i.l=0 To Peek.b(b+5)-1 : Print " ",Peek.l(b+8+i*4)-Peek.l(Addr BitMap(0)+8) : Next
  For i.l=48 To 62 Step 2 : Print " ",Peek.w(b+i) : Next
  NPrint ""
End Statement
BitMap 0,128,64,3
Use BitMap 0
Cls
Box 10,10,60,50,1 : Box 30,5,100,30,2 : Line 0,40,127,20,3 : Circle 90,45,15,4
FloodFill 20,20,5 : Print "flood 1" : bsum{0}
FloodFill 120,60,6 : Print "flood 2" : bsum{0}
FloodFill 0,0,7 : Print "flood 3" : bsum{0}
FloodFill 90,45,3,4 : Print "flood border" : bsum{0}
FloodFill 2,62,1,3 : Print "flood border 2" : bsum{0}
FloodFill 50,25,1,1 : Print "flood same" : bsum{0}
ReMap 7,2 : Print "remap" : bsum{0}
ReMap 3,0 : Print "remap 2" : bsum{0}
BitMap 1,128,64,3
Use BitMap 0
ReMap 1,6,1 : Print "remap into" : bsum{1} : bsum{0}
ReMap 5,5 : Print "remap same" : bsum{0}
; bitplanes / window bitmaps (bitmaps 2 and 3 are not real yet)
BitPlanesBitMap 0,2,%101 : bdump{2}
Use BitMap 2
Boxf 0,0,63,31,3 : Line 64,0,127,63,1
Print "planes" : bsum{0}
BitMapWindow 0,3,20,8,64,40 : bdump{3}
Use BitMap 3
Boxf 0,0,100,100,2 : Line 0,0,63,39,5 : Plot 63,39,7 : Plot 64,0,7
Print "window" : bsum{0}
BitPlanesBitMap 0,2,3 : NPrint "again (real)" : bdump{2}
BitMapOrigin 3,4,2 : bdump{3}
; wider than 64 words
BitMap 4,1056,20,2
Use BitMap 4
Cls
Box 5,2,1050,17,1 : Line 500,0,520,19,2 : Line 1040,0,1000,19,3
FloodFill 10,10,2 : Print "wide flood" : bsum{4}
FloodFill 1052,1,3,2 : Print "wide border" : bsum{4}
ReMap 2,1 : Print "wide remap" : bsum{4}
FreeFill
FloodFill 600,5,3 : Print "after free" : bsum{4}
FreeFill
End

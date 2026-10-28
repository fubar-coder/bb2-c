; t150_scroll - scrolllib in Amiga mode: Plot/Point (clipping at all
; edges, colour -1 = complement), Cls (with colour), Scroll (ascending,
; descending, overlapping, from another bitmap, wider than 960 pixels:
; two blits) and BlockScroll; results as checksums of the bitmap planes.
; All Scroll areas lie inside both bitmaps (error checks in the runerr
; configurations).
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
Statement pfill{bn.l,m.l}
  b.l=Addr BitMap(bn)
  For i.l=0 To Peek.b(b+5)-1
    pl.l=Peek.l(b+8+i*4)
    For k.l=0 To Peek.w(b)*Peek.w(b+2)-1 : Poke.b pl+k,(k*m+i*29+k/5) : Next
  Next
End Statement
BitMap 0,96,40,3
Use BitMap 0
Cls
For i=0 To 39 : Plot i*2,i,i&7 : Plot 95-i,i,(i/3)&7 : Next
Plot -1,5,3 : Plot 96,5,3 : Plot 5,40,3 : Plot 5,-1,3 : Plot 200,300,7
Plot 10,10,-1 : Plot 11,10,-1 : Plot 11,10,-1 : Plot 2,1,-1
Print "point ",Point(0,0)," ",Point(2,1)," ",Point(10,10)," ",Point(11,10)
NPrint " ",Point(95,0)," ",Point(94,1)," ",Point(39,39)," ",Point(56,39)
NPrint "point out ",Point(-1,0)&7," ",Point(96,0)&7," ",Point(0,40)&7," ",Point(0,-1)&7
Print "plot" : bsum{0}
Cls 5 : Print "cls 5" : bsum{0}
Cls 2 : Print "cls 2" : bsum{0}
Cls : Print "cls" : bsum{0}
pfill{0,7}
Scroll 0,0,40,20,30,10 : Print "desc" : bsum{0}
Scroll 30,10,40,20,3,2 : Print "asc" : bsum{0}
Scroll 5,5,17,9,60,25 : Print "odd" : bsum{0}
Scroll 33,7,50,1,33,6 : Print "up 1" : bsum{0}
Scroll 1,0,95,40,0,0 : Print "left 1" : bsum{0}
Scroll 0,0,95,40,1,0 : Print "right 1" : bsum{0}
BitMap 1,96,40,2
pfill{1,3}
Use BitMap 0
Scroll 8,4,50,20,20,10,1 : Print "from 1" : bsum{0}
Scroll 0,0,96,40,0,0,1 : Print "copy 1" : bsum{0}
pfill{0,11}
BlockScroll 8,0,32,16,40,20 : Print "block" : bsum{0}
BlockScroll 0,0,24,10,16,30,1 : Print "block 1" : bsum{0}
BlockScroll 16,8,64,24,8,0 : Print "block up" : bsum{0}
Use BitMap 1
Scroll 0,0,48,40,48,0,0 : Print "into 1" : bsum{1}
; wider than 960 pixels: two blits, the width word is changed on the stack
BitMap 2,1024,6,2
pfill{2,5}
Use BitMap 2
Scroll 0,0,1000,4,16,1 : Print "wide desc" : bsum{2}
Scroll 20,1,1000,5,0,0 : Print "wide asc" : bsum{2}
Scroll 3,0,961,6,40,0 : Print "wide 961" : bsum{2}
w=1000 : Scroll 0,0,w,3,8,2 : NPrint "w after ",w : Print "wide var" : bsum{2}
End

; t151_line - 2dlib in Amiga mode: Line (all three forms, all octants,
; clipping at every edge, complement colour -1), Box, Boxf (clipped,
; complement, long colour), BitMapOrigin (Box adds the origin twice -
; original behaviour), Plot with origin; checksums of the bitmap planes.
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
BitMap 0,128,64,3
Use BitMap 0
Cls
Line 0,0,127,63,1 : Line 127,0,0,63,2 : Line 10,5,10,50,3 : Line 5,30,120,30,4
Line 64,32,70,2,5 : Line 64,32,58,62,6 : Line 64,32,20,28,7 : Line 64,32,20,36,1
Line 64,32,108,28,2 : Line 64,32,108,36,3 : Line 64,32,70,62,4 : Line 64,32,58,2,5
Print "lines" : bsum{0}
Line 30,40,90,45,5,2 : Line 100,10,100,50,6,0 : Line 40,60,44,20,3,3
Print "width" : bsum{0}
Line 64,20,1 : Line 80,10,2 : Line 5,5,3 : Line 5,5,4
Print "lineto" : bsum{0}
Line -20,-10,140,70,6 : Line 100,-5,140,20,7 : Line -10,70,50,-10,1
Line -10,-10,-1,-1,2 : Line 130,10,200,20,3 : Line 20,64,30,90,4 : Line 20,-30,30,-1,5
Line -100,32,300,33,6 : Line 64,-100,65,300,7 : Line 127,0,127,63,1 : Line 0,63,127,63,2
Print "clip" : bsum{0}
Line 5,5,20,9,-1 : Line 5,5,20,9,-1 : Line 0,10,127,12,-1 : Line 3,0,3,63,-1
Print "xor" : bsum{0}
Cls
Box 10,10,50,40,3 : Box 60,50,20,5,6 : Box -5,-5,30,20,2 : Box 100,40,140,80,5
Box 70,8,70,8,7 : Box 4,60,90,60,1 : Box 30,30,40,35,-1
Print "box" : bsum{0}
Boxf 70,10,120,30,5 : Boxf 0,40,200,70,6 : Boxf 3,3,9,9,-1 : Boxf 40,20,41,21,$12345678
Boxf 20,20,17,17,3 : Boxf -10,-10,-1,-1,7 : Boxf 128,0,140,10,7 : Boxf 15,0,16,63,2
Boxf 31,50,32,51,$FFFFFFF8 : Boxf 50,5,99,15,-1
Print "boxf" : bsum{0}
Cls
BitMapOrigin 0,8,4
Plot 0,0,7 : Line 0,0,10,0,7 : Line 0,2,6 : Box 0,0,10,10,1 : Boxf 0,20,5,25,2
Line -8,-4,119,59,3 : Box 100,40,130,70,4 : Boxf 110,50,200,100,5
Print "origin" : bsum{0}
BitMapOrigin 0,0,0
Plot 1,1,3 : Line 0,63,64,0,4
Print "origin 0" : bsum{0}
; big bitmap: Boxf over more than 990 pixels and more than 1023 lines
BitMap 1,1056,8,2
Use BitMap 1
Boxf 3,1,1050,6,3 : Boxf 0,0,1055,7,-1 : Boxf 500,2,1500,4,1
Print "wide" : bsum{1}
BitMap 2,32,1100,1
Use BitMap 2
Boxf 4,2,27,1090,1 : Boxf 0,0,31,1099,-1 : Boxf 8,10,9,1080,0
Print "tall" : bsum{2}
Line 0,0,31,1099,1 : Line 31,5,0,1095,1
Print "tall line" : bsum{2}
End

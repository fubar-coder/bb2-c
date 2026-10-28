; t152_poly - 2dlib in Amiga mode: Poly (outline) and Polyf (filled, both
; orientations: anticlockwise points use colour2), polygons clipped at one
; edge each (top, bottom, left, right), BitMapOrigin; checksums of the
; bitmap planes.  At most 16 points (the original's point buffer).
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
Dim p.w(40)
Statement setp{m.l,a$}
  While a$<>""
    i.l=Instr(a$,",")
    If i=0 Then i=Len(a$)+1
    Poke.w m,Val(Left$(a$,i-1)) : m=m+2
    a$=Mid$(a$,i+1)
  Wend
End Statement
BitMap 0,160,100,3
Use BitMap 0
Cls
setp{&p(0),"10,10,60,15,40,50,5,40"} : Poly 4,&p(0),1 : Polyf 4,&p(0),2
setp{&p(0),"80,10,150,10,150,40,80,40"} : Polyf 4,&p(0),3 : Poly 4,&p(0),7
setp{&p(0),"80,50,80,90,150,90"} : Polyf 3,&p(0),4,5 : Polyf 3,&p(0),6
setp{&p(0),"100,60,130,55,120,85"} : Polyf 3,&p(0),1,2
setp{&p(0),"20,60,40,55,60,60,65,80,50,95,30,95,15,80,12,70"} : Polyf 8,&p(0),5 : Poly 8,&p(0),-1
setp{&p(0),"5,52,9,52"} : Poly 2,&p(0),3
Print "inside" : bsum{0}
Cls
setp{&p(0),"30,-20,70,10,20,30"} : Polyf 3,&p(0),3
setp{&p(0),"100,70,140,120,90,110"} : Polyf 3,&p(0),4
setp{&p(0),"-30,40,30,30,20,70"} : Polyf 3,&p(0),5
setp{&p(0),"130,20,200,50,140,80"} : Polyf 3,&p(0),6
setp{&p(0),"60,40,90,45,75,60"} : Polyf 3,&p(0),-1
Print "clip" : bsum{0}
Cls
setp{&p(0),"-10,-10,170,-5,165,110,-5,105"} : Poly 4,&p(0),2
setp{&p(0),"200,10,250,10,250,40"} : Polyf 3,&p(0),7 : Poly 3,&p(0),7
setp{&p(0),"10,120,50,130,30,150"} : Polyf 3,&p(0),7
Print "outside" : bsum{0}
Cls
BitMapOrigin 0,10,5
setp{&p(0),"0,0,30,0,30,20,0,20"} : Polyf 4,&p(0),3 : Poly 4,&p(0),5
BitMapOrigin 0,0,0
NPrint "points ",p(0)," ",p(1)," ",p(4)," ",p(5)
Print "origin" : bsum{0}
FreeFill
End

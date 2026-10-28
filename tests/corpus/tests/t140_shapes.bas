; t140_shapes - shapeslib in Amiga mode: GetaShape (AutoCookie on/off),
; MakeCookie, Handle, MidHandle, ShapeWidth/ShapeHeight, CopyShape,
; InitShape, SaveShapes/LoadShapes (both forms), DecodeShapes (CHIP
; memory: in place), LoadShape (ILBM, with palette).  Results: the shape
; blocks (without pointers) and checksums of data and cookies; files
; without the pointers of the blocks.
DEFTYPE .l
Function.l cks{a.l,n.l}
  c.l=0
  If a=0 Then Function Return -1
  For i.l=0 To n-1
    c=(c*3+(Peek.b(a+i) AND 255)) AND $FFFFFF
  Next
  Function Return c
End Function
Statement shinfo{s.l}
  a.l=Addr Shape(s)
  Print "shape ",s," ",Peek.w(a)," ",Peek.w(a+2)," ",Peek.w(a+4)," ",Peek.w(a+6)," ",Hex$(Peek.w(a+8) AND $FFFF)
  Print " h ",Peek.w(a+10)," ",Peek.w(a+12)," sz ",Peek.w(a+22)," ",Peek.w(a+24)," ",Peek.w(a+26)," ",Peek.w(a+28)
  NPrint " m ",Hex$(Peek.w(a+30) AND $FFFF)
  If Peek.w(a)<>0
    NPrint "  data ",Hex$(cks{Peek.l(a+14),Peek.w(a+26) AND $FFFF})," cookie ",Hex$(cks{Peek.l(a+18),Peek.w(a+22) AND $FFFF})
  EndIf
End Statement
Statement fsum{f$}
  If ReadFile(0,f$)
    l.l=Lof(0)
    m.l=AllocMem(l+8,65536)
    ReadMem 0,m,l
    CloseFile 0
    c.l=0 : p.l=0 : k.l=0
    While p<l
      For i.l=0 To 31
        If i<14 OR i>21 Then c=(c*3+(Peek.b(m+p+i) AND 255)) AND $FFFFFF
      Next
      w.l=Peek.w(m+p) : z.l=Peek.w(m+p+26) AND $FFFF
      p=p+32 : k=k+1
      If w<>0
        For i=0 To z-1 : c=(c*3+(Peek.b(m+p+i) AND 255)) AND $FFFFFF : Next
        p=p+z
      EndIf
    Wend
    NPrint f$," len ",l," blocks ",k," cks ",Hex$(c)
    FreeMem m,l+8
  Else
    NPrint f$," missing"
  EndIf
End Statement
BitMap 0,64,32,2
b=Addr BitMap(0)
For p=0 To 1
  For i=0 To 255
    v=(i*7+p*13+(i/8)*5) AND 255
    If (i AND 12)=4 Then v=0
    Poke.b Peek.l(b+8+p*4)+i,v
  Next
Next
Use BitMap 0
GetaShape 0,0,0,20,10
shinfo{0}
NPrint "size ",ShapeWidth(0)," ",ShapeHeight(0)
AutoCookie Off
GetaShape 1,3,5,33,7
shinfo{1}
MakeCookie 1
shinfo{1}
Handle 0,3,-4
MidHandle 1
shinfo{0} : shinfo{1}
AutoCookie On
CopyShape 0,2
shinfo{2}
CopyShape 1,3
shinfo{3}
InitShape 4,40,5,3
shinfo{4}
AutoCookie Off
InitShape 5,17,3,1
shinfo{5}
AutoCookie On
SaveShapes 0,5,"RAM:t140.shp"
fsum{"RAM:t140.shp"}
LoadShapes 10,15,"RAM:t140.shp"
For s=10 To 15 : shinfo{s} : Next
LoadShapes 30,"RAM:t140.shp"
For s=30 To 36 : shinfo{s} : Next
AutoCookie Off
LoadShapes 40,41,"RAM:t140.shp"
shinfo{40} : shinfo{41}
AutoCookie On
; DecodeShapes from CHIP memory: used in place ("cL")
If ReadFile(0,"RAM:t140.shp")
  l=Lof(0) : m=AllocMem(l+64,$10002) : ReadMem 0,m,l : CloseFile 0
  ; a block that does not match ends DecodeShapes (1*1<>9)
  Poke.w m+l+2,1 : Poke.w m+l+6,1 : Poke.w m+l+22,9
  DecodeShapes 50,55,m
  For s=50 To 55 : shinfo{s} : Next
  DecodeShapes 60,m
  For s=60 To 61 : shinfo{s} : Next
  NPrint "in place ",Peek.l(Addr Shape(50)+14)=m+32
EndIf
; LoadShape from an ILBM (iffmakelib SaveBitmap writes it)
InitPalette 0,4
PalRGB 0,1,15,0,0 : PalRGB 0,2,0,15,0 : PalRGB 0,3,3,4,5
SaveBitmap 0,"RAM:t140.iff",0
LoadShape 70,"RAM:t140.iff"
shinfo{70}
InitPalette 1,2
LoadShape 71,"RAM:t140.iff",1
shinfo{71}
p=Addr Palette(1)
NPrint "palette ",Peek.w(p+4)," ",Hex$(cks{Peek.l(p)+4,Peek.w(p+4)*12})
KillFile "RAM:t140.shp" : KillFile "RAM:t140.iff"
End

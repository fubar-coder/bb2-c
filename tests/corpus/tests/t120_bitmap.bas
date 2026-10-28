; t120_bitmap - bitmaplib: BitMap, CludgeBitMap, CopyBitMap, ScreensBitMap,
; ShapesBitMap, LoadBitMap (an ILBM written to RAM: first), with the
; object fields and checksums of the planes
DEFTYPE .l
BitMap 0,100,50,3
bn=0 : Gosub bdump
b=Addr BitMap(0)
For i=0 To 2
  pl=Peek.l(b+8+i*4)
  For k=0 To 13*50-1 Step 7 : Poke.b pl+k,i*40+k : Next
Next
bn=0 : Gosub bsum
CopyBitMap 0,1 : bn=1 : Gosub bdump : Gosub bsum
NPrint "copy own planes ",Peek.l(Addr BitMap(1)+8)<>Peek.l(Addr BitMap(0)+8)
Dim cm.l(200)
CludgeBitMap 2,40,20,2,&cm(0)
bn=2 : Gosub bdump
NPrint "cludge planes ",Peek.l(Addr BitMap(2)+8)-&cm(0)," ",Peek.l(Addr BitMap(2)+12)-&cm(0)
BitMap 2,17,3,1 : bn=2 : Gosub bdump
BitMap 2,64,8,8 : bn=2 : Gosub bdump
; a screen's bitmap
Screen 0,0,0,320,200,3,0,"t120",1,2
ScreensBitMap 0,3 : bn=3 : Gosub bdump
NPrint "screen bitmap ",Peek.l(Addr BitMap(3)+8)=Peek.l(Peek.l(Peek.l(Addr Screen(0))+88)+8)
; a shape from bitmap 0
Use BitMap 0
GetaShape 0,8,4,32,10
ShapesBitMap 0,4 : bn=4 : Gosub bdump
; ILBM 16x4x2, uncompressed, written to a file and loaded
Dim ib.l(30) : a=&ib(0)
Poke.l a,$464F524D : Poke.l a+4,76 : Poke.l a+8,$494C424D
Poke.l a+12,$424D4844 : Poke.l a+16,20
Poke.w a+20,16 : Poke.w a+22,4 : Poke.w a+24,0 : Poke.w a+26,0
Poke.b a+28,2 : Poke.b a+29,0 : Poke.b a+30,0 : Poke.b a+31,0 : Poke.w a+32,0
Poke.b a+34,10 : Poke.b a+35,11 : Poke.w a+36,16 : Poke.w a+38,4
Poke.l a+40,$434D4150 : Poke.l a+44,12
For i=0 To 11 : Poke.b a+48+i,i*20 : Next
Poke.l a+60,$424F4459 : Poke.l a+64,16
For i=0 To 15 : Poke.b a+68+i,i*17+3 : Next
f$="RAM:t120.iff"
If WriteFile(0,f$) : WriteMem 0,a,84 : CloseFile 0 : EndIf
BitMap 5,16,4,2
LoadBitMap 5,f$
bn=5 : Gosub bdump : Gosub bsum
BitMap 6,16,4,2
LoadBitMap 6,f$,0
bn=6 : Gosub bsum
p=Addr Palette(0) : NPrint "palette ",Peek.w(p+4)," ",Hex$(Peek.l(Peek.l(p)+4))," ",Hex$(Peek.l(Peek.l(p)+4+3*12+8))
KillFile f$
CloseScreen 0
End

bdump:
  b=Addr BitMap(bn)
  Print "bm ",bn," ",Peek.w(b)," ",Peek.w(b+2)," ",Peek.b(b+4)," ",Peek.b(b+5)," ",Hex$(Peek.w(b+6))
  NPrint " | ",Peek.b(b+52)&$80," ",Peek.w(b+56)," ",Peek.w(b+58)," ",Peek.w(b+60)," ",Peek.w(b+62)
  If Peek.w(b+62)<0 AND Peek.b(b+5)>1 Then NPrint "  dist ",Peek.w(b+54)," ",Peek.l(b+12)-Peek.l(b+8)
Return

bsum:
  b=Addr BitMap(bn) : s=0
  For i=0 To Peek.b(b+5)-1
    pl=Peek.l(b+8+i*4)
    For k=0 To Peek.w(b)*Peek.w(b+2)-1 : s=s*3+Peek.b(pl+k)+i : s=s&$FFFFFF : Next
  Next
  NPrint "  sum ",bn," ",s
Return

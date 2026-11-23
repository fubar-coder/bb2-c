; ripacklib-ilbmpalette-offset-ignored (#769) - ILBMPalette(source,
; palette#,offset) on a palette with fewer colours than offset + CMAP
; colours frees the palette's colours and makes a new block of the CMAP's
; size, the colours from 0 on: the offset is ignored, the old colours are
; lost.  Here palette 0 has 8 colours (PalRGB; colour 0 is 15,8,4,
; colour 5 1,2,3); the ILBM (built in memory) has a CMAP of 4 colours,
; loaded at offset 6.  A colour is read as red*65536+green*256+blue of
; its CMAP entry (-1 if the palette has no such colour).
; Correct: the palette grows to 10 colours and keeps colours 0 to 5;
; colours 6 to 9 come from the CMAP ($F0 $80 $10 ... $12 $34 $56).
; ripacklib needs RIAmosFuncLib (LoadIFF calls its NextBank, BLoad, ...).
DEFTYPE .l

!TestCase{offset_6}
  PalRGB 0,0,15,8,4 : PalRGB 0,5,1,2,3 : PalRGB 0,7,0,0,0
  m=AllocMem_(64,$10001)
  !Require{m<>0,"memory"}
  Poke.l m,$464F524D : Poke.l m+4,24 : Poke.l m+8,$494C424D
  Poke.l m+12,$434D4150 : Poke.l m+16,12
  Poke.l m+20,$F08010A0 : Poke.l m+24,$B0C02040 : Poke.l m+28,$60123456
  r=ILBMPalette(m,0,6)
  o=Addr Palette(0) : d=Peek.l(o) : n=Peek.w(o+4)
  c0=-1 : c5=-1 : c6=-1 : c7=-1 : c9=-1
  If n>0 Then e=d+4 : c0=(Peek.b(e) AND 255)*65536+(Peek.b(e+4) AND 255)*256+(Peek.b(e+8) AND 255)
  If n>5 Then e=d+4+5*12 : c5=(Peek.b(e) AND 255)*65536+(Peek.b(e+4) AND 255)*256+(Peek.b(e+8) AND 255)
  If n>6 Then e=d+4+6*12 : c6=(Peek.b(e) AND 255)*65536+(Peek.b(e+4) AND 255)*256+(Peek.b(e+8) AND 255)
  If n>7 Then e=d+4+7*12 : c7=(Peek.b(e) AND 255)*65536+(Peek.b(e+4) AND 255)*256+(Peek.b(e+8) AND 255)
  If n>9 Then e=d+4+9*12 : c9=(Peek.b(e) AND 255)*65536+(Peek.b(e+4) AND 255)*256+(Peek.b(e+8) AND 255)
  FreeMem_ m,64
  !CheckEq{r,-1,"result"}
  !CheckEq{n,10,"colours"}
  !CheckEq{c0,$FF8844,"colour_0"}
  !CheckEq{c5,$112233,"colour_5"}
  !CheckEq{c6,$F08010,"colour_6"}
  !CheckEq{c7,$A0B0C0,"colour_7"}
  !CheckEq{c9,$123456,"colour_9"}
!EndTestCase

; ripacklib-ilbmpalette-offset-ignored (#769) - ILBMPalette(source,
; palette#,offset) on a palette with fewer colours than offset + CMAP
; colours frees the palette's colours and makes a new block of the CMAP's
; size, the colours from 0 on: the offset is ignored, the old colours are
; lost.  Here palette 0 has 8 colours (PalRGB; colour 0 is 15,8,4,
; colour 5 1,2,3); the ILBM (built in memory) has a CMAP of 4 colours,
; loaded at offset 6.
; Correct: the palette grows to 10 colours and keeps colours 0 to 5;
; colours 6 to 9 come from the CMAP ($F0 $80 $10 ... $12 $34 $56).
; ripacklib needs RIAmosFuncLib (LoadIFF calls its NextBank, BLoad, ...).
; @fixed #769
; @rwe ripacklib riamosfunclib
DEFTYPE .l
PalRGB 0,0,15,8,4 : PalRGB 0,5,1,2,3 : PalRGB 0,7,0,0,0
m=AllocMem_(64,$10001)
If m=0 Then NPrint "no memory" : End
Poke.l m,$464F524D : Poke.l m+4,24 : Poke.l m+8,$494C424D
Poke.l m+12,$434D4150 : Poke.l m+16,12
Poke.l m+20,$F08010A0 : Poke.l m+24,$B0C02040 : Poke.l m+28,$60123456
r=ILBMPalette(m,0,6)
o=Addr Palette(0) : d=Peek.l(o) : n=Peek.w(o+4)
NPrint "result ",r," colours ",n
For i=0 To 9
  If i<n AND (i=0 OR i=5 OR i=6 OR i=7 OR i=9)
    e=d+4+i*12
    NPrint i," ",Hex$(Peek.b(e) AND 255)," ",Hex$(Peek.b(e+4) AND 255)," ",Hex$(Peek.b(e+8) AND 255)
  EndIf
Next
FreeMem_ m,64
End

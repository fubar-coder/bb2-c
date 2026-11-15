; palettelib-decodepalette-grow (#226) - DecodePalette Palette#,Memory,
; Offset on a palette too small for offset + colour count calls
; InitPalette, which frees the colour data and makes new, cleared data:
; the colours below the offset are lost (black).
; Here: a palette of 2 colours (PalRGB), then DecodePalette with offset 1
; of an ILBM in memory with a CMAP of 2 colours ($F0 $80 $10, $20 $40
; $60): the palette needs 3 colours.
; Correct: the palette grows (as PalRGB does) and keeps colour 0 (red 15,
; green 8, blue 4 as PalRGB set it); colours 1 and 2 come from the CMAP.
; @fixed #226
; @configs clib
DEFTYPE .l
PalRGB 0,0,15,8,4 : PalRGB 0,1,1,2,3
Dim mm.l(12) : a=&mm(0)
Poke.l a,$464F524D : Poke.l a+4,18 : Poke.l a+8,$494C424D
Poke.l a+12,$434D4150 : Poke.l a+16,6
Poke.b a+20,$F0 : Poke.b a+21,$80 : Poke.b a+22,$10
Poke.b a+23,$20 : Poke.b a+24,$40 : Poke.b a+25,$60
DecodePalette 0,a,1
o=Addr Palette(0) : d=Peek.l(o)
NPrint "colours ",Peek.w(o+4)
For i=0 To 2
  e=d+4+i*12
  NPrint i," ",Hex$(Peek.b(e) AND 255)," ",Hex$(Peek.b(e+4) AND 255)," ",Hex$(Peek.b(e+8) AND 255)
Next
End

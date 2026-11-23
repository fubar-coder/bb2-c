; palettelib-decodepalette-grow (#226) - DecodePalette Palette#,Memory,
; Offset on a palette too small for offset + colour count calls
; InitPalette, which frees the colour data and makes new, cleared data:
; the colours below the offset are lost (black).
; Here: a palette of 2 colours (PalRGB), then DecodePalette with offset 1
; of an ILBM in memory with a CMAP of 2 colours ($F0 $80 $10, $20 $40
; $60): the palette needs 3 colours.
; Correct: the palette grows (as PalRGB does) and keeps colour 0 (red 15,
; green 8, blue 4 as PalRGB set it); colours 1 and 2 come from the CMAP.
DEFTYPE .l

; the byte at a as two hex digits
Function$ hb{a.l}
  Function Return Right$(Hex$(Peek.b(a) AND 255),2)
End Function

; the three components of colour i of the palette data d as hex bytes
Function$ rgbhex{d.l,i.l}
  e=d+4+i*12
  Function Return hb{e}+" "+hb{e+4}+" "+hb{e+8}
End Function

!TestCase{grow}
  PalRGB 0,0,15,8,4 : PalRGB 0,1,1,2,3
  Dim mm.l(12) : a=&mm(0)
  Poke.l a,$464F524D : Poke.l a+4,18 : Poke.l a+8,$494C424D
  Poke.l a+12,$434D4150 : Poke.l a+16,6
  Poke.b a+20,$F0 : Poke.b a+21,$80 : Poke.b a+22,$10
  Poke.b a+23,$20 : Poke.b a+24,$40 : Poke.b a+25,$60
  DecodePalette 0,a,1
  o=Addr Palette(0) : d=Peek.l(o)
  !CheckEq{Peek.w(o+4),3,"colours"}
  !CheckStrEq{rgbhex{d,0},"FF 88 44","colour_0"}
  !CheckStrEq{rgbhex{d,1},"F0 80 10","colour_1"}
  !CheckStrEq{rgbhex{d,2},"20 40 60","colour_2"}
!EndTestCase

; palettelib-paletterange-one-colour (#227) - PaletteRange computes the
; step of each component with DIVS by EndCol - StartCol: with StartCol =
; EndCol (a range of one colour) it divides by 0 (Guru 8000 0005).
; Here: PaletteRange 0,3,3,15,0,0,0,0,15 on a palette of 8 colours.
; Correct: colour 3 gets the start colour (15, 0, 0: top bytes $0F 0 0),
; the program goes on.
; @fixed #227
; @configs clib
DEFTYPE .l
InitPalette 0,8
PaletteRange 0,3,3,15,0,0,0,0,15
d=Peek.l(Addr Palette(0)) : e=d+4+3*12
NPrint "3 ",Hex$(Peek.b(e) AND 255)," ",Hex$(Peek.b(e+4) AND 255)," ",Hex$(Peek.b(e+8) AND 255)
NPrint "went on"
End

; a child of palettelib-make4bitrgbs-lowbyte, started with exec's version
; set to 37: it opens a screen, decodes a CMAP (bytes i*21+5) and shows it,
; and prints Red/Green/Blue (GetRGB4) of colours 1-3
DEFTYPE .l
NPrint "exec ",ExecVersion
Screen 0,0,0,320,200,3,0,"bo224",1,2
Dim mm.l(20) : a=&mm(0)
Poke.l a,$464F524D : Poke.l a+4,24 : Poke.l a+8,$494C424D
Poke.l a+12,$434D4150 : Poke.l a+16,12
For i=0 To 11 : Poke.b a+20+i,i*21+5 : Next
DecodePalette 2,a : ShowPalette 2
Print "cols"
For i=1 To 3 : Print " ",Red(i),",",Green(i),",",Blue(i) : Next
NPrint ""
CloseScreen 0
End

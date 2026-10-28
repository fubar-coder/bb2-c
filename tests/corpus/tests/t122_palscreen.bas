; t122_palscreen - palettelib in Amiga mode on an own screen: RGB/Red/
; Green/Blue (SetRGB4/GetRGB4), AGARGB/AGARed/AGAGreen/AGABlue,
; ShowPalette and Use Palette (NewPaletteMode), DecodePalette colours
; (top byte only) shown; Cycle/StopCycle only called (timing dependent)
DEFTYPE .l
Screen 0,0,0,320,200,3,0,"t122",1,2
RGB 1,15,8,4 : RGB 2,0,1,2
NPrint "rgb ",Red(1)," ",Green(1)," ",Blue(1)," ",Red(2)," ",Green(2)," ",Blue(2)
; diagnostic: GetRGB32 of colour 1 directly (inline), then the lib
Dim tb.l(3) : Dim nm.l(8) : Poke.s &nm(0),"graphics.library"
GetReg d0,&nm(0) : MOVE.l d0,a1 : MOVE.l 4,a6 : JSR -408(a6) : PutReg d0,gfx
cm=Peek.l(Peek.l(Addr Screen(0))+48)
GetReg d0,gfx : MOVE.l d0,a6
GetReg d0,cm : MOVE.l d0,a0
GetReg d0,&tb(0) : MOVE.l d0,a1
MOVEQ #1,d0 : MOVEQ #1,d1 : JSR -900(a6)
NPrint "raw ",Hex$(tb(0))," ",Hex$(tb(1))," ",Hex$(tb(2))
NPrint "aga ",AGARed(1)," ",AGAGreen(1)," ",AGABlue(1)
AGARGB 3,200,100,50
NPrint "agargb ",AGARed(3)," ",AGAGreen(3)," ",AGABlue(3)," ",Red(3)," ",Green(3)," ",Blue(3)
For i=0 To 7 : PalRGB 0,i,i*2,15-i*2,i : Next
AGAPalRGB 0,5,201,99,17
ShowPalette 0 : Gosub cols
PalRGB 1,0,15,15,15 : PalRGB 1,1,1,2,3
NewPaletteMode On : Use Palette 1 : Gosub cols
NewPaletteMode Off : Use Palette 1 : Gosub cols
Dim mm.l(20) : a=&mm(0)
Poke.l a,$464F524D : Poke.l a+4,24 : Poke.l a+8,$494C424D
Poke.l a+12,$434D4150 : Poke.l a+16,12
For i=0 To 11 : Poke.b a+20+i,i*21+5 : Next
DecodePalette 2,a : ShowPalette 2 : Gosub cols
SetCycle 2,0,1,3,1
Cycle 2 : StopCycle
NPrint "cycle called"
CloseScreen 0
End

cols:
  Print "cols"
  For i=0 To 7 : Print " ",Red(i),",",Green(i),",",Blue(i),"/",AGARed(i),",",AGAGreen(i),",",AGABlue(i) : Next
  NPrint ""
Return

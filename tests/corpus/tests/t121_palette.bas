; t121_palette - palettelib palette objects (no display): PalRGB,
; AGAPalRGB (growing), InitPalette, PaletteRange, DuplicatePalette,
; MixPalette, FadePalette, SetCycle, CyclePalette, DecodePalette (ILBM in
; memory: CMAP + CRNG), LoadPalette (the same ILBM as a file, with and
; without offset); every palette printed as data words/longs.  SetCycle
; on an empty palette first: only that writes the end marker of the cycles
; (InitPalette does not), CyclePalette would run over the 14 entries.
DEFTYPE .l
PalRGB 0,0,1,2,3 : PalRGB 0,1,15,8,4 : PalRGB 0,3,7,0,15
pn=0 : Gosub pdump
AGAPalRGB 0,5,200,100,50 : AGAPalRGB 0,1,255,128,1
pn=0 : Gosub pdump
SetCycle 1,0,1,4
InitPalette 1,6
PaletteRange 1,0,5,0,0,0,250,100,-5
pn=1 : Gosub pdump
PaletteRange 1,1,3,10,20,30,40,50,60
pn=1 : Gosub pdump
DuplicatePalette 0,2 : pn=2 : Gosub pdump
MixPalette 0,1,2,0.25 : pn=2 : Gosub pdump
FadePalette 0,2,0.5 : pn=2 : Gosub pdump
FadePalette 0,2,2 : pn=2 : Gosub pdump
; cycles
SetCycle 1,1,5,2,-0.5 : SetCycle 1,0,0,2,0.25
pn=1 : Gosub pdump
For k=1 To 5 : CyclePalette 1 : Next
pn=1 : Gosub pdump
For k=1 To 3 : CyclePalette 1 : Next
pn=1 : Gosub pdump
; DecodePalette: FORM ILBM, a chunk, CMAP 4 colours, 3 CRNG
Dim mm.l(40) : a=&mm(0)
Poke.l a,$464F524D : Poke.l a+4,84 : Poke.l a+8,$494C424D
Poke.l a+12,$414E4E4F : Poke.l a+16,4 : Poke.l a+20,$41424344
Poke.l a+24,$434D4150 : Poke.l a+28,12
For i=0 To 11 : Poke.b a+32+i,i*21+5 : Next
Poke.l a+44,$43524E47 : Poke.l a+48,8 : Poke.w a+52,0 : Poke.w a+54,4096 : Poke.w a+56,1 : Poke.b a+58,1 : Poke.b a+59,3
Poke.l a+60,$43524E47 : Poke.l a+64,8 : Poke.w a+68,0 : Poke.w a+70,2048 : Poke.w a+72,0 : Poke.b a+74,0 : Poke.b a+75,2
Poke.l a+76,$43524E47 : Poke.l a+80,8 : Poke.w a+84,0 : Poke.w a+86,1000 : Poke.w a+88,1 : Poke.b a+90,2 : Poke.b a+91,2
DecodePalette 3,a : pn=3 : Gosub pdump
DecodePalette 3,a,2 : pn=3 : Gosub pdump
DecodePalette 2,a : pn=2 : Gosub pdump
; LoadPalette from a file with the same ILBM
f$="RAM:t121.iff"
If WriteFile(0,f$) : WriteMem 0,a,92 : CloseFile 0 : EndIf
Free Palette 3
LoadPalette 3,f$ : pn=3 : Gosub pdump
LoadPalette 3,f$,6 : pn=3 : Gosub pdump
KillFile f$
End

pdump:
  p=Addr Palette(pn) : d=Peek.l(p) : n=Peek.w(p+4)
  Print "pal ",pn," n ",n
  If d<>0 Then NPrint " hdr ",Peek.w(d)," ",Peek.w(d+2) Else NPrint " none"
  For i=0 To n-1
    e=d+4+i*12
    NPrint "  ",i," ",Hex$(Peek.l(e))," ",Hex$(Peek.l(e+4))," ",Hex$(Peek.l(e+8))
  Next
  If d<>0 Then NPrint "  end ",Hex$(Peek.l(d+4+n*12))
  For c=0 To 13
    lo=Peek.w(p+6+c*8)
    If lo<0 Then c=14 Else NPrint "  cyc ",c," ",lo," ",Peek.w(p+8+c*8)," ",Peek.w(p+10+c*8)," ",Peek.w(p+12+c*8)
  Next
Return

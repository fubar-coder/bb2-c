; t123_blitzfade - fadelib and the Blitz mode variants of palettelib on a
; slice: RGB/Red/Green/Blue (colour moves), ShowPalette, Use Palette,
; FadeOut/FadeIn stepped with ASyncFade On + DoFade (no timing), FadeStatus;
; values collected in Blitz mode, printed after AMIGA
DEFTYPE .l
Dim v.l(400) : vi=0
For i=0 To 7 : PalRGB 0,i,i*2,15-i*2,i : Next
PalRGB 1,0,15,15,15 : PalRGB 1,1,1,2,3
BLITZ
Slice 0,44,320,200,$fff8,3,8,8,320,320
sl=Addr Slice(0) : cp=Peek.l(sl+18)
v(vi)=Peek.w(sl+8) : vi=vi+1
RGB 2,1,2,3
v(vi)=Red(2) : v(vi+1)=Green(2) : v(vi+2)=Blue(2) : vi=vi+3
ShowPalette 0 : Gosub grab
NewPaletteMode On : Use Palette 1 : Gosub grab
NewPaletteMode Off : Use Palette 1 : Gosub grab
ShowPalette 0
ASyncFade On
FadeOut 0
v(vi)=FadeStatus : vi=vi+1
For k=1 To 17 : DoFade : Gosub grab : Next
v(vi)=FadeStatus : vi=vi+1
FadeIn 0,1,2,5
For k=1 To 16 : DoFade : Gosub grab : Next
v(vi)=FadeStatus : vi=vi+1
FreeSlices
AMIGA
NPrint "colours ",v(0)," rgb ",v(1)," ",v(2)," ",v(3)
k=4
For j=1 To 3 : Gosub showv : Next
NPrint "status ",v(k) : k=k+1
For j=1 To 17 : Gosub showv : Next
NPrint "status ",v(k) : k=k+1
For j=1 To 16 : Gosub showv : Next
NPrint "status ",v(k)
End

grab:
  For i=0 To 7 : v(vi)=Peek.w(cp+2+i*4) : vi=vi+1 : Next
Return

showv:
  Print j,":"
  For i=0 To 7 : Print " ",Hex$(v(k)) : k=k+1 : Next
  NPrint ""
Return

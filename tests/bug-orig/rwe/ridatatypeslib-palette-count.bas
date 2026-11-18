; ridatatypeslib-palette-count (#606) - DataTypesPalette copies as many
; colours as the palette object has, not as many as the picture has
; (PDTA_NumColors): a larger palette gets the bytes behind the picture's
; colour table as its further colours.  The picture (generated) has four
; colours; the palette has eight, colours 4-7 set to markers (PalRGB).
; The program reads the colour block of the palette (Addr Palette: the
; block at +0; per colour a long each for red, green and blue, the value
; in the top byte, behind the count and a word).
; Correct: colours 0-3 are the picture's (colour 1 red $F0), colours 4-7
; keep their markers.
; @fixed #606
; @rwe ridatatypeslib
; @pass file=p.ilbm|@TESTDATA@/ridatatypeslib-palette-count.ilbm
DEFTYPE .l
InitPalette 0,8
For c=4 To 7 : PalRGB 0,c,c,15-c,c+2 : Next
d=Peek.l(Addr Palette(0))
Dim m.b(47)
For i=0 To 47 : m(i)=Peek.b(d+52+i) : Next
If DataTypeOpen("CORPUS:p.ilbm")
  o=DataTypeCreate
  NPrint "object ",o<>0
  If o
    r=DataTypesPalette(0)
    NPrint "result ",r<>0
    NPrint "colour 1 red ",Hex$(Peek.b(d+16) AND 255)
    k=-1
    For i=0 To 47
      If Peek.b(d+52+i)<>m(i) Then k=0
    Next
    NPrint "colours 4-7 kept ",k
  EndIf
  DataTypeClose
Else
  NPrint "not opened"
EndIf
End

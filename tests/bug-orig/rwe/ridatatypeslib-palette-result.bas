; ridatatypeslib-palette-result (#607) - DataTypesPalette asks GetAttr
; for PDTA_ColorRegisters without clearing the library's buffer first,
; and takes its first long as the colour table: for an object that is
; not a picture (a text file, text.datatype) it is what the command
; before left there.  SetDataTypeAttr with a pair leaves its attribute
; there; the attribute is the address of an array of bytes $5A (an
; attribute the class does not know, ignored).  The program reads the
; red value of colour 0 of a palette of four colours (Addr Palette: the
; colour block at +0, colour 0's red long behind the count and a word).
; Correct: no colour table - DataTypesPalette returns 0 and the palette
; keeps its colours.
; @fixed #607
; @rwe ridatatypeslib
; @pass file=t.txt|@TESTDATA@/ridatatypeslib-palette-result.txt
DEFTYPE .l
InitPalette 0,4
PalRGB 0,0,1,2,3
d=Peek.l(Addr Palette(0))
r0=Peek.b(d+4)
Dim b.b(15)
For i=0 To 15 : b(i)=$5A : Next
If DataTypeOpen("CORPUS:t.txt")
  o=DataTypeCreate
  NPrint "object ",o<>0
  If o
    SetDataTypeAttr &b(0),0
    NPrint "result ",DataTypesPalette(0)<>0
    NPrint "colour 0 kept ",Peek.b(d+4)=r0
  EndIf
  DataTypeClose
Else
  NPrint "not opened"
EndIf
End

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
DEFTYPE .l

!TestCase{text_object}
  InitPalette 0,4
  PalRGB 0,0,1,2,3
  d=Peek.l(Addr Palette(0))
  r0=Peek.b(d+4)
  Dim b.b(15)
  For i=0 To 15 : b(i)=$5A : Next
  opened=DataTypeOpen("CORPUS:t.txt")
  !Require{opened<>0,"opened"}
  o=DataTypeCreate
  !Require{o<>0,"object"}
  SetDataTypeAttr &b(0),0
  r=DataTypesPalette(0)
  res=(r<>0)
  !CheckEq{res,0,"result"}
  kept=(Peek.b(d+4)=r0)
  !CheckEq{kept,-1,"colour0_kept"}
  DataTypeClose
!EndTestCase

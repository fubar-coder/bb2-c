; rishapeslib-lecludgeshapes-odd-cmap-68000 (#1118) - LESaveShapes writes
; the CMAP chunk as colours x 3 bytes without the IFF pad byte: with a
; palette of 3 colours the CMAP is 9 bytes and the SDAT chunk behind it
; starts at an odd offset of the file.  LECludgeShapes then reads the
; SDAT id and the shape headers at odd addresses, and links the shape
; data of an uncompressed file there.  On a 68000 a word or long read at
; an odd address is an address error (the session ends in a timeout).
; The machine is the A600 (68000).  Shape 0 (16 x 8, one plane, the left
; half set) is saved with palette 0 (3 colours) to CORPUS:, the file read
; into memory and cludged into shape 1 with palette 1.
; Correct: shape 1 is 16 x 8 with the shape's data (8 bytes $FF), its data
; at an even address (copied); palette 1 has the 3 colours.  The original
; ends in an address error in LECludgeShapes.
; rishapeslib needs RIPackLib (PackPBM, UnPackPBM), which needs
; RIAmosFuncLib (banks).
DEFTYPE .l

!TestCase{odd_cmap}
  InitPalette 0,3
  PalRGB 0,2,15,8,1
  InitPalette 1,2
  BitMap 0,32,16,1
  Boxf 0,0,7,7,1
  GetaShape 0,0,0,16,8
  LESaveShapes 0,0,"CORPUS:o.le",0
  m=AllocMem_(1024,$10001)
  !Require{m<>0,"memory"}
  n$="CORPUS:o.le"
  f=Open_(&n$,1005)
  !Require{f<>0,"file"}
  r=Read_(f,m,1024)
  Close_ f
  cm=Peek.l(m+44)
  !CheckEq{r,93,"file_size"}
  !CheckEq{cm,9,"cmap_size"}
  LECludgeShapes 1,1,m,1
  w=ShapeWidth(1) : h=ShapeHeight(1)
  a=Peek.l(Addr Shape(1)+14)
  s=0
  For i=0 To 15 : s=s+(Peek.b(a+i) & 255) : Next
  ev=1-(a & 1)
  p=Peek.l(Addr Palette(1))
  pc=Peek.w(p) : pl$=Hex$(Peek.l(p+28))
  !CheckEq{w,16,"width"}
  !CheckEq{h,8,"height"}
  !CheckEq{s,2040,"data_sum"}
  !CheckEq{ev,1,"data_even"}
  !CheckEq{pc,3,"palette_colours"}
  !CheckStrEq{pl$,"FFFFFFFF","palette_last"}
!EndTestCase

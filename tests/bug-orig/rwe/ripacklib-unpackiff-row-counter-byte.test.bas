; ripacklib-unpackiff-row-counter-byte (#774) - UnpackIFF counts the bytes
; of a row in a signed byte: rows of 129 bytes and more are "done" before
; anything is read.  The ILBM (built in memory) is 1040x2, one plane,
; ByteRun1, 130 bytes a row: row 0 is 130 bytes of $FF, row 1 128 bytes
; of $00 and 2 bytes of $0F.  The original unpacks nothing.
; Correct: pixel 0 and 1039 of row 0 have colour 1; of row 1 pixel 0 has
; colour 0, pixel 1039 colour 1.
; ripacklib needs RIAmosFuncLib (LoadIFF calls its NextBank, BLoad, ...).
DEFTYPE .l

!TestCase{wide_rows}
  m=AllocMem_(6000,$10001)
  !Require{m<>0,"memory"}
  Poke.l m,$464F524D : Poke.l m+4,48 : Poke.l m+8,$494C424D
  Poke.l m+12,$424D4844 : Poke.l m+16,20
  Poke.w m+20,1040 : Poke.w m+22,2 : Poke.b m+28,1 : Poke.b m+30,1
  Poke.l m+40,$424F4459 : Poke.l m+44,8
  Poke.l m+48,$81FFFFFF : Poke.l m+52,$8100FF0F
  BitMap 0,1040,2,1
  r=UnpackIFF(m,0)
  x0=Point(0,0) : x1039=Point(1039,0) : y0=Point(0,1) : y1039=Point(1039,1)
  FreeMem_ m,6000
  !CheckEq{r,-1,"result"}
  !CheckEq{x0,1,"row_0_pixel_0"}
  !CheckEq{x1039,1,"row_0_pixel_1039"}
  !CheckEq{y0,0,"row_1_pixel_0"}
  !CheckEq{y1039,1,"row_1_pixel_1039"}
!EndTestCase

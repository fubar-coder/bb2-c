; ripacklib-unpackiff-mask-rows (#1112) - UnpackIFF ignores the BMHD's
; masking byte: with masking 1 (mskHasMask) every line of the BODY has
; one mask row after its plane rows, which the library decodes as the
; next line's plane.  The ILBM (built in memory) is 16x3, one plane,
; masked, ByteRun1: line 0 $80,$00, line 1 $00,$80, line 2 $80,$80, each
; followed by the mask row $FF,$FF (a repeat run).  Ignoring the mask,
; line 1 becomes the mask row of line 0 and line 2 the plane row of
; line 1.
; Correct: the mask rows are skipped; pixels 0 and 8 of the lines are
; 1 0, 0 1 and 1 1.
; ripacklib needs RIAmosFuncLib (LoadIFF calls its NextBank, BLoad, ...).
DEFTYPE .l

!TestCase{masked_lines}
  m=AllocMem_(256,$10001)
  !Require{m<>0,"memory"}
  Poke.l m,$464F524D : Poke.l m+4,56 : Poke.l m+8,$494C424D
  Poke.l m+12,$424D4844 : Poke.l m+16,20
  Poke.w m+20,16 : Poke.w m+22,3 : Poke.b m+28,1 : Poke.b m+29,1 : Poke.b m+30,1
  Poke.l m+40,$424F4459 : Poke.l m+44,15
  Poke.b m+48,$01 : Poke.b m+49,$80 : Poke.b m+50,$00 : Poke.b m+51,$FF : Poke.b m+52,$FF
  Poke.b m+53,$01 : Poke.b m+54,$00 : Poke.b m+55,$80 : Poke.b m+56,$FF : Poke.b m+57,$FF
  Poke.b m+58,$01 : Poke.b m+59,$80 : Poke.b m+60,$80 : Poke.b m+61,$FF : Poke.b m+62,$FF
  BitMap 0,320,16,1
  r=UnpackIFF(m,0)
  x0=Point(0,0) : x8=Point(8,0)
  y0=Point(0,1) : y8=Point(8,1)
  c0=Point(0,2) : c8=Point(8,2)
  FreeMem_ m,256
  !CheckEq{x0,1,"line_0_pixel_0"}
  !CheckEq{x8,0,"line_0_pixel_8"}
  !CheckEq{y0,0,"line_1_pixel_0"}
  !CheckEq{y8,1,"line_1_pixel_8"}
  !CheckEq{c0,1,"line_2_pixel_0"}
  !CheckEq{c8,1,"line_2_pixel_8"}
!EndTestCase

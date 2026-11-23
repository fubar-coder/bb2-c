; rimapeditorlib-dbra-size0 (#895) - MapDataGrab, ShapeTo1616 and
; FillBitplane use a size - 1 as a DBRA counter without testing for 0:
; FillBitplane with a width below 16 writes width / 16 - 1 = $FFFF + 1
; words per row (128 KB).  Made safe: bitmap 5 is set up by hand (40
; bytes per row) with its plane 0 a block of the program's own of
; 256 KB; one row is filled, a marker 100000 bytes into the block shows
; the overrun.  The object is cleared again afterwards.  The width and
; the height are arguments of the programmer; the original has no error
; check area.  Here with runtime error checking.
; Correct: the FillBitplane of width 16 works; the error check refuses
; the width of 8: "FillBitplane width must be at least 16", nothing is
; written.
DEFTYPE .l

!TestCase{fill_width_8}
  b=AllocMem_(262144,$10001)
  !Require{b<>0,"allocated"}
  Poke.l b+100000,$12345678
  o=Addr BitMap(5)
  Poke.w o,40 : Poke.w o+2,1 : Poke.w o+4,1 : Poke.l o+8,b
  FillBitplane 5,16,1
  !ExpectError{"FillBitplane width must be at least 16","refused"}
  FillBitplane 5,8,1
  !Check{0,"not_refused"}
  v=Peek.l(b+100000)
  Poke.w o,0 : Poke.w o+2,0 : Poke.w o+4,0 : Poke.l o+8,0
  FreeMem_ b,262144
  !CheckEq{v,$12345678,"marker_kept"}
!EndTestCase

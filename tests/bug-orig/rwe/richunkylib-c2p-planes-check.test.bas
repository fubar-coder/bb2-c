; richunkylib-c2p-planes-check (#884) - ChunkyToPlanar writes plane n
; at plane 0 + n * 10240 and does not read the bitmap's other plane
; pointers; the conversion of other sizes takes the plane stride from the
; pointers and needs all planes equally spaced.  Here: bitmap 1 is 320 x
; 256 x 8 (CludgeBitMap, contiguous planes), converted once as it is;
; then its plane 7 pointer is set to a buffer of its own (a Poke into the
; bitmap) and it is converted again.  The original has no error check
; area.  Here with runtime error checking.
; Correct: the first conversion is done, the error check refuses the
; second: "Bitmap planes not equally spaced", the buffer stays as it is.
DEFTYPE .l

!TestCase{spacing}
  m=AllocMem_(81920,$10002)
  b=AllocMem_(10240,$10001)
  !Require{m<>0 AND b<>0,"memory"}
  CludgeBitMap 1,320,256,8,m
  ChunkyBitmap 0,320,256
  Poke.b ChunkyData(0),$FF
  ChunkyToPlanar 0,1
  !CheckEq{Peek.b(m+7*10240)&255,128,"contiguous"}
  bm=Addr BitMap(1)
  op=Peek.l(bm+8+28)
  Poke.l bm+8+28,b
  !ExpectError{"Bitmap planes not equally spaced","refused"}
  ChunkyToPlanar 0,1
  Poke.l bm+8+28,op
  !CheckEq{Peek.b(b)&255,0,"buffer_kept"}
  FreeMem_ b,10240
  FreeMem_ m,81920
!EndTestCase

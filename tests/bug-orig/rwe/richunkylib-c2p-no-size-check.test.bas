; richunkylib-c2p-no-size-check (#884) - ChunkyToPlanar always writes 8
; planes of 10240 bytes (320 x 256) from the bitmap's first plane on,
; whatever the bitmap's size.  Made safe: bitmap 1 is 320 x 200 x 8
; (CludgeBitMap, 8 contiguous planes of 8000 bytes = 64000 bytes) inside
; an allocation of 90000 bytes; a marker long at 80000, behind the
; bitmap's planes but inside the allocation, shows the overwrite.  The
; chunky bitmap has the size the command expects (320 x 256).
; Correct: the conversion stays inside the bitmap (or is refused), the
; marker stays.
DEFTYPE .l

!TestCase{marker}
  m=AllocMem_(90000,$10002)
  !Require{m<>0,"memory"}
  CludgeBitMap 1,320,200,8,m
  ChunkyBitmap 0,320,256
  Poke.l m+80000,$12345678
  ChunkyToPlanar 0,1
  !CheckEq{Peek.l(m+80000),$12345678,"marker_kept"}
  FreeMem_ m,90000
!EndTestCase

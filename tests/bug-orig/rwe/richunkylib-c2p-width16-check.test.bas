; richunkylib-c2p-width16-check (#884) - ChunkyToPlanar converts 320 x
; 256 pixels whatever the chunky bitmap's size; the conversion of other
; sizes takes widths that are a multiple of 16 (32-pixel steps and a
; 16-pixel tail per row).  Made safe: the bitmap is 320 x 256 x 8
; (CludgeBitMap, 81920 bytes of the program), so the original's writes
; stay inside it (its reads behind the small chunky data only read).
; First a chunky bitmap of 48 x 10 (a tail of 16), then one of 40 x 10.
; The original has no error check area.  Here with runtime error
; checking.
; Correct: 48 x 10 is converted (row 1 of the chunky data in row 1 of
; the bitmap), the error check refuses 40 x 10: "Chunky bitmap width not
; a multiple of 16".
DEFTYPE .l

!TestCase{widths}
  m=AllocMem_(81920,$10002)
  !Require{m<>0,"memory"}
  CludgeBitMap 1,320,256,8,m
  ChunkyBitmap 0,48,10
  Poke.b ChunkyData(0)+48,$FF
  ChunkyToPlanar 0,1
  !CheckEq{Peek.b(m+40)&255,128,"row1_plane0"}
  ChunkyBitmap 0,40,10
  !ExpectError{"Chunky bitmap width not a multiple of 16","refused"}
  ChunkyToPlanar 0,1
  FreeMem_ m,81920
!EndTestCase

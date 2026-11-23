; bitmaplib-copybitmap-zero (#78) - CopyBitMap copies (bytes per row *
; rows) / 2 words per plane with SUBQ.l #1:BNE: a source bitmap of width
; or height 0 (no words) runs the loop 2^32 times through the memory.
; Such a bitmap is a programming error, and BitMap's error check tests
; only the depth, not the size, even with runtime error checking.
; Here with runtime error checking: BitMap 0,16,10,1 (no error), then
; BitMap 1,0,10,1 (width 0); CopyBitMap is not called (the original would
; overwrite the memory).
; Correct: the error check refuses the size: "Illegal BitMap Size".  The
; error is announced with !ExpectError; the original raises none (see the
; configuration).
!TestCase{width_zero}
  BitMap 0,16,10,1
  !CheckEq{Peek.w(Addr BitMap(0)+2),10,"rows_10"}
  !ExpectError{"Illegal BitMap Size","refused"}
  BitMap 1,0,10,1
  !Check{0,"after"}
!EndTestCase

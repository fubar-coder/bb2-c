; bitmaplib-cludgebitmap-size-check (#78) - a bitmap of width or height 0
; has planes of 0 bytes, and CopyBitMap of it copies 2^32 words (the loop
; counts (bytes per row * rows) / 2 down with SUBQ.l #1:BNE).  Such a
; bitmap is a programming error; CludgeBitMap's error check (cludgechk,
; #393) tests only the depth, the original has none at all.
; Here with runtime error checking: CludgeBitMap with 16 x 10 (no error),
; then with height 0.
; Correct: the error check refuses the size: "Illegal BitMap Size".  The
; message is part of the tested behaviour, so the error is not announced
; with !ExpectError (see the configuration).
DEFTYPE .l

!TestCase{height_zero}
  m=AllocMem(1000,$10002)
  !Require{m<>0,"memory"}
  CludgeBitMap 0,16,10,1,m
  !CheckEq{Peek.w(Addr BitMap(0)+2),10,"rows_10"}
  CludgeBitMap 1,16,0,1,m
  !Check{0,"after"}
  FreeMem m,1000
!EndTestCase

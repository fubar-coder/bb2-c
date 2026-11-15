; bitmaplib-cludgebitmap-size-check (#78) - a bitmap of width or height 0
; has planes of 0 bytes, and CopyBitMap of it copies 2^32 words (the loop
; counts (bytes per row * rows) / 2 down with SUBQ.l #1:BNE).  Such a
; bitmap is a programming error; CludgeBitMap's error check (cludgechk,
; #393) tests only the depth, the original has none at all.
; Here with runtime error checking: CludgeBitMap with 16 x 10 (no error),
; then with height 0.
; Correct: the error check refuses the size: "Illegal BitMap Size".
; @fixed #78
; @configs clib
; @pass runtime_errors=on
DEFTYPE .l
m=AllocMem(1000,$10002)
CludgeBitMap 0,16,10,1,m
NPrint "16 x 10"
CludgeBitMap 1,16,0,1,m
NPrint "16 x 0"
FreeMem m,1000
End

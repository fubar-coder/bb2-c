; bitmaplib-copybitmap-zero (#78) - CopyBitMap copies (bytes per row *
; rows) / 2 words per plane with SUBQ.l #1:BNE: a source bitmap of width
; or height 0 (no words) runs the loop 2^32 times through the memory.
; Such a bitmap is a programming error, and BitMap's error check tests
; only the depth, not the size, even with runtime error checking.
; Here with runtime error checking: BitMap 0,16,10,1 (no error), then
; BitMap 1,0,10,1 (width 0); CopyBitMap is not called (the original would
; overwrite the memory).
; Correct: the error check refuses the size: "Illegal BitMap Size".
; @fixed #78
; @configs clib
; @pass runtime_errors=on
BitMap 0,16,10,1
NPrint "16 x 10"
BitMap 1,0,10,1
NPrint "0 x 10"
End

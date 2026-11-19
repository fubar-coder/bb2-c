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
; @rwe richunkylib
; @pass timeout=30
; @pass runtime_errors=on
DEFTYPE .l
m=AllocMem_(81920,$10002)
If m=0 Then NPrint "no memory" : End
CludgeBitMap 1,320,256,8,m
ChunkyBitmap 0,48,10
Poke.b ChunkyData(0)+48,$FF
ChunkyToPlanar 0,1
NPrint "48x10: row 1 of plane 0 ",Peek.b(m+40)&255
ChunkyBitmap 0,40,10
ChunkyToPlanar 0,1
NPrint "40x10: converted"
FreeMem_ m,81920
End

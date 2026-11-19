; richunkylib-c2p-larger-check (#884) - ChunkyToPlanar always writes 8
; planes of 10240 bytes (320 x 256) from the bitmap's first plane on,
; whatever the sizes of the chunky bitmap and the bitmap.  Made safe as
; richunkylib-c2p-no-size-check: bitmap 1 is 320 x 200 x 8 (CludgeBitMap,
; 64000 bytes) inside an allocation of 90000 bytes, a marker long at
; 80000.  First a chunky bitmap of the bitmap's size (320 x 200), then
; one of 320 x 256.  The original has no error check area.  Here with
; runtime error checking.
; Correct: the 320 x 200 one is converted, the error check refuses the
; 320 x 256 one: "Chunky bitmap larger than the bitmap", the marker stays.
; @fixed #884
; @rwe richunkylib
; @pass timeout=30
; @pass runtime_errors=on
DEFTYPE .l
m=AllocMem_(90000,$10002)
If m=0 Then NPrint "no memory" : End
CludgeBitMap 1,320,200,8,m
ChunkyBitmap 0,320,200
Poke.b ChunkyData(0),$FF
ChunkyToPlanar 0,1
NPrint "320x200: plane 0 ",Peek.b(m)&255
ChunkyBitmap 0,320,256
Poke.l m+80000,$12345678
ChunkyToPlanar 0,1
If Peek.l(m+80000)=$12345678 Then NPrint "marker kept" Else NPrint "marker overwritten"
FreeMem_ m,90000
End

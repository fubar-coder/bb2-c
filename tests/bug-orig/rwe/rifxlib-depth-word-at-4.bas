; rifxlib-depth-word-at-4 (#670) - ClearBitmap (also FadeInBitmap and
; ZoomyZoom) reads the depth as the word at offset 4 of the BitMap,
; Flags * 256 + Depth.  Bitmap 0 (one plane) gets the Flags byte 1
; (BMF_CLEAR): ClearBitmap then works on 257 planes, through plane
; pointers far behind the BitMap and over its 16-entry pointer arrays.
; Correct: the depth is the byte at 5, one plane is cleared, the program
; goes on.
; @rwe rifxlib
BitMap 0,320,16,1
Poke.b Addr BitMap(0)+4,1
NPrint "before"
ClearBitmap 0,1,1,0
Poke.b Addr BitMap(0)+4,0
NPrint "after"
End

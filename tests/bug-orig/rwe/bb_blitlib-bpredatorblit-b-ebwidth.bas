; bb_blitlib-bpredatorblit-b-ebwidth (#426) - BPredatorBlit computes its
; offsets with the used bitmap's bytes per row, but its B channel (the
; line below, D = cookie ? B : C) with the width of SetupMapTwo (default
; 40).  The bitmap is 640 pixels wide (80 bytes per row): B reads the same
; line 320 pixels to the right instead of the line below.  In the left
; half every line but line 10 has colour 1, the right half is 0; the
; shape is a solid 16x16 block blitted at 96,10.
; Correct: pixel 100,10 gets the line below's colour (1); the bug gives
; the colour 320 pixels to the right (0).
; @rwe bb_blitlib
BitMap 0,640,32,1
Boxf 0,0,15,15,1
GetaShape 0,0,0,16,16
Cls
Boxf 0,0,319,31,1
Line 0,10,319,10,0
Buffer 0,16384
OwnBlitter_
BPredatorBlit 0,0,96,10
WaitBlit_
DisownBlitter_
NPrint Point(100,10)
End

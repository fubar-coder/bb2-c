; bb_blitlib-depth0-dbra (#429) - the blit routines count the planes with
; SUBQ.w #1 / DBRA on the shape's (or bitmap's) depth without a check: a
; shape that was never defined has depth 0, and TBlock blits 65536 planes
; through the "plane pointers" behind the bitmap object.
; Correct: a shape of depth 0 draws nothing, the program goes on.
; @rwe bb_blitlib
BitMap 0,320,64,1
NPrint "before"
OwnBlitter_
TBlock 5,160,32
WaitBlit_
DisownBlitter_
NPrint "after"
End

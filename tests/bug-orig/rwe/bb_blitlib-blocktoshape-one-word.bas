; bb_blitlib-blocktoshape-one-word (#425) - BlockToShape copies the data
; with one blit of height x depth lines of one word, whatever the shape's
; width: of a 32x16 shape of 4 planes (256 bytes, 64 per plane) only the
; first 128 bytes, i.e. planes 0 and 1, are copied.  Shape 0 is a box of
; colour 15, shape 1 an empty area of the same size; after the copy
; shape 1 is drawn with Block on a cleared bitmap.
; Correct: shape 1 is a copy of shape 0, its pixels have colour 15 (the
; bug gives 3).
; @rwe bb_blitlib
BitMap 0,320,64,4
Boxf 0,0,31,15,15
GetaShape 0,0,0,32,16
GetaShape 1,0,20,32,16
OwnBlitter_
BlockToShape 0,1
WaitBlit_
DisownBlitter_
Cls
Block 1,0,0
WaitBlit_
NPrint Point(8,8)," ",Point(24,8)
End

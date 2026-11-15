; shapeslib-initshape-blitsize (#1046) - a shape keeps its blit size in
; one word (height << 6 | words + 1, the blitter's BLTSIZE): 10 bits of
; height, 6 of width.  InitShape takes any size: a shape taller than 1023
; rows or wider than 62 words (992 pixels) gets a wrong blit size.
; Here with runtime error checking: InitShape 0,16,1100,1 (the blit size
; word $1302: 76 rows).
; Correct: an error check refuses the shape: "Shape too Big", the
; command is not done.  (The width: shapeslib-initshape-blitsize-width.)
; @fixed #1046
; @configs clib
; @pass runtime_errors=on
DEFTYPE .l
NPrint "before"
InitShape 0,16,1100,1
NPrint "after, blit size ",Hex$(Peek.w(Addr Shape(0)+8) AND $FFFF)
End

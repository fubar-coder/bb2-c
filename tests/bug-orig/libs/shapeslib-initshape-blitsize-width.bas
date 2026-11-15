; shapeslib-initshape-blitsize-width (#1046) - a shape keeps its blit
; size in one word (height << 6 | words + 1, the blitter's BLTSIZE): 6
; bits of width.  From 63 words (993 pixels) on the "+ 1" carries into
; the height field.  InitShape takes any width.
; Here with runtime error checking: InitShape 0,1000,16,1 (63 words: the
; blit size word $0440, 17 rows of 64 words).
; Correct: an error check refuses the shape: "Shape too Big", the
; command is not done.
; @fixed #1046
; @configs clib
; @pass runtime_errors=on
DEFTYPE .l
NPrint "before"
InitShape 0,1000,16,1
NPrint "after, blit size ",Hex$(Peek.w(Addr Shape(0)+8) AND $FFFF)
End

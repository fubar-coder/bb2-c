; bitmaplib-shapesbitmap-word (#79) - ShapesBitMap sets the plane
; pointers of the bitmap with ADD d1,a0 (a word, sign extended), d1 = the
; plane size of the shape: a plane of 32768 bytes or more makes the next
; pointer point before the previous one.
; Here a 640 x 410 x 2 shape (InitShape: 80 bytes per row, 32800 bytes
; per plane) and ShapesBitMap 0,0; the plane pointers of the bitmap are
; read, nothing is drawn.
; Correct: plane 1 of the bitmap is plane 0 + 32800, as in the shape.
; @fixed #79
; @configs clib
DEFTYPE .l
InitShape 0,640,410,2
ShapesBitMap 0,0
b=Addr BitMap(0)
NPrint "plane 1 - plane 0 ",Peek.l(b+12)-Peek.l(b+8)
End

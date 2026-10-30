; rimapshapeslib-mapshape-number-unchecked (#906) - the map shape commands
; take baseshape + number x 64 without a check; before SetNumMapShapes
; baseshape is 0 and the map shapes lie at address 0 on (ShapeToMapShape
; would write over the ExecBase pointer at 4).  FreeMapShapes 0,0 clears
; the width word of map shape 0, at address 0; the long there holds a
; marker (put back at the end).
; Correct: without map shapes nothing is written (the marker stays).
; @rwe rimapshapeslib
DEFTYPE .l
o=Peek.l(0)
Poke.l 0,$12345678
FreeMapShapes 0,0
m=Peek.l(0)
Poke.l 0,o
If m=$12345678 Then NPrint "address 0 kept" Else NPrint "address 0 written"
End

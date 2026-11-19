; rimapshapeslib-setnummapshapes-allocfail (#906) - SetNumMapShapes stores
; the result of its AllocVec without a check, and the map shape commands
; do not look at it: after a failed allocation the map shapes lie at
; address 0 on (ShapeToMapShape would write over the ExecBase pointer at
; 4).  The command is a statement: the program cannot see the failure.
; The program runs through inject, which fails the first allocation of
; 6400-6416 bytes (the block of 100 map shapes); FreeMapShapes 0,0 clears
; the width word of map shape 0, at address 0, where the long holds a
; marker (put back at the end); MapShapeAddr(5) gives 5 x 64 from 0.
; Correct: without map shapes nothing is written (the marker stays) and
; MapShapeAddr returns 0.
; @rwe rimapshapeslib
; @pass runwrap=@INJECT@|allocfail=6400-6416/1
DEFTYPE .l
SetNumMapShapes 100
o=Peek.l(0)
Poke.l 0,$12345678
FreeMapShapes 0,0
m=Peek.l(0)
Poke.l 0,o
NPrint "address ",MapShapeAddr(5)
If m=$12345678 Then NPrint "address 0 kept" Else NPrint "address 0 written"
End

; rimapshapeslib-mapshape-number-check (#906) - the map shape commands
; take baseshape + number x 64 without comparing the number with the
; count SetNumMapShapes allocated: a number at or above it addresses
; memory behind the block.  Here with runtime error checking:
; ShapeToMapShape of map shape 10 after SetNumMapShapes 10 writes its 14
; bytes (width, BitMap head, one plane pointer) behind the 640-byte block.
; The program runs through inject pad (64 zeroed bytes behind every
; allocation of the process, so the write stays in memory of the
; program).
; Correct: the error check refuses the number: "Map shape not allocated
; by SetNumMapShapes".
; @fixed #906
; @rwe rimapshapeslib
; @pass runwrap=@INJECT@|pad
; @pass runtime_errors=on
DEFTYPE .l
BitMap 0,320,64,1
Boxf 0,0,15,15,1
GetaShape 0,0,0,16,16
SetNumMapShapes 10
NPrint "before"
ShapeToMapShape 0,10
NPrint "after"
End

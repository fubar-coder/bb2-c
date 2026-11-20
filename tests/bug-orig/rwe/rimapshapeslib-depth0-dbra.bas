; rimapshapeslib-depth0-dbra (#903) - ShapeToMapShape copies the shape's
; plane addresses with SUBQ.w #1 / DBRA over its depth: a shape of depth 0
; writes 65536 plane pointers (256 KB) from offset 10 of the map shape
; on.  A shape of depth 0 is an invalid object; the original has no error
; check area.  Here with runtime error checking.
; Made safe: 5000 map shapes (320 KB, cleared) and map shape 0, so
; the 256 KB stay inside the block; map shape 4000 (256000 bytes on) shows
; the overwrite.  The shape is a 16x16 one whose depth word is set to 0
; (and back before the end).
; Correct: the error check refuses the shape: "Illegal Shape Depth".
; @fixed #903
; @rwe rimapshapeslib
; @pass runtime_errors=on
DEFTYPE .l
BitMap 0,320,64,1
Boxf 0,0,15,15,1
GetaShape 0,0,0,16,16
SetNumMapShapes 5000
a=MapShapeAddr(4000)
s=Addr Shape(0)
Poke.w s+4,0
NPrint "before"
ShapeToMapShape 0,0
Poke.w s+4,1
If Peek.l(a)=0 AND Peek.l(a+4)=0 Then NPrint "map shape 4000 clear" Else NPrint "map shape 4000 overwritten"
End

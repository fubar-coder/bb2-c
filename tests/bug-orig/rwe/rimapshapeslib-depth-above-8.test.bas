; rimapshapeslib-depth-above-8 (#903) - ShapeToMapShape copies one plane
; address per plane of the shape into the 8 plane pointers of the 64-byte
; map shape (from offset 10 on) without limiting the depth: depths 9-13
; write into the unused bytes 42-63, from depth 14 on the pointers run
; into the next map shape (its width first).  A shape of more than 8
; planes is an invalid object; the original has no error check area.
; Here with runtime error checking and depth 14.
; The shape is a 16x16 one whose depth word is set to 14 (and back before
; the end); only its plane addresses are written, its data is not read.
; Map shape 1 shows the overrun.
; Correct: the error check refuses the shape: "Illegal Shape Depth".
DEFTYPE .l

!TestCase{depth_14}
  BitMap 0,320,64,1
  Boxf 0,0,15,15,1
  GetaShape 0,0,0,16,16
  SetNumMapShapes 10
  a=MapShapeAddr(1)
  s=Addr Shape(0)
  Poke.w s+4,14
  !ExpectError{"Illegal Shape Depth","refused"}
  ShapeToMapShape 0,0
  Poke.w s+4,1
  !Check{Peek.l(a)=0,"map_shape_1_clear"}
!EndTestCase

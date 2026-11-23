; rimapshapeslib-kick13-allocvec (#1141) - the library's init allocates
; the group block with AllocVec (exec V36), finit and SetNumMapShapes use
; AllocVec and FreeVec, without checking exec's version; on Kickstart 1.3
; (exec V34) the offsets lie behind exec's jump table: every program with
; the library crashes at start-up, before its first line (the session ends
; in a timeout).
; Correct: the program runs on Kickstart 1.3: the group block and the map
; shapes are allocated, a second SetNumMapShapes frees the first block,
; and the program ends normally (finit frees both).
DEFTYPE .l

!TestCase{kickstart_13}
  g=ResetGroup(16,16)
  SetNumMapShapes 4
  a=MapShapeAddr(1)
  SetNumMapShapes 8
  b=MapShapeAddr(7)
  !Check{g<>0,"group"}
  !Check{a<>0 AND b<>0,"map_shapes"}
!EndTestCase

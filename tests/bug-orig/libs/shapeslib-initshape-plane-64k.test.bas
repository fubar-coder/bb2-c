; shapeslib-initshape-plane-64k (#1047) - a shape keeps its plane sizes
; in words (+22 bytes per plane, +24 with the extra word of every row);
; InitShape has no check of them: a plane of 64K or more gets wrong word
; sizes (the data is allocated with the long product, the cookie with
; the word), and later commands overwrite memory.
; Here with runtime error checking: InitShape 0,640,820,1 (80 bytes per
; row: a plane of 65600 bytes, the word +22 is 64).
; Correct: an error check refuses the shape: "Shape too Big", the
; command is not done.
; The error is announced with !ExpectError; the original raises none (see
; the configuration).
DEFTYPE .l

!TestCase{plane_64k}
  !ExpectError{"Shape too Big","refused"}
  InitShape 0,640,820,1
  !Check{0,"after"}
!EndTestCase

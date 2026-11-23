; shapeslib-initshape-size-zero (#1047) - InitShape has no check of the
; width and height: a width or height of 0 makes a shape without data
; whose blits and saves run their loops 65536 times (SaveShape of such a
; shape: #181, closed as by design, the creation is checked here).
; Here with runtime error checking: InitShape 0,16,0,1, then the height
; of the shape (+2).  Nothing is blitted.
; Correct: an error check refuses the size: "Illegal Shape Size", the
; command is not done.
; The error is announced with !ExpectError; the original raises none (see
; the configuration).
DEFTYPE .l

!TestCase{size_zero}
  !ExpectError{"Illegal Shape Size","refused"}
  InitShape 0,16,0,1
  !Check{0,"after"}
!EndTestCase

; fadelib-uninit-palette-message (#330) - the error check of FadeIn and
; FadeOut (fadechk0, debugger only) tests the palette with
; TST (a0):BEQ err2, the message "Not enough colours in Slice"; the
; message "Uninitialized Palette" (mess3, err3) is never used.
; Here: with runtime error checking, Blitz mode, a slice of 8 colours
; and FadeIn of palette 3, which the program never created.
; Correct: the runtime error "Uninitialized Palette".  The message is the
; tested behaviour, so the error is not announced with !ExpectError (see
; the configuration).
DEFTYPE .l

!TestCase{palette_missing}
  BLITZ
  Slice 0,44,320,200,$fff8,3,8,8,320,320
  FadeIn 3
  FreeSlices
  AMIGA
  !Check{0,"faded"}
!EndTestCase

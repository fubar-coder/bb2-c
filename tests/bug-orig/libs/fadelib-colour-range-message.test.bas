; fadelib-colour-range-message (#331) - the error check of the form of
; FadeIn and FadeOut with low and high colour (fadechk, debugger only)
; compares both with the colours of the slice, but jumps to err1, the
; message "Uninitialized Slice", for a colour beyond them instead of
; err2, "Not enough colours in Slice".
; Here: with runtime error checking, Blitz mode, a slice of 8 colours,
; palette 0 of 8 colours and FadeIn 0,1,0,8.
; Correct: the runtime error "Not enough colours in Slice".  The error
; is announced with !ExpectError; the original raises another one (see the
; configuration).
DEFTYPE .l

!TestCase{colour_beyond}
  For i=0 To 7 : PalRGB 0,i,i*2,15-i*2,i : Next
  BLITZ
  Slice 0,44,320,200,$fff8,3,8,8,320,320
  !ExpectError{"Not enough colours in Slice","refused"}
  FadeIn 0,1,0,8
  FreeSlices
  AMIGA
  !Check{0,"faded"}
!EndTestCase

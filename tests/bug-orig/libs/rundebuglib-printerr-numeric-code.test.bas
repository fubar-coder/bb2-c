; rundebuglib-printerr-numeric-code (#6) - an executable compiled with
; runtime error checking gets rundebuglib instead of debuglib: its
; printerr takes d0 of the TRAP #0 as the message text, also for the
; numeric codes 1-3 that only debuglib translates.  The requester shows
; an empty text (the byte at address 1, 2 or 3).  Code 1 is the used
; object test (RGB without a screen), code 2 the bounds check of an
; array field of a NewType (v\x[i]), code 3 the end of a program without
; End (blitz2's eopcode: MOVEQ #3,d0 : TRAP #0).  Each error ends the
; program, so each is a case of its own.
; Correct: the texts of debuglib ("No currently used object!", "Offset
; out of range", "End of Program").
DEFTYPE .l
NEWTYPE .t
  x.w[4]
End NEWTYPE
DEFTYPE .t v

!TestCase{used_object}
  !ExpectError{"No currently used object!","message"}
  RGB 1,15,15,15
  !Check{0,"not_stopped"}
!EndTestCase

!TestCase{offset_range}
  i=6+Len("2")
  !ExpectError{"Offset out of range","message"}
  v\x[i]=1
  !Check{0,"not_stopped"}
!EndTestCase

!TestCase{end_of_program}
  !ExpectError{"End of Program","message"}
  MOVEQ #3,d0 : TRAP #0
  !Check{0,"not_stopped"}
!EndTestCase

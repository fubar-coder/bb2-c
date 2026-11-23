; inputoutputlib-default-typo (#193) - the runtime error of Inkey$ from the
; default input (which has no single characters) reads "Can't Input
; Character from Defualt Input": a typo.  As the corpus test t43, with
; runtime error checking.
; Correct: "Can't Input Character from Default Input".
DEFTYPE .l

!TestCase{inkey_default_input}
  !ExpectError{"Can't Input Character from Default Input","message"}
  a$=Inkey$
  !Check{0,"not_stopped"}
!EndTestCase

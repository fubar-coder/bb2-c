; memacclib-peeks-negative-check (#214) - Peeks$(Address,Length) counts
; the length down unsigned and only catches 0: a negative length copies up
; to 4 GB into the string buffer.  The fix of #214 (release code) gives
; the empty string; a negative length is a programming error, and the
; original has no error check for it, not even with runtime error
; checking.
; Here with runtime error checking: Peeks$ with the length -1; the
; original copies ~4 GB (the session does not come back).
; Correct: the error check refuses the negative length: "Can't Peeks$ a
; negative number of Characters!".
DEFTYPE .l

!TestCase{negative_length}
  n=-1
  !ExpectError{"Can't Peeks$ a negative number of Characters!","refused"}
  a$=Peeks$(0,n)
  !Check{0,"after"}
!EndTestCase

; stringfunclib-cvq-length-check (#288) - Cvq reads 4 bytes of its
; string without looking at the string's length.  The fix of #288
; (release code) reads only the string's own bytes; a shorter string is a
; programming error, and the original has no error check for it, not
; even with runtime error checking.
; Here with runtime error checking: Cvq(""); the original reads the 4
; bytes behind the string: its terminating 0 and "XYZ", which
; Len("WXYZ") left in the string buffer ($0058595A).
; Correct: the error check refuses the short string: "Cvq needs a String
; of at least 4 Characters".
DEFTYPE .l

!TestCase{short_cvq}
  !ExpectError{"Cvq needs a String of at least 4 Characters","refused"}
  x=Len("WXYZ") : q.q=Cvq("")
  !Check{0,"not_refused"}
!EndTestCase

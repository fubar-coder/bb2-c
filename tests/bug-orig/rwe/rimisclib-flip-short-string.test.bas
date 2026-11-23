; rimisclib-flip-short-string (#751) - Flip$ reverses the string in place
; with a DBRA over len/2 - 1 byte pairs: for a string of 0 or 1
; characters the count is -1, the DBRA swaps 65536 pairs around the
; string, and 128 KB of memory before and behind it (the program's
; string buffers, its code or the system's memory) are reversed - a crash
; or a hang.
; Correct: Flip$ of "" is "", of "x" is "x"; nothing else changes.
DEFTYPE .l

!TestCase{short_strings}
  a$="" : b$="x"
  c$=Flip$(a$)
  d$=Flip$(b$)
  !CheckStrEq{c$,"","flip_empty"}
  !CheckStrEq{d$,"x","flip_one"}
  e$=Flip$("abc")
  !CheckStrEq{e$,"cba","flip_three"}
!EndTestCase

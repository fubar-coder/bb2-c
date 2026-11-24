; rimisclib-flip-short-string (#751) - Flip$ reverses the string in place
; with a DBRA over len/2 - 1 byte pairs: for a string of 0 or 1
; characters the count is -1, the DBRA swaps 65536 pairs around the
; string, and 128 KB of memory before and behind it are reversed.
; To keep the wild write inside memory the test owns, the program builds
; a string of 70000 characters first and appends the result of Flip$ to
; it, and the cell runs with a string buffer of 600000 bytes (jobline
; strbuf): the argument of Flip$ lies 70000 bytes inside the buffer, so
; the 64 KB before and behind it lie in the string buffer.  The checks of
; the ten characters before the result of Flip$ fail, the length checks
; pass.  The end relies on the layout of the string buffer (the temporary
; strings lie one behind the other) and on its size, not on the bug.
; Correct: Flip$ of "" is "", of "x" is "x"; nothing else changes.
DEFTYPE .l

!TestCase{short_strings}
  a$="" : b$="x"
  l$=String$("l",70000)
  c$=l$+Flip$(a$)
  !CheckEq{Len(c$),70000,"flip_empty_length"}
  !CheckStrEq{Mid$(c$,69991,10),"llllllllll","flip_empty"}
  d$=l$+Flip$(b$)
  !CheckEq{Len(d$),70001,"flip_one_length"}
  !CheckStrEq{Mid$(d$,69991,11),"llllllllllx","flip_one"}
  e$=Flip$("abc")
  !CheckStrEq{e$,"cba","flip_three"}
!EndTestCase

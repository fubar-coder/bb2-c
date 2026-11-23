; rilesdebuglib-peekmax-extra-char (#718) - PeekMax$ tests the count
; after the copy: it writes max+1 characters and returns the length max,
; a3 is left one character behind the result.  A string expression that
; goes on behind the result builds on a3.
; Correct: PeekMax$(&t$,4) is "abcd", and the next part right behind it;
; a max below 1 gives the empty string.
DEFTYPE .l

!TestCase{max_4}
  t$="abcdefgh"
  a$=PeekMax$(&t$,4)+"|"
  b$=PeekMax$(&t$,0)+"|"
  !CheckStrEq{a$,"abcd|","max_4"}
  !CheckStrEq{b$,"|","max_0"}
!EndTestCase

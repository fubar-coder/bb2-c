; memacclib-peeks-negative (#214) - Peeks$(Address,Length) counts the
; length down unsigned and only catches 0: a negative length copies up to
; 4 GB into the string buffer and overwrites the memory behind it (the
; program crashes).
; Correct: a negative length gives the empty string, as 0 does.
DEFTYPE .l

!TestCase{negative_length}
  n=-1
  a$=Peeks$(0,n)
  !CheckStrEq{a$,"","minus_one"}
  m=-5
  l=Len(Peeks$(0,m))
  !CheckEq{l,0,"minus_five"}
  a$=Peeks$(0,0)
  !CheckStrEq{a$,"","zero"}
!EndTestCase

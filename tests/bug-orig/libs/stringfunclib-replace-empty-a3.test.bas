; stringfunclib-replace-empty-a3 (#424) - Replace$ with an empty find
; string returns its first argument unchanged (d0 = its length) but
; leaves a3 where the compiler left it, behind all three arguments.  In
; a string expression the next part is then written behind the
; replacement string: the result has the right length but holds the 0
; of the first argument and further argument bytes instead of the text.
; Correct: Replace$("ab","","c")+"x" is "abx" (the 3rd character "x" =
; 120), Replace$("ab","","")+"yz" is "abyz".
DEFTYPE .l

!TestCase{empty_find}
  a$=Replace$("ab","","c")+"x"
  n1=Len(a$) : c1=Asc(Mid$(a$,3))
  a$=Replace$("ab","","")+"yz"
  n2=Len(a$) : c2=Asc(Mid$(a$,3)) : c3=Asc(Mid$(a$,4))
  !CheckEq{n1,3,"first_len"}
  !CheckEq{c1,120,"first_third"}
  !CheckEq{n2,4,"second_len"}
  !CheckEq{c2,121,"second_third"}
  !CheckEq{c3,122,"second_fourth"}
!EndTestCase

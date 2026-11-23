; compiler-incdir-rest-of-line (#17) - doincdir copies the rest of the
; line into the path of IncDir and only skips the quotes: after IncDir
; "CORPUS:src/" : a=2 the path is "CORPUS:src/:a=2", a=2 is not
; compiled, and Include "inc.bb2" opens "CORPUS:src/:a=2inc.bb2" (Can't
; open Include).
; Correct: the quoted string is the path, ':' starts the next statement;
; a is 2 and #k is 7.
!TestCase{rest_of_line}
  a.l=1
  IncDir "CORPUS:src/" : a=2
  Include "inc.bb2"
  v.l=#k
  !CheckEq{a,2,"statement_after_incdir"}
  !CheckEq{v,7,"constant"}
!EndTestCase

; compiler-newtype-forward-undefined (#974) - a by-value field of a type
; that only a pointer field named and that is never defined.
; Correct: "Type Not Found" at the end of the pass, at the line of the
; by-value field x.fwd (its layout needs the type).
NEWTYPE .pp
  *p.fwd
End NEWTYPE
NEWTYPE .dd
  x.fwd
  y.l
End NEWTYPE
DEFTYPE .dd v

!TestCase{undefined}
  !CheckEq{1,1,"compiled"}
!EndTestCase

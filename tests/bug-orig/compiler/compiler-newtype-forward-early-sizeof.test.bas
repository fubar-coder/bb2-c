; compiler-newtype-forward-early-sizeof (#974) - SizeOf a type with a
; by-value field of the forward type .fwd before NEWTYPE .fwd.
; Correct: "Type not yet defined" at the SizeOf.
NEWTYPE .pp
  *p.fwd
End NEWTYPE
NEWTYPE .dd
  x.fwd
End NEWTYPE

!TestCase{early_sizeof}
  o.l=SizeOf .dd
  NEWTYPE .fwd
    a.l
  End NEWTYPE
  !CheckEq{1,1,"compiled"}
!EndTestCase

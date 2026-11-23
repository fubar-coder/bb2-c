; compiler-newtype-forward-early-field (#974) - the field y of .dd lies
; behind the by-value field x of the forward type .fwd; it is used before
; NEWTYPE .fwd gives the size.  The offset of x itself is known.
; Correct: "Type not yet defined" at v\y=1 (SizeOf .dd\x before it is 0
; and compiles).
NEWTYPE .pp
  *p.fwd
End NEWTYPE
NEWTYPE .dd
  x.fwd
  y.l
End NEWTYPE
DEFTYPE .dd v

!TestCase{early_field}
  o.l=SizeOf .dd\x
  v\y=1
  NEWTYPE .fwd
    a.l
  End NEWTYPE
  !CheckEq{o,0,"offset"}
!EndTestCase

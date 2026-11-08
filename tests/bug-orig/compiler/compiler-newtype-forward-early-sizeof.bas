; compiler-newtype-forward-early-sizeof (#974) - SizeOf a type with a
; by-value field of the forward type .fwd before NEWTYPE .fwd.
; Correct: "Type not yet defined" at the SizeOf.
; @fixed #974
NEWTYPE .pp
  *p.fwd
End NEWTYPE
NEWTYPE .dd
  x.fwd
End NEWTYPE
o.l=SizeOf .dd
NEWTYPE .fwd
  a.l
End NEWTYPE
NPrint o
End

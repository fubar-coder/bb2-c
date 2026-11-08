; compiler-newtype-forward-undefined (#974) - a by-value field of a type
; that only a pointer field named and that is never defined.
; Correct: "Type Not Found" at the end of the pass, at the line of the
; by-value field x.fwd (its layout needs the type).
; @fixed #974
NEWTYPE .pp
  *p.fwd
End NEWTYPE
NEWTYPE .dd
  x.fwd
  y.l
End NEWTYPE
DEFTYPE .dd v
NPrint "compiled"
End

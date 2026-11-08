; compiler-newtype-forward-early-dim (#974) - Dim of an array of a type
; with a by-value field of the forward type .fwd before NEWTYPE .fwd: the
; element size is not known.
; Correct: "Type not yet defined" at the Dim.
; @fixed #974
NEWTYPE .pp
  *p.fwd
End NEWTYPE
NEWTYPE .dd
  x.fwd
End NEWTYPE
Dim a.dd(3)
NEWTYPE .fwd
  a.l
End NEWTYPE
NPrint "compiled"
End

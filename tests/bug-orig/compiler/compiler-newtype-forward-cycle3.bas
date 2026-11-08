; compiler-newtype-forward-cycle3 (#974) - a cycle over three types: .ta
; contains the forward .tb by value, .tb (defined) the forward .tc, and .tc
; is defined with a field .ta by value.
; Correct: "Illegal Type" at the field z.ta.
; @fixed #974
NEWTYPE .pp
  *p.tb
  *q.tc
End NEWTYPE
NEWTYPE .ta
  x.tb
End NEWTYPE
NEWTYPE .tb
  y.tc
End NEWTYPE
NEWTYPE .tc
  z.ta
End NEWTYPE
NPrint "compiled"
End

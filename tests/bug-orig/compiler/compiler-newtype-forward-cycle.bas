; compiler-newtype-forward-cycle (#974) - .ta contains the forward type .tb
; by value, then .tb is defined with a field .ta by value: each would
; contain the other.
; Correct: "Illegal Type" at the field y.ta (as for a type that contains
; itself, #973).
; @fixed #974
NEWTYPE .pp
  *p.tb
End NEWTYPE
NEWTYPE .ta
  x.tb
End NEWTYPE
NEWTYPE .tb
  y.ta
End NEWTYPE
NPrint "compiled"
End

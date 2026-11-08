; compiler-newtype-forward-early-local (#974) - a procedure with a local
; variable of a type with a by-value field of the forward type .fwd ends
; before NEWTYPE .fwd: its memory is laid out at the end of the procedure.
; Correct: "Type not yet defined" at End Statement.
; @fixed #974
NEWTYPE .pp
  *p.fwd
End NEWTYPE
NEWTYPE .dd
  x.fwd
End NEWTYPE
Statement s{}
  DEFTYPE .dd v
  NPrint "in s"
End Statement
NEWTYPE .fwd
  a.l
End NEWTYPE
s{}
End

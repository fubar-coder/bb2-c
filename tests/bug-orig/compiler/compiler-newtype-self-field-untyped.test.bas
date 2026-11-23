; compiler-newtype-self-field-untyped (#973) - the same for a field
; without a type behind a pointer field of the type being defined: it takes
; the previous field's type, the NewType itself, by value. Its size is the
; type node's before End NEWTYPE sets it: inject fill=01 gives $0101 (257)
; bytes, so z lies at 8 and the type has 266 bytes.
; Correct: "Illegal Type" at the field z.
NEWTYPE .selft
  y.l
  *nxt.selft
  z
End NEWTYPE

!TestCase{self_field_untyped}
  o.l=SizeOf .selft\z
  !CheckEq{SizeOf .selft,266,"size"}
  !CheckEq{o,8,"z_at"}
!EndTestCase

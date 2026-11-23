; compiler-newtype-self-field (#973) - a NewType field of the type being
; defined, not a pointer, takes the size of the type node before End
; NEWTYPE sets it: donewtype adds the node with addhere (AllocMem without
; MEMF_CLEAR), so the field x gets whatever the memory held, y's offset
; and the type's size follow from it. inject fill=01 makes that memory the
; same in every run: x gets $0101 (257) bytes, y lies at 258, the type has
; 262 bytes.
; Correct: "Illegal Type" at the field x - a type cannot contain itself
; (a pointer field *nxt.selft is allowed, corpus t15_newtype).
NEWTYPE .selft
  x.selft
  y.l
End NEWTYPE
DEFTYPE .selft v

!TestCase{self_field}
  v\y=5
  o.l=SizeOf .selft\y
  !CheckEq{SizeOf .selft,262,"size"}
  !CheckEq{o,258,"y_at"}
  !CheckEq{v\y,5,"y"}
!EndTestCase

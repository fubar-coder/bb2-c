; compiler-newtype-field-count-word (#1084) - the element count of an array field
; of a NewType (a.b[n]) is multiplied with the element size as a word
; (typemode, blitz2.s:13820, MULU d1,d3): a count of 65536 or more is
; taken modulo 65536 without an error.  a.b[65540] is a field of 4 bytes,
; and the NewType passes the check "Type too Big" (32768 bytes or more)
; that a.b[40000] fails.  The program prints the NewType's size.
; Correct: the compile error "Type too Big" at the field's line.
NEWTYPE .t
  a.b[65540]
End NEWTYPE
NPrint SizeOf .t
End

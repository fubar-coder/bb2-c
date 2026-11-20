; compiler-newtype-field-count-negative (#1084) - a negative element count
; of an array field of a NewType (a.b[n]): the count is stored and
; multiplied with the element size as a word (typemode, blitz2.s:13819-13820,
; move d1,8(a2) and MULU d1,d3), so -65532 ($FFFF0004) is taken as 4.
; a.b[-65532] is a field of 4 bytes, and the NewType passes the check
; "Type too Big" that a.b[-1] (65535 bytes) fails.  The program prints the
; NewType's size.
; Correct: the compile error "Type too Big" at the field's line.
; @fixed #1084
NEWTYPE .t
  a.b[-65532]
End NEWTYPE
NPrint SizeOf .t
End

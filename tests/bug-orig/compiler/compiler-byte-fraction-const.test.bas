; compiler-byte-fraction-const (#1188) - a constant with a fraction is cut
; to a whole number when the type so far is Byte (consttype2,
; blitz2.s:10938: .byte calls makeint without testing for a fraction,
; where .word, .long and .float keep it, as a Quick or a Float), before
; the operation: under a Byte destination 1.5+1.5 computes 1+1 (2) and
; 0.5*4 computes 0*4 (0), while the same expressions under a Word
; destination give 3 and 2.  The same in every Byte context: a Byte
; parameter of a statement and of a function, a Byte field, a Byte array
; element; a negative literal and a literal with an exponent.
; Correct: the constant keeps its fraction in every context, the
; operation is computed with it and only the result takes the type of the
; destination, as under Word.  A comparison is not affected in the port:
; its left operand is a Float again (#1186) and its right one has no type.
NEWTYPE.nb
  f.b
End NEWTYPE
Statement sb{x.b}
  !CheckEq{x,3,"param"}
End Statement
Function.b fb{x.b}
  Function Return x
End Function

!TestCase{byte_fraction}
  vb.b=2 : rb.b=0 : rw.w=0
  Dim ab.b(2) : nv.nb\f=0
  rb=(1.5+1.5)
  !CheckEq{rb,3,"b_add"}
  rw=(1.5+1.5)
  !CheckEq{rw,3,"w_add"}
  rb=(0.5*4)
  !CheckEq{rb,2,"b_mul_half"}
  rw=(0.5*4)
  !CheckEq{rw,2,"w_mul_half"}
  rb=(1.5*vb)
  !CheckEq{rb,3,"b_mul_var"}
  rw=(1.5*vb)
  !CheckEq{rw,3,"w_mul_var"}
  rb=(2.5*2)
  !CheckEq{rb,5,"b_mul_25"}
  rw=(2.5*2)
  !CheckEq{rw,5,"w_mul_25"}
  rb=(-1.5*vb)
  !CheckEq{rb,-3,"b_negative"}
  rw=(-1.5*vb)
  !CheckEq{rw,-3,"w_negative"}
  rb=(1.25e1*2)
  !CheckEq{rb,25,"b_exponent"}
  rw=(1.25e1*2)
  !CheckEq{rw,25,"w_exponent"}
  sb{1.5*vb}
  rb=fb{1.5*vb}
  !CheckEq{rb,3,"function"}
  nv\f=(1.5+1.5)
  !CheckEq{nv\f,3,"field"}
  ab(1)=(1.5*vb)
  !CheckEq{ab(1),3,"array"}
!EndTestCase

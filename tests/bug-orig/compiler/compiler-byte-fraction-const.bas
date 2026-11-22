; compiler-byte-fraction-const (#1188) - a constant with a fraction is
; cut to a whole number when the type so far is Byte (consttype2,
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
  NPrint "sb ",x
End Statement
Function.b fb{x.b}
  Function Return x
End Function
vb.b=2 : rb.b=0 : rw.w=0
Dim ab.b(2) : nv.nb\f=0
rb=(1.5+1.5) : NPrint "01 rb.b=(1.5+1.5) ",rb
rw=(1.5+1.5) : NPrint "02 rw.w=(1.5+1.5) ",rw
rb=(0.5*4) : NPrint "03 rb.b=(0.5*4) ",rb
rw=(0.5*4) : NPrint "04 rw.w=(0.5*4) ",rw
rb=(1.5*vb) : NPrint "05 rb.b=(1.5*vb) ",rb
rw=(1.5*vb) : NPrint "06 rw.w=(1.5*vb) ",rw
rb=(2.5*2) : NPrint "07 rb.b=(2.5*2) ",rb
rw=(2.5*2) : NPrint "08 rw.w=(2.5*2) ",rw
rb=(-1.5*vb) : NPrint "09 rb.b=(-1.5*vb) ",rb
rw=(-1.5*vb) : NPrint "10 rw.w=(-1.5*vb) ",rw
rb=(1.25e1*2) : NPrint "11 rb.b=(1.25e1*2) ",rb
rw=(1.25e1*2) : NPrint "12 rw.w=(1.25e1*2) ",rw
NPrint "13 sb{1.5*vb}" : sb{1.5*vb}
rb=fb{1.5*vb} : NPrint "14 fb{1.5*vb} ",rb
nv\f=(1.5+1.5) : NPrint "15 nv\f=(1.5+1.5) ",nv\f
ab(1)=(1.5*vb) : NPrint "16 ab(1)=(1.5*vb) ",ab(1)
End

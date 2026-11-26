; compiler-byte-whole-literal.test/asl-whole-point.bas: a whole-valued
; literal with a point as a shift count, refused (#1216)
vb.b=3 : rb.b=0
rb=vb ASL 1.0
NPrint rb
End

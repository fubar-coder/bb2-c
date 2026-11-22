; compiler-byte-whole-literal (#1188) - a literal written with a point or
; an exponent whose value is whole (2.0, 1.5e1) stays the whole-number
; constant it is under a Byte type so far: rb.b=vb+2.0 is an add of the
; whole number 2 and rb.b=vb AND 2.0 compiles; Data.b with a fraction
; keeps the whole part (Data.b 1.5 is 1).
; A regression test of the fix of #1188, which makes only a literal with a
; non-zero fraction a Float under a Byte type so far: the original already
; gives these (correct) results.
vb.b=3 : rb.b=0
rb=vb+2.0 : NPrint "01 rb.b=vb+2.0 ",rb
rb=vb AND 2.0 : NPrint "02 rb.b=vb AND 2.0 ",rb
rb=vb+1.5e1 : NPrint "03 rb.b=vb+1.5e1 ",rb
rb=vb ASL 1.0 : NPrint "04 rb.b=vb ASL 1.0 ",rb
Read db.b : NPrint "05 Data.b 1.5 ",db
Read db : NPrint "06 Data.b -1.5 ",db
End
Data.b 1.5,-1.5

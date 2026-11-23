; compiler-byte-whole-literal (#1188) - a literal written with a point or
; an exponent whose value is whole (2.0, 1.5e1) stays the whole-number
; constant it is under a Byte type so far: rb.b=vb+2.0 is an add of the
; whole number 2 and rb.b=vb AND 2.0 compiles; Data.b with a fraction
; keeps the whole part (Data.b 1.5 is 1).
; A regression test of the fix of #1188, which makes only a literal with a
; non-zero fraction a Float under a Byte type so far: the original already
; gives these (correct) results.

.pts
Data.b 1.5,-1.5

!TestCase{byte_whole_literal}
  vb.b=3 : rb.b=0
  rb=vb+2.0
  !CheckEq{rb,5,"add_whole_point"}
  rb=vb AND 2.0
  !CheckEq{rb,2,"and_whole_point"}
  rb=vb+1.5e1
  !CheckEq{rb,18,"add_exponent"}
  rb=vb ASL 1.0
  !CheckEq{rb,6,"asl_whole_point"}
  Restore pts
  Read db.b
  !CheckEq{db,1,"data_fraction_positive"}
  Read db
  !CheckEq{db,-1,"data_fraction_negative"}
!EndTestCase

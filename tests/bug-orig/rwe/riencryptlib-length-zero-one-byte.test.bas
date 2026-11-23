; riencryptlib-length-zero-one-byte (#630) - Encrypt and Decrypt count
; the length down with SUBQ.l #1 before a loop whose first pass is not
; tested: a length of 0 processes one byte.  Decrypt starts at
; memadr + length - 1, so with a length of 0 it changes the byte in front
; of the block.  Here the block is a(1) to a(2) of a byte array of zeros.
; Correct: a length of 0 changes nothing: all bytes stay 0 both times.
DEFTYPE .l

!TestCase{length_zero}
  Dim a.b(3)
  Encrypt &a(1),0,1,2,3
  e0=a(0) : e1=a(1) : e2=a(2) : e3=a(3)
  a(1)=0
  Decrypt &a(1),0,1,2,3
  !CheckEq{e0,0,"encrypt_byte_0"}
  !CheckEq{e1,0,"encrypt_byte_1"}
  !CheckEq{e2,0,"encrypt_byte_2"}
  !CheckEq{e3,0,"encrypt_byte_3"}
  !CheckEq{a(0),0,"decrypt_byte_0"}
  !CheckEq{a(1),0,"decrypt_byte_1"}
  !CheckEq{a(2),0,"decrypt_byte_2"}
  !CheckEq{a(3),0,"decrypt_byte_3"}
!EndTestCase

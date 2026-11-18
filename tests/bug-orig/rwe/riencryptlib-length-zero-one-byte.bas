; riencryptlib-length-zero-one-byte (#630) - Encrypt and Decrypt count
; the length down with SUBQ.l #1 before a loop whose first pass is not
; tested: a length of 0 processes one byte.  Decrypt starts at
; memadr + length - 1, so with a length of 0 it changes the byte in front
; of the block.  Here the block is a(1) to a(2) of a byte array of zeros.
; Correct: a length of 0 changes nothing: "0 0 0 0" both times.
; @fixed #630
; @rwe riencryptlib
DEFTYPE .l
Dim a.b(3)
Encrypt &a(1),0,1,2,3
NPrint a(0)," ",a(1)," ",a(2)," ",a(3)
a(1)=0
Decrypt &a(1),0,1,2,3
NPrint a(0)," ",a(1)," ",a(2)," ",a(3)
End

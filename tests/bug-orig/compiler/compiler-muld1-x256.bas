; compiler-muld1-x256 (#15) - muld1 makes the code that multiplies an
; array index by the element size: a power of two from 256 on is a shift
; by the count minus 8 and one by 8 (count field 0), but for 256 itself
; both shifts are by 8, the index is multiplied by 65536. The elements of
; an array of a 256-byte NewType, of a List array of a 248-byte NewType
; (the 8 bytes of the item header make 256) are 65536 bytes apart; the
; offset into a field array of 256-byte elements is added as a word, so
; there every element is the first one.
; Correct: the elements are the element size apart (128, 256, 512 and
; 1024 bytes), the values written are read back at those offsets.
; @fixed #15
NEWTYPE .t128
  v.l
  p.b[124]
End NEWTYPE
NEWTYPE .t256
  v.l
  p.b[252]
End NEWTYPE
NEWTYPE .t512
  v.l
  p.b[508]
End NEWTYPE
NEWTYPE .t1024
  v.l
  p.b[1020]
End NEWTYPE
NEWTYPE .t248
  v.l
  p.b[244]
End NEWTYPE
NEWTYPE .h256
  w.l
  x.t256[3]
End NEWTYPE
NPrint "sizes ",SizeOf .t128," ",SizeOf .t256," ",SizeOf .t512," ",SizeOf .t1024," ",SizeOf .t248
; with the bug element 2 is 131072 bytes in: the arrays are big enough
Dim a128.t128(2)
Dim a256.t256(520)
Dim a512.t512(2)
Dim a1024.t1024(2)
Dim List l248.t248(520)
Dim h.h256(260)
For i.l=0 To 2
  a128(i)\v=i*100+1
  a256(i)\v=i*100+2
  a512(i)\v=i*100+3
  a1024(i)\v=i*100+4
  l248(i)\v=i*100+5
  h(0)\x[i]\v=i*100+6
Next
NPrint "128: ",&a128(1)-&a128(0)," ",&a128(2)-&a128(0)
NPrint "256: ",&a256(1)-&a256(0)," ",&a256(2)-&a256(0)
NPrint "512: ",&a512(1)-&a512(0)," ",&a512(2)-&a512(0)
NPrint "1024: ",&a1024(1)-&a1024(0)," ",&a1024(2)-&a1024(0)
NPrint "list 248: ",&l248(1)-&l248(0)," ",&l248(2)-&l248(0)
NPrint "field 256: ",&h(0)\x[1]-&h(0)\x[0]," ",&h(0)\x[2]-&h(0)\x[0]
For i.l=0 To 2
  Print Peek.l(&a128(0)+i*128)," ",Peek.l(&a256(0)+i*256)," ",Peek.l(&a512(0)+i*512)," "
  NPrint Peek.l(&a1024(0)+i*1024)," ",Peek.l(&l248(0)+i*256)," ",Peek.l(&h(0)\x[0]+i*256)
Next
For i.l=0 To 2
  NPrint a128(i)\v," ",a256(i)\v," ",a512(i)\v," ",a1024(i)\v," ",l248(i)\v," ",h(0)\x[i]\v
Next
End

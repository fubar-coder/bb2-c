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
DEFTYPE .l
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

Function$ trio{a.l,b.l,c.l}
  Function Return Str$(a)+","+Str$(b)+","+Str$(c)
End Function

!TestCase{element_stride}
  ; with the bug element 2 is 131072 bytes in: the arrays are big enough
  Dim a128.t128(2)
  Dim a256.t256(520)
  Dim a512.t512(2)
  Dim a1024.t1024(2)
  Dim List l248.t248(520)
  Dim h.h256(260)
  !CheckEq{SizeOf .t128,128,"size_128"}
  !CheckEq{SizeOf .t256,256,"size_256"}
  !CheckEq{SizeOf .t512,512,"size_512"}
  !CheckEq{SizeOf .t1024,1024,"size_1024"}
  !CheckEq{SizeOf .t248,248,"size_248"}
  For i.l=0 To 2
    a128(i)\v=i*100+1
    a256(i)\v=i*100+2
    a512(i)\v=i*100+3
    a1024(i)\v=i*100+4
    l248(i)\v=i*100+5
    h(0)\x[i]\v=i*100+6
  Next
  !CheckEq{&a128(1)-&a128(0),128,"dist_128_1"}
  !CheckEq{&a128(2)-&a128(0),256,"dist_128_2"}
  !CheckEq{&a256(1)-&a256(0),256,"dist_256_1"}
  !CheckEq{&a256(2)-&a256(0),512,"dist_256_2"}
  !CheckEq{&a512(1)-&a512(0),512,"dist_512_1"}
  !CheckEq{&a512(2)-&a512(0),1024,"dist_512_2"}
  !CheckEq{&a1024(1)-&a1024(0),1024,"dist_1024_1"}
  !CheckEq{&a1024(2)-&a1024(0),2048,"dist_1024_2"}
  !CheckEq{&l248(1)-&l248(0),256,"dist_list_1"}
  !CheckEq{&l248(2)-&l248(0),512,"dist_list_2"}
  !CheckEq{&h(0)\x[1]-&h(0)\x[0],256,"dist_field_1"}
  !CheckEq{&h(0)\x[2]-&h(0)\x[0],512,"dist_field_2"}
  ; the values read at the element size apart
  !CheckStrEq{trio{Peek.l(&a128(0)),Peek.l(&a128(0)+128),Peek.l(&a128(0)+256)},"1,101,201","raw_128"}
  !CheckStrEq{trio{Peek.l(&a256(0)),Peek.l(&a256(0)+256),Peek.l(&a256(0)+512)},"2,102,202","raw_256"}
  !CheckStrEq{trio{Peek.l(&a512(0)),Peek.l(&a512(0)+512),Peek.l(&a512(0)+1024)},"3,103,203","raw_512"}
  !CheckStrEq{trio{Peek.l(&a1024(0)),Peek.l(&a1024(0)+1024),Peek.l(&a1024(0)+2048)},"4,104,204","raw_1024"}
  !CheckStrEq{trio{Peek.l(&l248(0)),Peek.l(&l248(0)+256),Peek.l(&l248(0)+512)},"5,105,205","raw_list"}
  !CheckStrEq{trio{Peek.l(&h(0)\x[0]),Peek.l(&h(0)\x[0]+256),Peek.l(&h(0)\x[0]+512)},"6,106,206","raw_field"}
  ; the values read back by index
  !CheckStrEq{trio{a128(0)\v,a128(1)\v,a128(2)\v},"1,101,201","name_128"}
  !CheckStrEq{trio{a256(0)\v,a256(1)\v,a256(2)\v},"2,102,202","name_256"}
  !CheckStrEq{trio{a512(0)\v,a512(1)\v,a512(2)\v},"3,103,203","name_512"}
  !CheckStrEq{trio{a1024(0)\v,a1024(1)\v,a1024(2)\v},"4,104,204","name_1024"}
  !CheckStrEq{trio{l248(0)\v,l248(1)\v,l248(2)\v},"5,105,205","name_list"}
  !CheckStrEq{trio{h(0)\x[0]\v,h(0)\x[1]\v,h(0)\x[2]\v},"6,106,206","name_field"}
!EndTestCase

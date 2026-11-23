; rimapeditorlib-mapdatagrab-byte-carry (#898) - MapDataGrab of bytes
; clears its value register once before the loops and then loads each
; byte with MOVE.b, which replaces only the low byte: the upper byte of
; the previous sum stays.  Bytes 250 and 5 with mapnull 10 give 260 and
; 271 ($0104, $010F): the carry of the first value goes into the second.
; Correct: each value is its byte + mapnull: 260 and 15.
DEFTYPE .l

!TestCase{bytes}
  b=AllocMem_(2,$10000)
  !Require{b<>0,"allocated"}
  Poke.b b,250 : Poke.b b+1,5
  CreateMap 0,2,1,16,16,"m"
  MapDataGrab b,0,10
  a=MapDataAdr
  w1=Peek.w(a)
  w2=Peek.w(a+2)
  FreeMem_ b,2
  !CheckEq{w1,260,"first_value"}
  !CheckEq{w2,15,"second_value"}
!EndTestCase

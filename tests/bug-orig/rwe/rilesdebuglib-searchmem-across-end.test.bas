; rilesdebuglib-searchmem-across-end (#722) - SearchMem's word and long
; searches test only the start of the value against the end address: a
; word or long that starts in front of the end and goes on behind it is
; found (the bytes compared lie up to 1 or 3 behind the end).
; Correct: a value counts only if it lies in front of the end: the word
; at b+2 with the end b+3, and the long at b+8 with the end b+9, are not
; found (0); the word with the end b+4 is.
DEFTYPE .l

!TestCase{across_end}
  b=AllocMem_(64,$10001)
  !Require{b<>0,"allocated"}
  For i=0 To 63 : Poke.b b+i,85 : Next
  Poke.w b+2,$1234
  Poke.l b+8,$11223344
  w=SearchMem(2,$1234,b,b+3)
  l=SearchMem(3,$11223344,b+4,b+9)
  wi=SearchMem(2,$1234,b,b+4)
  FreeMem_ b,64
  !CheckEq{w,0,"word"}
  !CheckEq{l,0,"long"}
  !CheckEq{wi,-1,"word_inside"}
!EndTestCase

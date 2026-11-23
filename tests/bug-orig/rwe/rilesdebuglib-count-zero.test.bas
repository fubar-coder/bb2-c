; rilesdebuglib-count-zero (#701) - D_HexWord, D_CheckNMove,
; D_CheckNMoveA and D_CheckNMoveWord count with DBRA from count-1: a
; count of 0 runs 65536 times (D_HexWord and the D_CheckNMove forms write
; 327680 bytes, D_CheckNMoveWord 65536); D_CheckNMoveWord doubles the
; count as a word, so $8001 words copy 2 bytes.  GetDataItem with size 0
; shifts 65536 bytes through the result, with size -1 65535 (its help
; text: "otherwise long").  The program gives the commands blocks large
; enough for the original's writes.
; Correct: a count of 0 writes nothing; $8001 words are 65538 bytes;
; sizes 0 and -1 read a long.
DEFTYPE .l

!TestCase{zero_count}
  b=AllocMem_(400000,$10001)
  s=AllocMem_(300000,$10001)
  !Require{b<>0 AND s<>0,"allocated"}
  Poke.b b,85 : D_HexWord s,b,0 : v1=Peek.b(b)
  Poke.b b,85 : D_CheckNMove s,b,0 : v2=Peek.b(b)
  Poke.b b,85 : D_CheckNMoveA s,b,0 : v3=Peek.b(b)
  Poke.b b,85 : D_CheckNMoveWord s,b,0 : v4=Peek.b(b)
  Poke.b b+65537,85 : D_CheckNMoveWord s,b,$8001 : v5=Peek.b(b+65537)
  Poke.l s,$11223344
  g0=GetDataItem(s,0)
  g1=GetDataItem(s,-1)
  FreeMem_ s,300000
  FreeMem_ b,400000
  !CheckEq{v1,85,"hexword"}
  !CheckEq{v2,85,"checknmove"}
  !CheckEq{v3,85,"checknmovea"}
  !CheckEq{v4,85,"checknmoveword"}
  !Check{v5<>85,"checknmoveword_8001"}
  !CheckEq{g0,$11223344,"size_0"}
  !CheckEq{g1,$11223344,"size_minus_1"}
!EndTestCase

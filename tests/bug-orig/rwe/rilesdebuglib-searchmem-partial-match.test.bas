; rilesdebuglib-searchmem-partial-match (#723) - SearchMem's string search
; starts again at the next byte after a mismatch, not one byte behind the
; start of the partial match: "ab" is not found in "aab".
; Correct: "ab" is found at b+1.
DEFTYPE .l

!TestCase{aab}
  b=AllocMem_(64,$10001)
  !Require{b<>0,"allocated"}
  Poke$ b,"aab"
  s=b+32
  Poke$ s,"ab"
  r=SearchMem(4,s,b,b+3)
  f=SearchMemFound
  fo=0
  If f<>0 Then fo=f-b
  FreeMem_ b,64
  !CheckEq{r,-1,"found"}
  !CheckEq{fo,1,"offset"}
!EndTestCase

; rilesdebuglib-searchmem-empty-string (#721) - SearchMem's string search
; with an empty string compares its terminating 0 and then the bytes
; behind it: a 0 followed by the same bytes in memory is "found".  The
; program puts a 0 and "Q" behind the empty string and into memory.
; Correct: an empty search string finds nothing (0).
DEFTYPE .l

!TestCase{empty}
  b=AllocMem_(64,$10001)
  !Require{b<>0,"allocated"}
  For i=0 To 15 : Poke.b b+i,120 : Next
  Poke.b b+4,0 : Poke.b b+5,81
  s=b+32
  Poke.b s,0 : Poke.b s+1,81 : Poke.b s+2,0
  r=SearchMem(4,s,b,b+16)
  FreeMem_ b,64
  !CheckEq{r,0,"found"}
!EndTestCase

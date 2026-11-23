; rilesdebuglib-addprocessptr-no-limit (#700) - AddProcessPtr appends to
; the table of 20 entries without a count: the 21st goes over the end
; mark and the bytes behind the table (in the original the code of
; D_InitParse, called before, not after).  ProcessString then finds the
; 21st entry ("U").
; Correct: the 21st entry is refused, "%U" gives nothing.
DEFTYPE .l

!TestCase{table_full}
  b=AllocMem_(64,$10001)
  !Require{b<>0,"allocated"}
  Poke$ b,"<%U>"
  Poke$ b+16,"LEAK"
  D_InitParse b
  For i=1 To 21 : AddProcessPtr Chr$(64+i),b+16 : Next
  a$=ProcessString
  !CheckStrEq{a$,"<>","result"}
!EndTestCase

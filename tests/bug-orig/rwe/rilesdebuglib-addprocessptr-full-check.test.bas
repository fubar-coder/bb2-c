; rilesdebuglib-addprocessptr-full-check (#700) - AddProcessPtr appends to
; the table of 20 entries without a count: the 21st goes over the end mark
; and the bytes behind the table.  Here with runtime error checking: a
; program that adds more entries than the table holds is told so.
; Correct: the error check refuses the 21st entry: "AddProcessPtr table
; holds at most 20 entries".
DEFTYPE .l

!TestCase{twenty_first}
  b=AllocMem_(64,$10001)
  !Require{b<>0,"allocated"}
  Poke$ b,"<%U>"
  Poke$ b+16,"LEAK"
  For i=1 To 20 : AddProcessPtr Chr$(64+i),b+16 : Next
  !ExpectError{"AddProcessPtr table holds at most 20 entries","refused"}
  AddProcessPtr "U",b+16
  !Check{0,"not_refused"}
  D_InitParse b
  a$=ProcessString
  !CheckStrEq{a$,"<>","result"}
!EndTestCase

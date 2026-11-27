; proc-bb2-fast - BB2 mode: FAST is not a word of the definition, a
; procedure may have the name FAST
!TestBatchInit
  zo$=""
!EndTestBatchInit
Statement FAST{a.l}
  SHARED zo$
  zo$="fast "+Str$(a)
End Statement
!TestCase{proc_bb2_fast}
  FAST{3}
  !CheckStrEq{zo$,"fast 3","l1"}
!EndTestCase

; proc-nonewsyntax - optional parameters without new syntax (OPTIMIZE 0;
; spec ab3-compat/procedures)
OPTIMIZE 0
!TestBatchInit
  zo$=""
!EndTestBatchInit
Statement s{a.l,@b.l}
  SHARED zo$
  zo$=Str$(a)+" "+Str$(b)
End Statement
!TestCase{proc_nonewsyntax}
  s{1}
  !CheckStrEq{zo$,"1 -1","l1"}
!EndTestCase

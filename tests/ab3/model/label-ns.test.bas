; label-ns - labels without colon (new syntax; spec
; ab3-compat/syntax-rules): a name followed by a token at the start of a
; statement is a label - zz is reached twice; followed by one of
; allowtab's tokens (LSL) it is BB2's assignment shortcut: a = a LSL 2
OPTIMIZE 4
!TestCase{label_ns}
  n = 0 : zt$=""
zz Let zt$=zt$+"x"
  n + 1
  If n < 2 Then Goto zz
  zs$=Left$(zt$,1)
  !CheckStrEq{zs$,"x","l1"}
  zs$=Mid$(zt$,2)
  !CheckStrEq{zs$,"x","l2"}
  a = 1
  a LSL 2
  !CheckEq{a,4,"l3"}
!EndTestCase

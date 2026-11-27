; dotlabel - .name followed by a statement on the same line: the label,
; then the statement - zgo is reached twice (spec ab3-compat/syntax-rules)
!TestCase{dotlabel}
  n = 0 : zt$=""
.zgo Let zt$=zt$+"go"
  n + 1
  If n < 2 Then Goto zgo
  zs$=Left$(zt$,2)
  !CheckStrEq{zs$,"go","l1"}
  zs$=Mid$(zt$,3)
  !CheckStrEq{zs$,"go","l2"}
!EndTestCase

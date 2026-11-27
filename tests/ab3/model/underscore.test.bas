; underscore - a variable beginning with an underscore in an expression
; (spec ab3-compat/syntax-rules)
!TestCase{underscore}
  _start.l = 5
  !CheckStrEq{Bin$(_start),"00000000000000000000000000000101","l1"}
  _x = _start * 2 + 1
  !CheckEq{_x,11,"l2"}
!EndTestCase

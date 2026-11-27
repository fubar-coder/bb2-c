; opt-expr - OPTIMIZE takes a constant expression: 1|4 (the 68020 bit and
; new syntax: the macro call takes its parameter without braces) and a
; constant
Macro pm
zs$="<`1>"
End Macro
#opt = 16
!TestCase{opt_expr}
  OPTIMIZE 1|4
  !pm 1
  !CheckStrEq{zs$,"<1>","l1"}
  OPTIMIZE #opt
  !CheckStrEq{"two","two","l2"}
!EndTestCase

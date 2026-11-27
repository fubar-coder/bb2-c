; proc-ntret0 - a NewType return type with Function Return 0 (spec
; ab3-compat/procedures; AmiBlitz3 refuses the Return, the returned type is
; not checked here: proc-ntret0.test.toml)
OPTIMIZE 4
NEWTYPE .nt
x.l
End NEWTYPE
Function.nt mk{}
  Function Return 0
End Function
!TestCase{proc_ntret0}
  *p.nt = mk{}
  If *p = 0 Then zs$="null" Else zs$="other"
  !CheckStrEq{zs$,"null","l1"}
!EndTestCase

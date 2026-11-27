; esc-func - \\__THIS_FUNCTION (spec ab3-compat/strings): the name of the
; procedure as written, GLOBAL outside; in a macro the procedure it is
; expanded in; the characters behind the name stay; the name in any case.
; The case prints the names (asserted in esc-func.test.toml).
Macro where
NPrint "> macro [\\__THIS_FUNCTION]"
End Macro
OPTIMIZE 4
Statement st{}
  NPrint "> [\\__THIS_FUNCTION]"
  !where
End Statement
Function$ MyFunc{a}
  Function Return "[\\__this_function]"
End Function
!TestCase{esc_func}
  NPrint "> [\\__THIS_FUNCTION]"
  NPrint "> [\\__THIS_FUNCTIONXY]"
  !where
  st{}
  NPrint "> ", MyFunc{1}
!EndTestCase

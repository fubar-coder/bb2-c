; esc-repeat0 - \\R00c gives an empty text (spec ab3-compat/strings;
; AmiBlitz3 runs over: esc-repeat0.test.toml). The case prints the strings
; (asserted in esc-repeat0.test.toml).
OPTIMIZE 4
!TestCase{esc_repeat0}
  NPrint "> [\\R00x]"
  NPrint "end"
!EndTestCase

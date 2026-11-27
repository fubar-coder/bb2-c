; esc-repeat - \\Rddc (spec ab3-compat/strings): the character c dd times,
; dd decimal, c unchanged in case, reading goes on behind c (AmiBlitz3
; repeats once more and in upper case: esc-repeat.test.toml). The case
; prints the strings (asserted in esc-repeat.test.toml).
OPTIMIZE 4
!TestCase{esc_repeat}
  NPrint "> [\\R05*]"
  NPrint "> [\\R03ab]"
  NPrint "> [\\r12-]"
  NPrint "> [\\R01*]"
!EndTestCase

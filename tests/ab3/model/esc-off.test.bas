; esc-off - escapes only with new syntax (spec ab3-compat/strings): without
; OPTIMIZE 4 a string keeps "\\41" as written; OPTIMIZE 4 evaluates it,
; OPTIMIZE 0 switches it off again. The case prints the strings (asserted
; in esc-off.test.toml).
!TestCase{esc_off}
  NPrint "> [\\41]"
  OPTIMIZE 4
  NPrint "> [\\41]"
  OPTIMIZE 0
  NPrint "> [\\41]"
!EndTestCase

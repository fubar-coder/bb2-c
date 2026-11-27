; esc-bb2 - BB2 mode: a string literal keeps every byte as written,
; escapes are AB3 mode's (spec ab3-compat/strings). The case prints the
; string (asserted in esc-bb2.test.toml).
!TestCase{esc_bb2}
  NPrint "> [\\41] [\\n] [\\__THIS_FUNCTION]"
!EndTestCase

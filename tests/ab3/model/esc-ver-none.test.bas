; esc-ver-none - \\__VER_* without an embedded header (spec
; ab3-compat/strings): 0. The case prints the string (asserted in
; esc-ver-none.test.toml).
OPTIMIZE 4
!TestCase{esc_ver_none}
  NPrint "> [\\__VER_MAJOR.\\__VER_MINOR.\\__VER_PATCH/\\__VER_BUILD]"
!EndTestCase

; esc-dcb-bb2 - BB2 mode: a Dc.b string keeps every byte as written, its
; escapes are AB3 mode's (spec ab3-compat/strings). The case prints the
; string (asserted in esc-dcb-bb2.test.toml).
.v
Dc.b "V\\__VER_MAJOR.\\__VER_MINOR [\\41] [\\__DATE__]",0
!TestCase{esc_dcb_bb2}
  NPrint Peek$(?v)
!EndTestCase

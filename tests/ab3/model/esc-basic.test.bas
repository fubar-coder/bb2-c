; esc-basic - the escapes AmiBlitz3 shares (spec ab3-compat/strings): a
; single '\' is a character, \\\\ gives \\, \\hh a byte (hex digits in
; either case), \\N a line feed before the hex digits; the length counts
; the bytes. The case prints the strings (asserted in esc-basic.test.toml).
OPTIMIZE 4
!TestCase{esc_basic}
  NPrint "> [\41]"
  NPrint "> [\\41]"
  NPrint "> [\\\\]"
  NPrint "> [\\\\41]"
  NPrint "> [\\4e\\4E\\4a]"
  NPrint "a\\nb"
  NPrint "> [\\N4]"
  NPrint "> [\\22]"
  a$ = "x\\41\\42y"
  NPrint Len(a$), " ", a$
  NPrint Asc("\\ff")
!EndTestCase

; esc-data - a Data string takes no escapes, also with new syntax (spec
; ab3-compat/strings; AmiBlitz3.ab3:20390). The case prints the strings
; (asserted in esc-data.test.toml). The Data stands behind the case:
; AmiBlitz3's Restore of a label above it reads nothing.
OPTIMIZE 4
!TestCase{esc_data}
  Restore strs
  Read a$
  NPrint "> ", a$
  Read a$
  NPrint "> ", a$
!EndTestCase
.strs
Data$ "[\\41]"
Data$ "[\\__THIS_FUNCTION]"

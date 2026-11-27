; join - joining lines with @@ (spec ab3-compat/syntax-rules): a parameter
; list over three lines (after a ',', the rest of the line ignored), at
; the start of a function's parameters, a line of a macro; in a string @@
; stays. The case prints the joined statements (asserted in
; join.test.toml): AmiBlitz3 joins a list after a ',' but not inside the
; braces of a call, so the joined lines stay NPrint's.
Macro zm
NPrint "m", @@
"n"
End Macro
!TestCase{join}
  NPrint "a", @@
  "b", @@   ; a comment
  "c"
  NPrint "x@@y"
  NPrint Str$(@@
3)
  !zm
!EndTestCase

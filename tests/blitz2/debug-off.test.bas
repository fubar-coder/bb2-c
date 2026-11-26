; debug-off - #__debug (#1226) without runtime error checking: 0; Stop inside CNIF
; #__debug is skipped; a source's own #__debug replaces the predefined one
; (as in AmiBlitz3, where it is an ordinary constant defined before the
; source). The case prints the values (asserted in debug-off.test.toml).
; Correct: 0, release build, 5, end.
!TestCase{release}
  NPrint #__debug
  CNIF #__debug : Stop : CEND
  CNIF #__debug=0
  NPrint "release build"
  CEND
  #__debug=5
  NPrint #__debug
  NPrint "end"
!EndTestCase

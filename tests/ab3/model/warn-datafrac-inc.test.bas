; warn-datafrac-inc - the warning datafrac in an include of text: the place
; is the include and its line, as for an error (ab3_incplace). The include
; is the test's own data (warn-datafrac-inc.test/datafrac_inc_data.ab3); an
; Include stands inside the case, as no statement is allowed between the
; cases, and its Data is no code.
!TestCase{datafrac_inc}
  Restore dfi_d
  Read a.b
  !CheckEq{a,2,"cut"}
  INCLUDE "@TESTDIR@/datafrac_inc_data.ab3"
!EndTestCase

; compiler-for-global-string (#39) - after a string assignment dolet2 returns
; varmode (0 for a global variable) as the type of the variable, so dofor's
; type check lets a global string through as a For variable.
; Correct: a string For variable is refused ("Bad Type for For...Next"), as
; it is for a local one.
!TestCase{for_global_string}
  For a$="a" To 10
  Next
  !CheckEq{1,1,"after_loop"}
!EndTestCase

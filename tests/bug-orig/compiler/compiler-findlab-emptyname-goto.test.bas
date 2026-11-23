; compiler-findlab-emptyname-goto (#30) - makealab takes the label name after
; Goto with makename2 and does not test that there was one: an empty label
; is created with an outstanding reference, and the second Goto runs findlab
; with length 0 against it (the comparison goes past the empty name).
; Correct: Goto without a label name is a syntax error.
!TestCase{findlab_emptyname_goto}
  Goto
  Goto
  !CheckEq{1,1,"after_goto"}
!EndTestCase

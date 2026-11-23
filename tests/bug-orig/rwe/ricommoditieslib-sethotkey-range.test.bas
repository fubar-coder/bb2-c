; ricommoditieslib-sethotkey-range (#564) - SetHotKey uses the number as
; the index into its tables of 16 filters and senders without a range
; check.  With -1 the "filter" slot is the variable in front of the
; table, the broker itself: it is removed with RemoveCxObj (and the
; variable overwritten with the new filter).  The program shows that
; through a second broker of the same name: commodities refuses it
; (NBU_UNIQUE) while the first one is still there.  The original has no
; error check area; here with runtime error checking.
; Correct: the error check refuses the number: "Hotkey number must be 0
; to 15".
DEFTYPE .l

!TestCase{number_minus1}
  made=MakeCommodity("BugOrigRange","BugOrig","range")
  !Require{made<>0,"commodity"}
  !ExpectError{"Hotkey number must be 0 to 15","refused"}
  r=SetHotKey(-1,"lalt f1")
  second=MakeCommodity("BugOrigRange","BugOrig","range")
  !CheckEq{second,0,"second_refused"}
!EndTestCase

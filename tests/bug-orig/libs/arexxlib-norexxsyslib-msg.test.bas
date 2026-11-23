; arexxlib-norexxsyslib-msg (#56) - without rexxsyslib.library the
; internal RexxMsg check of the message commands shows its alert and then
; returns "valid" with the message address 0: GetRexxResult(12345,2)
; reads rm_Result2 of a message at address 0, the long word at $24 (other
; commands write there, FillRexxMsg into the exception vectors).
; The program runs through inject: the OpenLibrary of rexxsyslib.library
; fails (openlibfail=) and the alerts are answered at once (alerts).
; Correct: after the alert the message is no RexxMsg: the result is 0.
DEFTYPE .l

!TestCase{result_without_library}
  v=GetRexxResult(12345,2)
  !CheckEq{v,0,"result"}
!EndTestCase

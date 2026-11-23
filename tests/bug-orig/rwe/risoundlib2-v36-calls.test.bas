; risoundlib2-v36-calls (#1139) - the library's init calls CreateMsgPort
; and CreateIORequest (exec V36) without checking exec's version; on
; Kickstart 1.3 (exec V34) the offsets lie behind exec's jump table: every
; program with the library crashes at start-up, before its first line
; (the session ends in a timeout).
; Correct: the program runs on Kickstart 1.3 (the commands fail, OpenAHI
; returns 0) and ends normally.
DEFTYPE .l

!TestCase{start_and_open}
  r.l=OpenAHI(0)
  !CheckEq{r,0,"open"}
!EndTestCase

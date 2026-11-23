; ricompactdisclib-v36-calls (#1138) - the library's init calls
; CreateMsgPort (exec V36) without checking exec's version; on Kickstart
; 1.3 (exec V34) the offset lies behind exec's jump table: every program
; with the library crashes at start-up, before its first line (the
; session ends in a timeout).  OpenCD calls CreateIORequest and
; DeleteIORequest (V36) as well.
; Correct: the program runs on Kickstart 1.3 (the library builds its port
; and request with exec V33 functions), OpenCD of a device that does not
; exist fails, and the program ends normally.
DEFTYPE .l

!TestCase{open_fails}
  r=OpenCD("nonexistent.device",0)
  !CheckEq{r,0,"result"}
!EndTestCase

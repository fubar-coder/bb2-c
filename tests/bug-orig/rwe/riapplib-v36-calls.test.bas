; riapplib-v36-calls (#1143) - the library's init calls CreateMsgPort
; (exec V36) before it opens workbench, dos and icon.library V37; on
; Kickstart 1.3 (exec V34) the offset lies behind exec's jump table: every
; program with the library crashes at start-up, before its first line
; (the session ends in a timeout).
; Correct: the program runs on Kickstart 1.3; the V37 libraries do not
; open there, so init fails quietly: AddAppMenu returns 0, AppEvent 0,
; AppMsgPort 0, and the program ends normally.
DEFTYPE .l

!TestCase{kickstart_13}
  r=AddAppMenu(0,"bug-orig")
  !CheckEq{r,0,"menu"}
  r=AppEvent
  !CheckEq{r,0,"event"}
  p=AppMsgPort
  !CheckEq{p,0,"port"}
!EndTestCase

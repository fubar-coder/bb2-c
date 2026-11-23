; ricommoditieslib-no-replymsg (#561) - the messages commodities.library
; sends to the library's port are never replied (replymessage starts with
; RTS), so commodities never frees them: every message a program takes
; with CommodityEvent leaks its CxMsg and the copied input event.  A
; hotkey on the input class "timer" (input.device sends about ten timer
; events a second) gives messages without input; the program takes 150
; of them and compares the free memory before and after (10 taken before
; the first measurement).
; Correct: the memory comes back - less than 10000 bytes missing (the
; leak is about 100 bytes per message, 150 messages ~15000; a cold-boot
; session allocates 6136 bytes of its own once, so the margin is above it).
DEFTYPE .l

!TestCase{leak}
  made=MakeCommodity("BugOrigReply","BugOrig","no ReplyMsg")
  !Require{made<>0,"commodity"}
  hk=SetHotKey(1,"timer")
  !Require{hk<>0,"hotkey"}
  n=0
  While n<10
    If CommodityEvent Then n+1 Else VWait
  Wend
  m0=AvailMem_(0)
  n=0
  While n<150
    If CommodityEvent Then n+1 Else VWait
  Wend
  m1=AvailMem_(0)
  leaked=(m0-m1>10000)
  !CheckEq{leaked,0,"leaked"}
!EndTestCase

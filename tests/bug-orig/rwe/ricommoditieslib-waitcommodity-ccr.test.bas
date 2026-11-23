; ricommoditieslib-waitcommodity-ccr (#566) - WaitCommodity branches on
; the condition codes GetMsg leaves, and inverted: with messages queued
; GetMsg removes the first (lost) and WaitPort returns the second, which
; stays queued, so the next CommodityEvent returns it again.  Hotkeys 1
; and 2 on the input class "timer" (input.device sends timer events all
; the time): each timer event sends a message with id 1, then one with
; id 2 (input.device's task sends both before the program runs again).
; The port is emptied with CommodityEvent (the messages of hotkey 1 alone
; before SetHotKey 2), then after VWait 10 several pairs are queued.
; Correct: WaitCommodity returns the first message (1), CommodityEvent
; the next one (2).
DEFTYPE .l

!TestCase{queued}
  made=MakeCommodity("BugOrigWait","BugOrig","WaitCommodity")
  !Require{made<>0,"commodity"}
  hk1=SetHotKey(1,"timer")
  !Require{hk1<>0,"hotkey1"}
  hk2=SetHotKey(2,"timer")
  !Require{hk2<>0,"hotkey2"}
  While CommodityEvent : Wend
  VWait 10
  first=0 : nxt=0
  If WaitCommodity Then first=HotKeyHit
  If CommodityEvent Then nxt=HotKeyHit
  !CheckEq{first,1,"wait_id"}
  !CheckEq{nxt,2,"event_id"}
!EndTestCase

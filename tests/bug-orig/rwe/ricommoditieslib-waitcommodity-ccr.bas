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
; @fixed #566
; @rwe ricommoditieslib
If MakeCommodity("BugOrigWait","BugOrig","WaitCommodity")
  If SetHotKey(1,"timer") AND SetHotKey(2,"timer")
    While CommodityEvent : Wend
    VWait 10
    If WaitCommodity Then NPrint HotKeyHit
    If CommodityEvent Then NPrint HotKeyHit
  Else
    NPrint "no hotkey"
  EndIf
Else
  NPrint "no commodity"
EndIf
End

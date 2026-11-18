; ricommoditieslib-exchange-type (#555) - ExchangeDisable ... ExchangeUnique
; compare only the id of the last message with the CXCMD_ code, not its
; type: the id of a hotkey message is the hotkey number, so hotkey 15 (a
; valid slot) counts as CXCMD_DISABLE.  The hotkey is on the input class
; "timer" (input.device sends timer events all the time), so a message
; comes without input.
; Correct: HotKeyHit 15, ExchangeDisable 0 (the message is no command).
; @fixed #555
; @rwe ricommoditieslib
If MakeCommodity("BugOrigType","BugOrig","exchange type")
  If SetHotKey(15,"timer")
    n=0
    Repeat
      VWait
      n+1
    Until CommodityEvent Or n>250
    NPrint HotKeyHit
    NPrint ExchangeDisable
  Else
    NPrint "no hotkey"
  EndIf
Else
  NPrint "no commodity"
EndIf
End

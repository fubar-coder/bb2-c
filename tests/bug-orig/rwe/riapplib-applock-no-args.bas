; riapplib-applock-no-args (#522) - AppLock returns am_ArgList->wa_Lock
; without looking at am_NumArgs.  The program puts an App message of its
; own without arguments (am_NumArgs 0, as for an App menu item chosen
; without icons) into the library's AppMsgPort; am_ArgList points to a
; long with a marker.
; Correct: a message without arguments has no lock, AppLock returns 0.
; @rwe riapplib
DEFTYPE .l
m=AllocMem_(128,$10001)
w=AllocMem_(16,$10001)
If m=0 OR w=0 Then NPrint "no memory" : End
Poke.l w,$12345678
Poke.b m+8,5 : Poke.w m+18,64
Poke.w m+20,9 : Poke.l m+30,0 : Poke.l m+34,w
PutMsg_ AppMsgPort,m
If AppEvent
  If AppLock=$12345678 Then NPrint "the marker" Else NPrint AppLock
  r=AppEvent
Else
  NPrint "no message"
EndIf
FreeMem_ w,16
FreeMem_ m,128
End

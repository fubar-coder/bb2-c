; riapplib-appfile-index-le-0 (#520) - AppFile(n) takes argument n - 1
; and checks only the upper bound (signed): AppFile(0) reads the WBArg in
; front of am_ArgList.  The program puts an App message of its own (one
; argument) into the library's AppMsgPort; in front of its argument list
; lies a WBArg {lock on RAM:, "before"} (a root lock: AppFile does not
; unlock it).
; Correct: AppFile(0) is out of range and returns an empty string.
; @rwe riapplib
DEFTYPE .l
m=AllocMem_(128,$10001)
w=AllocMem_(64,$10001)
l=Lock_("RAM:",-2)
If m=0 OR w=0 OR l=0 Then NPrint "no memory or lock" : End
n$="before"
For i=1 To 6 : Poke.b w+31+i,Asc(Mid$(n$,i,1)) : Next
Poke.l w,l : Poke.l w+4,w+32
Poke.l w+8,l : Poke.l w+12,w+32
Poke.b m+8,5 : Poke.w m+18,64
Poke.w m+20,7 : Poke.l m+30,1 : Poke.l m+34,w+8
PutMsg_ AppMsgPort,m
If AppEvent
  f$=AppFile(0)
  c$="other"
  If f$="" Then c$="empty"
  If Right$(f$,6)="before" Then c$="the WBArg before the list"
  NPrint c$
  r=AppEvent
Else
  NPrint "no message"
EndIf
UnLock_ l
FreeMem_ w,64
FreeMem_ m,128
End

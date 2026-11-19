; riapplib-applock-current-file (#1103) - AppLock returns the lock of the
; App message's first argument, whatever NextAppFile or AppFile read;
; RIAppLib.guide (node AppLock): "the address of the lock associated with
; the current AppFile".  The program puts two App messages of its own
; into the library's AppMsgPort: one with three arguments in three
; directories (RAM:, RAM:bo1103a, RAM:bo1103b; the names f1, f2, f3), one
; with the first of them.  It reads AppLock after the first AppEvent and
; after NextAppFile (f1), NextAppFile (f2), AppFile(1), NextAppFile (f3),
; NextAppFile (none left), AppFile(9) (out of range), AppFile(1), and
; after the AppEvent of the second message.
; Correct: before any read the first argument's lock, then the lock of
; the argument read last; a read that finds no argument changes nothing;
; the next AppEvent starts again at the first argument.
; The original also unlocks the lock of an argument that is no root and
; clears it in the message (#521): the program reads f2 and f3 once each
; and unlocks only the locks still in the message.
; @rwe riapplib
DEFTYPE .l
d=CreateDir_("RAM:bo1103a")
If d Then UnLock_ d
d=CreateDir_("RAM:bo1103b")
If d Then UnLock_ d
l1=Lock_("RAM:",-2)
l2=Lock_("RAM:bo1103a",-2)
l3=Lock_("RAM:bo1103b",-2)
m=AllocMem_(128,$10001)
k=AllocMem_(128,$10001)
w=AllocMem_(64,$10001)
If m=0 OR k=0 OR w=0 OR l1=0 OR l2=0 OR l3=0 Then NPrint "no memory or lock" : End
Poke.l w,l1 : Poke.l w+8,l2 : Poke.l w+16,l3
For i=1 To 3
  Poke.l w+i*8-4,w+32+i*4
  Poke.b w+32+i*4,Asc("f") : Poke.b w+33+i*4,Asc("0")+i
Next
Poke.b m+8,5 : Poke.w m+18,64
Poke.w m+20,7 : Poke.l m+30,3 : Poke.l m+34,w
Poke.b k+8,5 : Poke.w k+18,64
Poke.w k+20,7 : Poke.l k+30,1 : Poke.l k+34,w
PutMsg_ AppMsgPort,m
PutMsg_ AppMsgPort,k
If AppEvent
  t$="AppEvent" : Gosub lockline
  t$=NextAppFile : Gosub lockline
  t$=NextAppFile : Gosub lockline
  t$="AppFile(1) "+AppFile(1) : Gosub lockline
  t$=NextAppFile : Gosub lockline
  a$=NextAppFile : t$="none left ("+a$+")" : Gosub lockline
  a$=AppFile(9) : t$="AppFile(9) ("+a$+")" : Gosub lockline
  t$="AppFile(1) "+AppFile(1) : Gosub lockline
  If AppEvent
    t$="next AppEvent" : Gosub lockline
  Else
    NPrint "no second message"
  EndIf
  r=AppEvent
Else
  NPrint "no message"
EndIf
UnLock_ l1
If Peek.l(w+8)=l2 Then UnLock_ l2
If Peek.l(w+16)=l3 Then UnLock_ l3
r=DeleteFile_("RAM:bo1103a")
r=DeleteFile_("RAM:bo1103b")
FreeMem_ w,64
FreeMem_ k,128
FreeMem_ m,128
End

lockline:
a=AppLock
If a=l1 Then NPrint t$,": lock 1" : Return
If a=l2 Then NPrint t$,": lock 2" : Return
If a=l3 Then NPrint t$,": lock 3" : Return
NPrint t$,": lock ",Hex$(a)
Return

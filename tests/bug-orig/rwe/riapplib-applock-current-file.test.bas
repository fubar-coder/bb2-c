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
DEFTYPE .l

!TestCase{lock_of_current_file}
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
  !Require{m<>0 AND k<>0 AND w<>0 AND l1<>0 AND l2<>0 AND l3<>0,"memory_and_locks"}
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
  ev=AppEvent
  !Require{ev<>0,"message_found"}
  !CheckEq{AppLock,l1,"lock_app_event"}
  t$=NextAppFile
  !CheckStrEq{t$,"Ram Disk:f1","path_f1"}
  !CheckEq{AppLock,l1,"lock_f1"}
  t$=NextAppFile
  !CheckStrEq{t$,"Ram Disk:bo1103a/f2","path_f2"}
  !CheckEq{AppLock,l2,"lock_f2"}
  t$=AppFile(1)
  !CheckStrEq{t$,"Ram Disk:f1","path_file_1"}
  !CheckEq{AppLock,l1,"lock_file_1"}
  t$=NextAppFile
  !CheckStrEq{t$,"Ram Disk:bo1103b/f3","path_f3"}
  !CheckEq{AppLock,l3,"lock_f3"}
  t$=NextAppFile
  !CheckStrEq{t$,"","path_none_left"}
  !CheckEq{AppLock,l3,"lock_none_left"}
  t$=AppFile(9)
  !CheckStrEq{t$,"","path_out_of_range"}
  !CheckEq{AppLock,l3,"lock_out_of_range"}
  t$=AppFile(1)
  !CheckStrEq{t$,"Ram Disk:f1","path_file_1_again"}
  !CheckEq{AppLock,l1,"lock_file_1_again"}
  ev=AppEvent
  !Check{ev<>0,"second_message"}
  !CheckEq{AppLock,l1,"lock_next_event"}
  r=AppEvent
  UnLock_ l1
  If Peek.l(w+8)=l2 Then UnLock_ l2
  If Peek.l(w+16)=l3 Then UnLock_ l3
  r=DeleteFile_("RAM:bo1103a")
  r=DeleteFile_("RAM:bo1103b")
  FreeMem_ w,64
  FreeMem_ k,128
  FreeMem_ m,128
!EndTestCase

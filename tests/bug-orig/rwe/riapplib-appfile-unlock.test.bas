; riapplib-appfile-unlock (#521) - NextAppFile and AppFile build the path
; of an argument with NameFromLock and, if it does not end with ':' (the
; lock is not a root), unlock the argument's lock (ChangeMode + UnLock)
; and clear wa_Lock in the App message: the lock belongs to the message
; (to Workbench), and a second AppFile of the same argument gets lock 0.
; The program puts an App message of its own (one argument: a lock on
; the directory RAM:bo521, the name "file") into the library's
; AppMsgPort and reads AppFile(1) twice.
; Correct: both paths are "Ram Disk:bo521/file", the lock stays in the
; message (the program unlocks it itself).
DEFTYPE .l

!TestCase{lock_stays}
  d=CreateDir_("RAM:bo521")
  If d Then UnLock_ d
  l=Lock_("RAM:bo521",-2)
  m=AllocMem_(128,$10001)
  w=AllocMem_(64,$10001)
  !Require{m<>0 AND w<>0 AND l<>0,"memory_and_lock"}
  n$="file"
  For i=1 To 4 : Poke.b w+31+i,Asc(Mid$(n$,i,1)) : Next
  Poke.l w+8,l : Poke.l w+12,w+32
  Poke.b m+8,5 : Poke.w m+18,64
  Poke.w m+20,7 : Poke.l m+30,1 : Poke.l m+34,w+8
  PutMsg_ AppMsgPort,m
  ev=AppEvent
  !Require{ev<>0,"message_found"}
  a$=AppFile(1)
  b$=AppFile(1)
  !CheckStrEq{a$,"Ram Disk:bo521/file","first_path"}
  !CheckStrEq{b$,a$,"second_path"}
  !CheckEq{Peek.l(w+8),l,"lock_kept"}
  r=AppEvent
  If Peek.l(w+8)=l Then UnLock_ l
  r=DeleteFile_("RAM:bo521")
  FreeMem_ w,64
  FreeMem_ m,128
!EndTestCase

; riapplib-appfile-a3-behind-0 (#519) - NextAppFile and AppFile copy the
; file name with its terminating 0 and leave a3 behind the 0 instead of
; on it.  In a string expression the next part is then written one byte
; too far: the result has the right length but holds the 0 behind the
; name, and the last character of the next part is lost.  The program
; puts an App message of its own (one argument: a lock on RAM:, the name
; "file") into the library's AppMsgPort (a root lock: AppFile does not
; unlock it).
; Correct: NextAppFile+".bak" and AppFile(1)+".bak" end in ".bak" (the
; 4th character from the end "." = 46, the last "k" = 107); the path is
; the volume's name ("RAM Disk:file").
DEFTYPE .l

!TestCase{bak_suffix}
  m=AllocMem_(128,$10001)
  w=AllocMem_(64,$10001)
  l=Lock_("RAM:",-2)
  !Require{m<>0 AND w<>0 AND l<>0,"memory_and_lock"}
  n$="file"
  For i=1 To 4 : Poke.b w+31+i,Asc(Mid$(n$,i,1)) : Next
  Poke.l w+8,l : Poke.l w+12,w+32
  Poke.b m+8,5 : Poke.w m+18,64
  Poke.w m+20,7 : Poke.l m+30,1 : Poke.l m+34,w+8
  PutMsg_ AppMsgPort,m
  ev=AppEvent
  !Require{ev<>0,"message_found"}
  f$=NextAppFile+".bak"
  !CheckEq{Asc(Mid$(f$,Len(f$)-3)),46,"next_dot"}
  !CheckEq{Asc(Right$(f$,1)),107,"next_last"}
  f$=AppFile(1)+".bak"
  !CheckEq{Asc(Mid$(f$,Len(f$)-3)),46,"file_dot"}
  !CheckEq{Asc(Right$(f$,1)),107,"file_last"}
  r=AppEvent
  UnLock_ l
  FreeMem_ w,64
  FreeMem_ m,128
!EndTestCase

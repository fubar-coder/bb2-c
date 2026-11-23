; riarexxlib-v36-calls (#1144) - RXCreatePort allocates its message lists
; with AllocVec and makes the port with CreateMsgPort (exec V36) without
; checking exec's version; on Kickstart 1.3 (exec V34) the offsets lie
; behind exec's jump table: the program crashes at RXCreatePort, after
; "before" (the session ends in a timeout).  The library opens
; rexxsyslib.library in version 34, the ARexx of Kickstart 1.3.
; Here: a public port made, found by its name, a message of the program's
; put into it and read (RXReadPort), the port deleted.  The message has
; no reply port and a sender's name no port has: RXReplyMsg finds no
; sender and replies nothing.  Workbench 1.3.5 has no ARexx
; (rexxsyslib.library), so nothing is sent to another port.
; Correct: the program runs on every machine (the library builds its port
; and blocks with exec V33 functions) and ends normally.
DEFTYPE .l

!TestCase{own_port}
  q$="BUGORIG"
  p=RXCreatePort(q$)
  created=(p<>0)
  !CheckEq{created,-1,"created"}
  f=FindPort_(&q$)
  same_port=(f=p)
  !CheckEq{same_port,-1,"public"}
  m=AllocMem_(128,$10001)
  !Require{m<>0,"memory"}
  n$="RXNOSUCHPORT"
  Poke.b m+8,5                        ; NT_MESSAGE
  Poke.l m+10,&n$                     ; the sender's name
  Poke.w m+18,128
  PutMsg_ p,m
  r=RXReadPort
  got=(r=m)
  !CheckEq{got,-1,"read"}
  s$=RXSendersName
  !CheckStrEq{s$,"RXNOSUCHPORT","sender"}
  RXReplyMsg 0
  !CheckEq{RXReadPort,0,"read_again"}
  RXDeletePort
  f=FindPort_(&q$)
  gone=(f=0)
  !CheckEq{gone,-1,"deleted"}
  FreeMem_ m,128
!EndTestCase

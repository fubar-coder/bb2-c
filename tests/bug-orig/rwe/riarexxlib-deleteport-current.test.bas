; riarexxlib-deleteport-current (#1101) - RXDeletePort replies the
; messages still waiting with code 5, but its loop calls RXReadPort first,
; which makes the next message of the list current without replying the
; one that is current: the message current when RXDeletePort is called is
; never replied, its sender waits for ever.  The program puts a message of
; its own (sender's name BUGORIG, reply port a port of the program's) to
; the library's port, takes it with RXReadPort (it becomes current) and
; calls RXDeletePort.
; Correct: the message is replied to the program's port with code 5.
DEFTYPE .l

!TestCase{current_message}
  n$="BUGORIG"
  rp=CreateMsgPort_
  m=AllocMem_(128,$10001)
  !Require{rp<>0 AND m<>0,"memory"}
  p=RXCreatePort("BUGORIG")
  !Require{p<>0,"port"}
  Poke.l m+10,&n$                     ; ln_Name: the sender's port (exists)
  Poke.l m+14,rp                      ; mn_ReplyPort
  Poke.w m+18,128                     ; mn_Length
  Poke.l m+$1C,$01000000              ; rm_Action RXCOMM
  PutMsg_ p,m
  c=RXReadPort
  !CheckEq{c,m,"current"}
  RXDeletePort
  r=GetMsg_(rp)
  !CheckEq{r,m,"replied"}
  If r
    !CheckEq{Peek.l(m+$20),5,"code"}
  EndIf
  DeleteMsgPort_ rp
  FreeMem_ m,128                      ; ours again: taken from rp, or in no list
!EndTestCase

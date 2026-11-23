; riarexxlib-replymsg-sender-gone (#540) - RXReplyMsg code,result$ makes
; the result string (rm_Result2) before it looks for the sender's port
; (the message's ln_Name); with the port gone the message is not replied,
; and whether the string is deleted again is decided by the long 36 bytes
; behind the a0 that FindPort leaves: the string is lost, or (in the form
; without a result string) DeleteArgString is called with 0.  The program
; sends "hello" to its own port, receives it, marks it as asking for a
; result (RXFF_RESULT), gives it a sender's name no port has and replies
; it with a result string.
; Correct: nothing is replied, so no result string is made - rm_Result2
; stays 0.
; The message sent to the own port is never replied: finit waits 5 s for
; its reply (#1095), silent; the hang check gets 20 s instead of 8.
DEFTYPE .l

!TestCase{sender_gone}
  p=RXCreatePort("BUGORIG")
  RXTokenise On
  RXRunCommand "BUGORIG","hello"
  m=RXReadPort
  !Require{m<>0,"message_received"}
  Poke.l m+$1C,Peek.l(m+$1C) OR $20000
  n$="RXNOSUCHPORT"
  Poke.l m+10,&n$
  RXReplyMsg 0,"result"
  set=(Peek.l(m+$24)<>0)
  !CheckEq{set,0,"result2_set"}
!EndTestCase

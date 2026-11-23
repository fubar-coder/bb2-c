; riarexxlib-runcommand-result (#543) - RXRunCommand returns the d0 that
; exec's PutMsg leaves when the message was sent: not a defined result
; (two runs of the original gave 0 and 256, so a program that tests it
; may take a sent message for one not sent).  The program sends "hello"
; to its own port and checks whether the result is -1.
; Correct: the message was sent - -1 (0 only when it is not sent).
DEFTYPE .l

!TestCase{sent}
  p=RXCreatePort("BUGORIG")
  r=RXRunCommand("BUGORIG","hello")
  is_true=(r=-1)
  !CheckEq{is_true,-1,"result_is_true"}
  m=RXReadPort
  If m Then RXReplyMsg 0
!EndTestCase

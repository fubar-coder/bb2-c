; riarexxlib-runcommand-result (#543) - RXRunCommand returns the d0 that
; exec's PutMsg leaves when the message was sent: not a defined result
; (two runs of the original gave 0 and 256, so a program that tests it
; may take a sent message for one not sent).  The program sends "hello"
; to its own port and prints whether the result is -1.
; Correct: the message was sent - -1 (0 only when it is not sent).
; @fixed #543
; @rwe riarexxlib
DEFTYPE .l
p=RXCreatePort("BUGORIG")
r=RXRunCommand("BUGORIG","hello")
NPrint "result -1: ",r=-1
m=RXReadPort
If m Then RXReplyMsg 0
End

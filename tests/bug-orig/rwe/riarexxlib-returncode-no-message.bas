; riarexxlib-returncode-no-message (#542) - RXReturnCode returns the
; Result1 of the last reply to RXSendCommand only while a message is
; current; right after an RXSendCommand that found no other message it
; returns 0.  The program starts RexxMast (not started in the sessions;
; SYS:System/RexxMast writes nothing), waits for its port REXX and sends
; it the string program "/*", which fails (ARexx error 6, unterminated
; comment; RexxMast prints its error message); no message of its own is
; waiting.  (A program that ends with "exit 5" does not do: with
; RXFF_RESULT ARexx gives its value as the result string, code 0.)
; Correct: RXReturnCode is the error's severity - 10.
; @fixed #542
; @rwe riarexxlib
DEFTYPE .l
r$="REXX"
If FindPort_(&r$)=0
  c$="SYS:System/RexxMast >NIL:"
  Execute_ &c$,0,0
EndIf
w=0
While FindPort_(&r$)=0 AND w<100
  Delay_ 5 : w+1
Wend
x=FindPort_(&r$)
NPrint "rexx ",x<>0
p=RXCreatePort("BUGORIG")
RXTokenise On
a$=RXSendCommand("REXX","/*")
NPrint "rc ",RXReturnCode
End

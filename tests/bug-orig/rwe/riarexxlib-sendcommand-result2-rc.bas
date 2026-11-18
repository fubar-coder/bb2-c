; riarexxlib-sendcommand-result2-rc (#545) - with rm_Result1 not 0,
; rm_Result2 is a secondary error code, but RXSendCommand takes any
; Result2 that is not 0 as the address of the result string.  The program
; sends RexxMast (port REXX) a string program that fails ("/*": ARexx
; error 6, unterminated comment): the result string is read from address
; 6 (the lower half of the ExecBase pointer and on).
; Correct: an error has no result string - "".
; RexxMast is not started in the sessions: the program starts it
; (SYS:System/RexxMast, which writes nothing) and waits for its port.
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
NPrint "empty ",a$=""
End

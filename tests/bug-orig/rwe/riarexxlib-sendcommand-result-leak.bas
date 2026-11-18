; riarexxlib-sendcommand-result-leak (#546) - RXSendCommand copies the
; reply's result string (AllocSt) but never deletes the result argstring
; (the sender has to) and never frees the copy: two strings lost per
; call.  The program sends RexxMast (port REXX) 50 string programs that
; return 4000 characters and compares the free memory before and after
; (one call before the first measurement starts RexxMast's work).  The
; program text is one quoted word (the tokenizer splits at spaces).
; Correct: the memory comes back - less than 100000 bytes missing (the
; leak is about 400000 bytes).
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
q$=Chr$(34)
a$=RXSendCommand("REXX",q$+"return 'x'"+q$)
m0=AvailMem_(0)
For i=1 To 50
  a$=RXSendCommand("REXX",q$+"return copies('x',4000)"+q$)
Next
m1=AvailMem_(0)
NPrint Len(a$)
NPrint "leaked ",m0-m1>100000
End

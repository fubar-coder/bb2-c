; riarexxlib-sendcommand-result-leak (#546) - RXSendCommand copies the
; reply's result string (AllocSt) but never deletes the result argstring
; (the sender has to) and never frees the copy: two strings lost per
; call.  The program sends RexxMast (port REXX) 50 string programs that
; return 4000 characters and compares the free memory before and after
; (one call before the first measurement starts RexxMast's work).  The
; program text is one quoted word (the tokenizer splits at spaces).
; Correct: the memory comes back - less than 100000 bytes missing (the
; leak is about 400000 bytes).
; @rwe riarexxlib
DEFTYPE .l
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

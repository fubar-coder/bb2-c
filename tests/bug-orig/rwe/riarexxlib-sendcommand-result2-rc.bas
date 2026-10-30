; riarexxlib-sendcommand-result2-rc (#545) - with rm_Result1 not 0,
; rm_Result2 is a secondary error code, but RXSendCommand takes any
; Result2 that is not 0 as the address of the result string.  The program
; sends RexxMast (port REXX) a string program that fails ("/*": ARexx
; error 6, unterminated comment): the result string is read from address
; 6 (the lower half of the ExecBase pointer and on).
; Correct: an error has no result string - "".
; @rwe riarexxlib
DEFTYPE .l
p=RXCreatePort("BUGORIG")
RXTokenise On
a$=RXSendCommand("REXX","/*")
NPrint "empty ",a$=""
End

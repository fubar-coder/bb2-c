; riarexxlib-replymsg-no-result-check (#539) - RXReplyMsg code,result$
; always puts an argstring of result$ into rm_Result2, whatever the
; message's rm_Action says.  The program sends "hello" to its own port
; with RXRunCommand (RXCOMM|RXFF_STRING: no RXFF_RESULT), receives it and
; replies it with a result string; the reply waits in its port, so the
; message can still be read.
; Correct: no result was asked for, rm_Result2 stays 0.
; @rwe riarexxlib
DEFTYPE .l
p=RXCreatePort("BUGORIG")
RXTokenise On
RXRunCommand "BUGORIG","hello"
m=RXReadPort
If m
  NPrint "action ",Hex$(Peek.l(m+$1C))
  RXReplyMsg 0,"done"
  NPrint "result2 set ",Peek.l(m+$24)<>0
Else
  NPrint "no message"
EndIf
End

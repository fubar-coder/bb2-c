; riarexxlib-parser-no-limit (#537) - with RXTokenise on the command is
; copied into a 256-byte buffer (tempstorage) and each word into another
; (stringbuffer), both without a limit; in the original both lie in the
; code hunk, directly in front of code (_nextparsearg behind
; tempstorage, which runs right afterwards).  The program sends a
; command of one word of 300 characters to its own port.
; Correct: the message arrives (its argument is the word, or its first
; 255 characters).
; @rwe riarexxlib
; @pass timeout=30
DEFTYPE .l
p=RXCreatePort("BUGORIG")
RXTokenise On
RXRunCommand "BUGORIG",String$("x",300)
m=RXReadPort
NPrint "received ",m<>0
If m
  NPrint "arg ",Left$(RXGetArg(1),3)
  RXReplyMsg 0
EndIf
End

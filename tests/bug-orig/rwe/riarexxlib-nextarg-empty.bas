; riarexxlib-nextarg-empty (#534) - RXNextArg advances to the next
; argument only when the one it returned is not empty (it tests the
; length, not the argument slot): at an empty argument it returns "" for
; ever.  The program sends "a b c" to its own port (three arguments),
; receives the message and makes argument 2 empty (its first byte 0: the
; tokenizer cannot make an empty argument), then calls RXNextArg three
; times.
; Correct: a, "", c.
; @rwe riarexxlib
DEFTYPE .l
p=RXCreatePort("BUGORIG")
RXTokenise On
RXRunCommand "BUGORIG","a b c"
m=RXReadPort
If m
  Poke.b Peek.l(m+$2C),0
  NPrint RXNumArgs," args"
  For i=1 To 3
    NPrint "<",RXNextArg,">"
  Next
  RXReplyMsg 0
Else
  NPrint "no message"
EndIf
End

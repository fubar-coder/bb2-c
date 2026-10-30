; riarexxlib-replymsg-stays-current (#541) - RXReplyMsg replies the
; current message but does not clear it: until the next
; RXReadPort/RXWaitPort the argument functions still read the message
; that now belongs to its sender again.  The program sends "hello" to its
; own port, receives it, replies it and asks for its arguments (the
; sender, the program itself, has not freed it yet: it gets the reply at
; its next RXReadPort).
; Correct: after the reply there is no current message: 0 arguments, "".
; @rwe riarexxlib
DEFTYPE .l
p=RXCreatePort("BUGORIG")
RXTokenise On
RXRunCommand "BUGORIG","hello"
m=RXReadPort
If m
  NPrint RXGetArg(1)
  RXReplyMsg 0
  NPrint RXNumArgs," args"
  NPrint "<",RXGetArg(1),">"
Else
  NPrint "no message"
EndIf
End

; riarexxlib-parser-empty-word (#536) - the tokenizer gives the length 0
; both for an empty quoted word and for the end of the command: an empty
; word ("") ends the arguments.  The program sends a "" c to its own port
; and prints the arguments.
; Correct: three arguments, the second empty - a, "", c.
; @fixed #536
; @rwe riarexxlib
DEFTYPE .l
p=RXCreatePort("BUGORIG")
RXTokenise On
q$=Chr$(34)
RXRunCommand "BUGORIG","a "+q$+q$+" c"
m=RXReadPort
If m
  n=RXNumArgs
  a$=Str$(n)+":"
  For i=1 To n
    a$=a$+" <"+RXGetArg(i)+">"
  Next
  NPrint a$
  RXReplyMsg 0
Else
  NPrint "no message"
EndIf
End

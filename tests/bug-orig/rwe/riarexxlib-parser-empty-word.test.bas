; riarexxlib-parser-empty-word (#536) - the tokenizer gives the length 0
; both for an empty quoted word and for the end of the command: an empty
; word ("") ends the arguments.  The program sends a "" c to its own port
; and records the arguments.
; Correct: three arguments, the second empty - a, "", c.
DEFTYPE .l

!TestCase{empty_word}
  p=RXCreatePort("BUGORIG")
  RXTokenise On
  q$=Chr$(34)
  RXRunCommand "BUGORIG","a "+q$+q$+" c"
  m=RXReadPort
  !Require{m<>0,"message_received"}
  n=RXNumArgs
  a$=Str$(n)+":"
  For i=1 To n
    a$=a$+" <"+RXGetArg(i)+">"
  Next
  RXReplyMsg 0
  !CheckStrEq{a$,"3: <a> <> <c>","arguments"}
!EndTestCase

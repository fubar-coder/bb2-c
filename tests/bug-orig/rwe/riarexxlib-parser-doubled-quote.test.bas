; riarexxlib-parser-doubled-quote (#535) - the tokenizer counts the
; quotes of a quoted word: after a doubled quote (one quote character)
; the closing quote is the third, which does not close, and the
; characters behind it are taken into the word.  The program sends
; "e""f" c d to its own port and records the arguments.
; Correct: three words - e"f, c, d.
DEFTYPE .l

!TestCase{doubled_quote}
  p=RXCreatePort("BUGORIG")
  RXTokenise On
  q$=Chr$(34)
  RXRunCommand "BUGORIG",q$+"e"+q$+q$+"f"+q$+" c d"
  m=RXReadPort
  !Require{m<>0,"message_received"}
  n=RXNumArgs
  a$=Str$(n)+":"
  For i=1 To n
    a$=a$+" <"+RXGetArg(i)+">"
  Next
  RXReplyMsg 0
  !CheckStrEq{a$,"3: <e"+q$+"f> <c> <d>","arguments"}
!EndTestCase

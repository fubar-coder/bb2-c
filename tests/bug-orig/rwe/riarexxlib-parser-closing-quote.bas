; riarexxlib-parser-closing-quote (#538) - after the closing quote of a
; quoted word the tokenizer skips the next character: a character glued
; to the quote is lost, and after a quoted last word the terminating 0 is
; skipped, so the rest of an earlier, longer command in the parser's
; buffer becomes further arguments.  The program sends three commands to
; its own port and prints the arguments of each: "one two three four five
; six", then say "hi", then "g"h.
; Correct: six words; say, hi; g, h (the character after the closing
; quote starts a new word, user decision W4 Q2 (a)).
; @fixed #538
; @rwe riarexxlib
DEFTYPE .l
p=RXCreatePort("BUGORIG")
RXTokenise On
q$=Chr$(34)
Dim c$(3)
c$(1)="one two three four five six"
c$(2)="say "+q$+"hi"+q$
c$(3)=q$+"g"+q$+"h"
For k=1 To 3
  RXRunCommand "BUGORIG",c$(k)
  m=RXReadPort
  If m
    n=RXNumArgs
    a$=Str$(n)+":"
    For i=1 To n
      a$=a$+" <"+RXGetArg(i)+">"
    Next
    NPrint a$
    RXReplyMsg 0
    m=RXReadPort
  Else
    NPrint "no message"
  EndIf
Next
End

; literal-strings - literal characters (spec compiler/literal-characters, #1018):
; the Latin-1 bytes of this text become literal characters ($90 cc, tedsrc
; tok), the umlauts the codes 26-31; a string literal, Data$, Dc.b, a quoted
; constant and a string in a macro hold the byte of each (the umlaut code
; as gs maps it); a comment with one is skipped (café). The case prints the
; codes (asserted in literal-strings.test.toml).
; Correct: the values of the toml; the original compiler gives the same codes
; (a plain test, judged on both sides).
Macro m
NPrint Asc("ßü"),Asc(Right$("ßü",1))
End Macro
.d
Data$ "éÿ"
.c
Dc.b "èà",0
; the Dc.b above is of odd length: Even keeps the end code of the original
; compiler on an even address (#979), so the program also ends there
Even
!TestCase{literals}
  a$="Größe" ; café
  NPrint Len(a$)
  For i=1 To Len(a$)
    NPrint Asc(Mid$(a$,i,1))
  Next
  Restore d
  Read b$
  NPrint Asc(b$)," ",Asc(Right$(b$,1))
  NPrint Peek.b(?c)&255," ",Peek.b(?c+1)&255
  MOVE.l #"ß",d0 : PutReg d0,q.l
  NPrint q
  !m
!EndTestCase

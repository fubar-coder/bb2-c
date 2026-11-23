; compiler-include-quote-colon (#17) - blitz2.s takes the rest of the
; line after a quoted Include name into the name: Include "inc.bb2" :
; a=1 opens "CORPUS:src/inc.bb2:a=1" (Can't open Include).
; Correct: after the closing quote of an Include name only a comment or
; the line end may follow; a ':' is a syntax error at the Include line
; (the rest of the line after an include is not compiled, #996).
!TestCase{include_quote_colon}
  IncDir "CORPUS:src/"
  Include "inc.bb2" : a=1
  !CheckEq{a,1,"a"}
!EndTestCase

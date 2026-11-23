; compiler-incdir-quote-garbage (#17) - blitz2.s takes the rest of the
; line after IncDir into the path, also after the closing quote: IncDir
; "CORPUS:src/" x sets the path "CORPUS:src/x".
; Correct: after the closing quote of a quoted path only ':', a comment
; or the line end may follow; other text is a syntax error at the IncDir
; line.
!TestCase{quote_garbage}
  IncDir "CORPUS:src/" x
  !CheckEq{1,1,"after"}
!EndTestCase

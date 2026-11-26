; literal-stray - a literal character outside a string and a comment (spec
; compiler/literal-characters, #1018): the word $90E9 of literallib, a
; dummy token without the statement flag, is a Syntax Error at its line;
; no code is written
; Correct: the compile ends with the error, nothing runs.
!TestCase{stray}
  NPrint "a"
  é
!EndTestCase

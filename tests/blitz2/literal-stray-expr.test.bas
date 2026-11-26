; literal-stray-expr - a literal character in an expression (spec
; compiler/literal-characters, #1018): the word $90E9 of literallib, a
; dummy token without the function flag, is a Syntax Error at its line
; Correct: the compile ends with the error, nothing runs.
!TestCase{stray_expr}
  NPrint "a"
  a=é
!EndTestCase

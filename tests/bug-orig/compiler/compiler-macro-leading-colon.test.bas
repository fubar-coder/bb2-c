; compiler-macro-leading-colon (#942) - domacro reads the macro name and
; backs up one character; .gather then collects every character, and a ':'
; directly behind the name becomes the first character of the macro text
; (the clean-up .mclean removes only trailing colons).  Macro a:1:End Macro
; stores ":1": v=!a becomes v=:1, an assignment without a value and the
; statement 1 (a syntax error).
; Correct: the macro text is "1", v is 1.
DEFTYPE .l
Macro a:1:End Macro

!TestCase{leading_colon}
  v=!a
  !CheckEq{v,1,"macro_value"}
!EndTestCase

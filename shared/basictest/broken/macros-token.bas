; macros-token.bas - deliberately broken input of the check of the macro
; names (check-macros.cmake, ctest basictest_macros_tokens_broken): NPrint
; and loadshape (a token without its case) are tokens of blitz2's
; libraries, TestCase is none
Macro TestCase
  .tc_`1
End Macro
Macro NPrint
  TLCheck `1,0,0
End Macro
Macro loadshape
  TLCheck `1,0,0
End Macro

; macros-newtype-token.bas - deliberately broken input of the check of the
; NewType names of the macro text (check-macros.cmake, ctest
; basictest_macros_newtype_broken): the field cls is the token Cls of
; blitz2's libraries, the NewType TLRun and the field rc are none
NEWTYPE .TLRun
  rc.l
  cls.w
End NEWTYPE
Macro RunProgram
TLRunProgram(`1,`2)
End Macro

; compiler-macro-error-freemem-nested (#1203) - a compile error in a
; nested macro expansion: !outer expands to !m and the rest of the line,
; !m to a$="..." (30 x), and with b$="..." (100 x) the line does not fit
; in the Macro Buffer of 128 bytes: "Macro Buffer Overflow". The error path
; of blitz2 freed neither the inner expansion buffer (myline) nor the
; outer one (myoline, freed only after a successful expansion): two blocks
; of 128 bytes left at the compiler's end. inject track reports them.
; Correct: the compile fails and the compiler frees what it allocated.
Macro m
a$="xxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
End Macro
Macro outer
!m
End Macro
!TestCase{macro_error_freemem_nested}
  !outer:b$="xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
  !CheckEq{1,1,"hello"}
!EndTestCase

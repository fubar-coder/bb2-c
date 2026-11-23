; compiler-macro-error-freemem (#1203) - a compile error on a line that
; came from a macro expansion: !m expands to a$="..." (30 x) and the rest
; of the line, b$="..." (100 x), does not fit in the Macro Buffer of 128
; bytes: "Macro Buffer Overflow". The error path of blitz2 did not free
; the expansion buffer (myline, macrolen bytes): one block of 128 bytes
; left at the compiler's end. inject track reports it.
; Correct: the compile fails and the compiler frees what it allocated.
Macro m
a$="xxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
End Macro
!TestCase{macro_error_freemem}
  !m:b$="xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
  !CheckEq{1,1,"hello"}
!EndTestCase

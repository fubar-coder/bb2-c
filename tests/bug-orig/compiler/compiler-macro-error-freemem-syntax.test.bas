; compiler-macro-error-freemem-syntax (#1203) - a compile error in the
; text of an expanded macro: !m expands to a=), a Syntax Error, with the
; default Macro Buffer of 8192 bytes. The error path of blitz2 did not
; free the expansion buffer (myline, macrolen bytes): one block of 8192
; bytes left at the compiler's end. inject track reports it.
; Correct: the compile fails and the compiler frees what it allocated.
Macro m
a=)
End Macro
!TestCase{macro_error_freemem_syntax}
  !m
  !CheckEq{1,1,"hello"}
!EndTestCase

; compiler-findlab-emptyname (#30) - fetchconst takes the name after "#" in
; an expression with makename and does not test that there was one: the
; empty name goes to findlab, which with length 0 compares up to 65536 bytes
; when the list holds an entry with an empty name (here none: the search
; fails and the error is "Constant Not Found").
; Correct: "#" without a name is a syntax error, and findlab compares no
; byte past an empty name.
; @fixed #30
a=#+1
NPrint a
End

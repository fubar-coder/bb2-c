; compiler-findlab-emptyname-goto (#30) - makealab takes the label name after
; Goto with makename2 and does not test that there was one: an empty label
; is created with an outstanding reference, and the second Goto runs findlab
; with length 0 against it (the comparison goes past the empty name).
; Correct: Goto without a label name is a syntax error.
; @fixed #30
Goto
Goto
NPrint "ok"
End

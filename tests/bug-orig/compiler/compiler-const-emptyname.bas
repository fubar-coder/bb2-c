; compiler-const-emptyname (#7) - handleconst takes the constant's name with
; makename and does not test that there was one: "#=5" creates a constant
; with an empty name and the program compiles.
; Correct: a constant definition without a name is a syntax error.
#=5
NPrint "ok"
End

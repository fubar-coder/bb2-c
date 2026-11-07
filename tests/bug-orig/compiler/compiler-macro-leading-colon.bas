; compiler-macro-leading-colon (#942) - domacro reads the macro name and
; backs up one character; .gather then collects every character, and a ':'
; directly behind the name becomes the first character of the macro text
; (the clean-up .mclean removes only trailing colons).  Macro a:1:End Macro
; stores ":1": NPrint !a becomes NPrint :1, an empty NPrint and the
; statement 1 (a syntax error).
; Correct: the macro text is "1", the program prints 1.
; @fixed #942
Macro a:1:End Macro
NPrint !a
End

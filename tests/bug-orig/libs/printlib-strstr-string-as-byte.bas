; printlib-strstr-string-as-byte (#242) - Str$ and UStr$ choose the
; conversion by the type of the argument and have no case for a string:
; it falls into the byte case and gives the low byte of the string's
; address as a number (its length, 1 to 4 characters, depends on the
; address; the program only compares, so that its output does not).
; Correct: Str$ and UStr$ of a string give the string itself, also as a
; part of a longer string expression.
; @fixed #242
; @configs clib
a$="Hallo"
If Str$(a$)=a$ Then NPrint "a same" Else NPrint "a differs"
If UStr$(a$)=a$ Then NPrint "b same" Else NPrint "b differs"
b$="<"+Str$(a$+"!")+">"
If b$="<Hallo!>" Then NPrint "c same" Else NPrint "c differs"
End

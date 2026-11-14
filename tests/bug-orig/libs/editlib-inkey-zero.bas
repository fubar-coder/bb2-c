; editlib-inkey-zero (#128) - Inkey$(n) with n = 0 jumps to its end
; before MOVEQ #0,d6 and returns the caller's d6 as the length of the
; string: a string of a random length.  Here d6 is set to 5 and 3 right
; before the call (inline assembler).
; Correct: Inkey$(0) reads nothing and is the empty string.
; @fixed #128
; @configs clib
MOVEQ #5,d6 : NPrint "len ",Len(Inkey$(0))
MOVEQ #3,d6 : a$=Inkey$(0) : NPrint "[",a$,"]"
End

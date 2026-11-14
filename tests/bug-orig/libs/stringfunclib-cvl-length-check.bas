; stringfunclib-cvl-length-check (#288) - Cvl reads 4 bytes of its
; string without looking at the string's length.  The fix of #288
; (release code) reads only the string's own bytes; a shorter string is a
; programming error, and the original has no error check for it, not
; even with runtime error checking.
; Here with runtime error checking: Cvl("abc"); the original reads the
; terminating 0 behind the string as the fourth byte ($61626300).
; Correct: the error check refuses the short string: "Cvl needs a String
; of at least 4 Characters".
; @fixed #288
; @configs clib
; @pass runtime_errors=on
DEFTYPE .l
NPrint "before"
x=Len("WXYZ") : v=Cvl("abc") : NPrint "cvl abc ",Hex$(v)
End

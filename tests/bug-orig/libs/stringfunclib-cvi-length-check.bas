; stringfunclib-cvi-length-check (#288) - Cvi reads 2 bytes of its
; string, Cvl and Cvq 4, without looking at the string's length.  The fix
; of #288 (release code) reads only the string's own bytes; a shorter
; string is a programming error, and the original has no error check for
; it, not even with runtime error checking.
; Here with runtime error checking: Cvi, Cvl and Cvq of strings that are
; long enough (no error), then Cvi("a"); the original reads the
; terminating 0 behind the string as the second byte ($6100).
; Correct: the error check refuses the short string: "Cvi needs a String
; of at least 2 Characters".
; @fixed #288
; @configs clib
; @pass runtime_errors=on
DEFTYPE .l
v=Cvi("ab") : NPrint "cvi ab ",v
v=Cvl("abcd") : NPrint "cvl abcd ",Hex$(v)
q.q=Cvq("abcd") : NPrint "cvq abcd ",Hex$(Peek.l(&q))
x=Len("WXYZ") : v=Cvi("a") : NPrint "cvi a ",v
End

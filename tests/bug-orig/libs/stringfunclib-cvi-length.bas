; stringfunclib-cvi-length (#288) - Cvi always reads 2 bytes of its
; string, Cvl and Cvq 4, without looking at the string's length: a shorter
; string gets the bytes behind it (its terminating 0, then what an earlier
; expression left in the string buffer).  Before each call
; Len("WXYZWXYZ") leaves these characters at the start of the buffer.
; Correct: only the string's own bytes count, the missing ones are 0:
; Cvl("a") = $61000000, Cvl("ab") = $61620000, Cvl("") = 0, Cvi("") = 0,
; Cvi("a") = $6100.
; @fixed #288
; @configs clib
DEFTYPE .l
x=Len("WXYZWXYZ") : v=Cvl("a") : NPrint "cvl a ",Hex$(v)
x=Len("WXYZWXYZ") : v=Cvl("ab") : NPrint "cvl ab ",Hex$(v)
x=Len("WXYZWXYZ") : v=Cvl("") : NPrint "cvl ",Hex$(v)
x=Len("WXYZWXYZ") : v=Cvi("") : NPrint "cvi ",v
x=Len("WXYZWXYZ") : v=Cvi("a") : NPrint "cvi a ",v
x=Len("WXYZWXYZ") : v=Cvl("abcd") : NPrint "cvl abcd ",Hex$(v)
x=Len("WXYZWXYZ") : v=Cvi("ab") : NPrint "cvi ab ",v
End

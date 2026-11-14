; stringfunclib-instr-startpos (#287) - Instr(s$,f$,Startpos) starts the
; search at Startpos-1 without checking Startpos: a start position of 0 or
; below compares the bytes before the string, and an empty find string is
; "found" there: Instr(s$,"",0) is 0, Instr(s$,"",-5) is -5.
; Correct: a start position below 1 searches from the first character, as
; Instr(s$,f$) does: 1 for the empty find string, 2 for "b".
; @fixed #287
; @configs clib
a$="abc"
NPrint "empty0 ",Instr(a$,"",0)
NPrint "empty-5 ",Instr(a$,"",-5)
NPrint "b0 ",Instr(a$,"b",0)
NPrint "empty1 ",Instr(a$,"",1)
End

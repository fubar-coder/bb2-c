; rilesdebuglib-addprocessptr-full-check (#700) - AddProcessPtr appends to
; the table of 20 entries without a count: the 21st goes over the end mark
; and the bytes behind the table.  Here with runtime error checking: a
; program that adds more entries than the table holds is told so.
; Correct: the error check refuses the 21st entry: "AddProcessPtr table
; holds at most 20 entries".
; @fixed #700
; @rwe rilesdebuglib
; @pass runtime_errors=on
; @pass timeout=30
DEFTYPE .l
b=AllocMem_(64,$10001)
If b=0 Then NPrint "no memory" : End
Poke$ b,"<%U>"
Poke$ b+16,"LEAK"
NPrint "before"
For i=1 To 21 : AddProcessPtr Chr$(64+i),b+16 : Next
D_InitParse b
NPrint ProcessString
End

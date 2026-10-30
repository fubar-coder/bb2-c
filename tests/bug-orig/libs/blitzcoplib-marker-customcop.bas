; blitzcoplib-marker-customcop (#405) - Slice marks the place of a line
; 256 wait with the copper move $01FE1234; the copy loop of inscop2 looks
; for this long in every inserted block, also in the string of CustomCop:
; it replaces it by $01FE0000 or the line 255 wait (blwait) and turns the
; following word of the source into a wait (low byte, 1).
; Here: CustomCop at line 100 with the string $01FE1234 $01800F00 (a MOVE
; to the unused register $1FE, a colour move), in Amiga mode; the program
; prints its string variable and the copper list (CopLoc, CopLen).  The
; string variable is not changed (CustomCop gets a copy in the string
; work buffer, which is changed); the original puts blwait and the wait
; $80010F00 into the list instead of the two moves.
; Correct: the list holds the instructions as given.
; @configs clib
DEFTYPE .l
c$=Mki$($1FE)+Mki$($1234)+Mki$($180)+Mki$($F00)
CustomCop c$,100
NPrint "string ",Hex$(Peek.l(&c$))," ",Hex$(Peek.l(&c$+4))
l=CopLoc : n=CopLen
NPrint "len ",n
Print "list"
For i=0 To n-4 Step 4 : Print " ",Hex$(Peek.l(l+i)) : Next
NPrint ""
End

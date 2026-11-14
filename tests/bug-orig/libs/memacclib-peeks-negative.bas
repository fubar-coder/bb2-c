; memacclib-peeks-negative (#214) - Peeks$(Address,Length) counts the
; length down unsigned and only catches 0: a negative length copies up to
; 4 GB into the string buffer and overwrites the memory behind it (the
; program crashes).
; Correct: a negative length gives the empty string, as 0 does.
; @fixed #214
; @configs clib
; @pass timeout=30
DEFTYPE .l
n=-1
NPrint "[",Peeks$(0,n),"] ",Len(Peeks$(0,-5))
NPrint "[",Peeks$(0,0),"]"
End

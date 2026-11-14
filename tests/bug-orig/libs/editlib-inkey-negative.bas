; editlib-inkey-negative (#1031) - Inkey$(n) counts its n attempts as an
; unsigned long (SUBQ.l #1,d7:BNE 'loop): a negative n is about 4 billion
; attempts to read a character, and every character that comes is
; appended to the string, past the end of the string buffer.  The
; original has no error check for it (a programming error), not even
; with runtime error checking.
; Here with runtime error checking: the input is a file of the two
; characters "xy" (FileInput: one character per attempt, none at the end
; of the file), then Inkey$(-1): the original reads "xy" and then makes
; its other attempts at the end of the file - it does not come back (the
; session times out).
; Correct: the error check refuses the negative count: "Can't Inkey$ a
; negative number of Characters!".
; @fixed #1031
; @configs clib
; @pass runtime_errors=on
; @pass timeout=30
If WriteFile(0,"RAM:editlib-inkey-negative") Then FileOutput 0 : Print "xy" : CloseFile 0
DefaultOutput
If ReadFile(0,"RAM:editlib-inkey-negative")=0 Then NPrint "no file" : End
NPrint "before"
FileInput 0
a$=Inkey$(-1)
DefaultInput
CloseFile 0
NPrint "len ",Len(a$)
End

; ridebuglib-name-no-limit (#613) - the commands work in their error
; check routines (only in a program compiled with runtime error
; checking): AddVarTrace/DelVarTrace copy the name into the 252
; characters of the message block mess1 without a limit; in the
; original the block lies in the error check area, directly followed by
; the check routines.  DelVarTrace with a name of 300 characters $4A,$FC
; (ILLEGAL) overwrites the check code behind mess1 (_addvartracechk);
; the next AddVarTrace executes it: an illegal instruction exception
; (the session ends in a timeout).
; Correct: the name is cut to the block, AddVarTrace works, the program
; goes on.
; @rwe ridebuglib
; @pass runtime_errors=on
DEFTYPE .l
a=1
n$=""
For i=1 To 150 : n$=n$+Chr$($4A)+Chr$($FC) : Next
NPrint "before"
DelVarTrace n$
AddVarTrace a,"a",1
NPrint "after"
End
